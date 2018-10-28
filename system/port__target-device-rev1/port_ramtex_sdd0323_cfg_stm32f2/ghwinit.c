/************************** ghwinit.c *****************************

   Low-level driver functions for the SSD0323 LCD display controller
   initialization and error handling.

   The SSD0323 controller is assumed to be used with a LCD module.

   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is copied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Revision date:     4-09-04
   Revision Purpose:  Bug in simulator interface removed. Software
                      tracking of block window wrapping corrected.
   Revision date:     030804
   Revision Purpose:  Simulator update simplified
   Revision date:     27-10-04
   Revision Purpose   GPALETTE_GREY type used for palette to save a
                      few ROM bytes
                      GHW_INVERSE_DISP,GHW_XOFFSET,GHW_YOFFSET added
   Revision date:     4-1-05
   Revision Purpose:  ghw_puterr speed optimized with better use of
                      range settings.
   Revision date:     27-12-05
   Revision Purpose:  ghw_init power-on test corrected (bug in revision 1.3 only)
   Revision date:     04-24-06
   Revision Purpose:  SSD1329 support added
   Revision date:     15-02-07
   Revision Purpose:  GCTRL_MUX_RATIO setting now adjust to follow internal
                      video-ram size
   Revision date:     080808
   Revision Purpose:  Updated for new switch handling
   Revision date:     27-02-2015
   Revision Purpose:  Support for use of vertical storage units
                      and 90-(270) degree rotation added
                      Device variant specific initialization and control
                      functions moved to ghwinitctrl.c to maximize
                      reuse of ghwinit.c
   Revision date:     30-03-2017
   Revision Purpose:  ghw_set_state function. Change split pointer assignment and
                      NULL compare in two statements to increase compiler portability.
   Revision date:
   Revision Purpose:

   Version number: 1.11
   Copyright (c) RAMTEX Engineering Aps 2004-2016

*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

//#define WR_RD_TEST   /* Define to include write-read-back test in ghw_init() */

#ifdef SGPCMODE
 #include <stdio.h>    /* printf for test messages */
#endif

#ifdef GBASIC_INIT_ERR

/***********************************************************************/
/** All static LCD driver data is located here in this ghwinit module **/
/***********************************************************************/

/* Use software font */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR  b[8];           /* Symbol data, fixed size = 8 bytes */
   }
GCODE FCODE sysfontsym[0x80] =
   {
   /* The default font MUST be a monospaced black & white (two-color) font */
   #include <sfs0323.sym> /* System font symbol table */
   };

/* Default system font */
GCODE GFONT FCODE SYSFONT =
   {
   6,      /* width */
   8,      /* height */
   sizeof(sysfontsym[0])-sizeof(GSYMHEAD), /* number of data bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL) sysfontsym,  /* pointer to array of SYMBOLS */
   0x80,   /* num symbols in sysfontsym[] */
   NULL    /* pointer to code page */ /* NULL means code page is not used */
   };

GHWCOLOR ghw_tmpbuf[TMPBUFSIZE];

#ifdef GBUFFER
   #ifdef GHW_ALLOCATE_BUF
      /* <stdlib.h> is included via gdisphw.h */
      GHWCOLOR *gbuf = NULL;           /* Graphic buffer pointer */
      static SGBOOL gbuf_owner = 0;   /* Identify pointer ownership */
   #else
      GHWCOLOR gbuf[GBUFSIZE];         /* Graphic buffer */
   #endif
   GXT GFAST iltx,irbx;      /* "Dirty area" speed optimizers in buffered mode */
   GYT GFAST ilty,irby;
   SGBOOL  ghw_upddelay;     /* Flag for delayed update */
#endif /* GBUFFER */

#ifdef GHW_INTERNAL_CONTRAST
static SGUCHAR ghw_contrast; /* Current contrast value */
#endif

/* Active foreground and background color */
GCOLOR ghw_def_foreground;
GCOLOR ghw_def_background;
GHWCOLOR ghw_foreground;
GHWCOLOR ghw_background;

SGBOOL glcd_err;      /* Internal error */
#ifndef GNOCURSOR
GCURSOR ghw_cursor;   /* Current cursor state */
#endif

/* Operative palette (copy of grey scale palette for palette read back) (normalized to 0-255)*/
GPALETTE_GREY ghw_palette_opr[(1<<GDISPPIXW)];

/*
   Arrays with fast mask values used by the low-level graphic functions
   and view ports.
*/
/* B&W symbol pixel mask */
GCODE SGUCHAR FCODE sympixmsk[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

/* Storage unit handling masks */
#if (GDISPPIXW==4)
#if (GHWPIXPSU==2)
GCODE GHWCOLOR FCODE pixmsk[GHWPIXPSU]    = {0xf0,0x0f};
GCODE GHWCOLOR FCODE startmask[GHWPIXPSU] = {0xff,0x0f};
GCODE GHWCOLOR FCODE stopmask[GHWPIXPSU]  = {0xf0,0xff};
GCODE GHWCOLOR FCODE shiftmsk[GHWPIXPSU]  = {4,0};
#elif (GHWPIXPSU==4)
GCODE GHWCOLOR FCODE pixmsk[GHWPIXPSU]    = {0xf000,0x0f00,0x00f0,0x000f};
GCODE GHWCOLOR FCODE startmask[GHWPIXPSU] = {0xffff,0x0fff,0x00ff,0x000f};
GCODE GHWCOLOR FCODE stopmask[GHWPIXPSU]  = {0xf000,0xff00,0xfff0,0xffff};
GCODE GHWCOLOR FCODE shiftmsk[GHWPIXPSU]  = {12,8,4,0};
#endif
#else
 #error Illegal / unsupported GDISPPIXW setting in gdispcfg.h
#endif

/****************************************************************/
/** Low level interface functions used only by ghw_xxx modules **/
/****************************************************************/

/*
   Fast set or clear of LCD module RAM buffer
       dat  = data byte

   Internal ghw function
*/
static void ghw_bufset( GHWCOLOR dat )
   {
   #ifdef GBUFFER
   GBUFINT cnt;
   cnt = GBUFSIZE-1;
   /* Clear video buffer in RAM*/
   do
      {
      gbuf[cnt] = dat;
      }
   while(cnt-- != 0);

   /* Prepare for flush of full screen */
   iltx = 0;
   ilty = 0;
   irbx = GDISPW-1;
   irby = GDISPH-1;
   ghw_upddelay = 0;

   #else

   GXT x; GYT y;
   /* Set wrap range in advance */
   ghw_set_yrange(0,GDISPH-1);
   ghw_set_xrange(0,GDISPW-1);
   #ifdef GHW_USING_VBYTE
   for (y=0;y < GDISPH;y+=GHWPIXPSU)
   #else
   for (y=0;y < GDISPH;y++)
   #endif
      {
      #ifdef GHW_USING_VBYTE
      for (x=0;x<GDISPW;x++)
      #else
      for (x=0;x<GDISPW;x+=GHWPIXPSU)
      #endif
         {
         ghw_auto_wr(dat);
         }
      }
   #endif
   ghw_updatehw();  /* Flush to display hdw or simulator */
   }

/*
   Repeat color for all pixels in GCOLOR storage unit
*/
GHWCOLOR ghw_prepare_color(GCOLOR color)
   {
   GHWCOLOR col;
   col = (GHWCOLOR) (color & GPIXMAX);
   col |= (col << GDISPPIXW);
   #if (GDISPHCW > 8)
   col |= (col << GDISPPIXW*2);
   #endif
   return col;
   }

/*
   Change default (palette) colors
*/
void ghw_setcolor(GCOLOR fore, GCOLOR back)
   {
   /* Force resonable palette values */
   GLIMITU(fore,GPIXMAX);
   GLIMITU(back,GPIXMAX);
   /* Update active colors */
   ghw_def_foreground = fore;
   ghw_def_background = back;
   ghw_foreground = ghw_prepare_color(fore);
   ghw_background = ghw_prepare_color(back);
   }

#if (defined( WR_RD_TEST ) && !defined(GBUFFER))
/*
   Make write-readback test on controller memory.

   This test returns ok (== 0) when the write-readback test succeded. This indicates that
   the processor / display hardware interface / library configuration combination is
   working ok.

   This test will fail if some databus or control signals is not connected correctly.

   This test may fail if illegal GCTRLW, GCTRLH, GDISPPIXW configuration settings
   cause overrun of the on-chip video RAM.

   This test can be exectuted correctly with only logic power on the display module.

   Return 0 if no error,
   Return != 0 if some readback error is detected
   (the bit pattern may give information about connector pins in error)
*/
#define  GPRINTF( format, data ) printf((format), (data) )  /* Info via printf */
//#define  GPRINTF( format, data ) /* Use no info */

static GHWCOLOR ghw_wr_rd_test(void)
   {
   #ifndef GHW_NOHDW
   int i;
   GHWCOLOR msk,result;
   GPRINTF("\n%s","");
   ghw_set_yrange(0,GDISPH-1);
   ghw_set_xrange(0,GDISPW-1);
   for (i = 0, msk = 1; i < 8; i++)
      {
      ghw_auto_wr(msk);
      GPRINTF("0x%02x ", (unsigned int) msk);
      ghw_auto_wr(~msk);
      GPRINTF(" 0x%02x\n", (unsigned int) (~msk & GHWCOLOR_NO_MSK));
      msk <<= 1;
      }
   GPRINTF("\n%s","");
   ghw_set_yrange(0,GDISPH-1);
   ghw_set_xrange(0,GDISPW-1);
   ghw_auto_rd_start();
   for (i=0, msk=1, result=0; i < 8; i++)
      {
      GHWCOLOR val;
      val = ghw_auto_rd();
      result |= (val ^ msk);
      GPRINTF("0x%02x ",   (unsigned int) val);
      val = ghw_auto_rd();
      GPRINTF(" 0x%02x\n", (unsigned int) val);
      result |= (val ^ (~msk));
      msk <<= 1;
      }
   return result & GHWCOLOR_NO_MSK;  /* 0 = Nul errors */
   #else
   return 0; /* 0 = Nul errors */
   #endif
   }
#endif /* WR_RD_TEST */

/*
   Initialize display, clear ram  (low-level)
   Clears glcd_err status before init

   Return 0 if no error,
   Return != 0 if some error
*/
SGBOOL ghw_init(void)
   {
   glcd_err = 0;

   /* Set default colors */
   ghw_setcolor( GHW_PALETTE_FOREGROUND, GHW_PALETTE_BACKGROUND );

   #if (defined( GHW_ALLOCATE_BUF) && defined( GBUFFER ))
   if (gbuf == NULL)
      {
      /* Allocate graphic ram buffer */
      if ((gbuf = calloc(ghw_gbufsize(),1)) == NULL)
         {
         glcd_err = 1;
         return 1;                   /* Some lowlevel io error detected */
         }
      else
         gbuf_owner = 1;
      }

   #endif

   /* Initialize hardware (and/or PC simulator) */
   ghw_ctrl_init();

   if (glcd_err)
      return 1;                   /* Some lowlevel io error detected */

   ghw_cont_set(95);              /* Set ghw_contrast / init OLED intensity */
   /*ghw_dispon();*/              /* Turn on here to ease first hardware tests */

   #if (defined( WR_RD_TEST ) && !defined(GBUFFER))
   /*
      NOTE:
      The call of ghw_wr_rd_test() should be commented out in serial mode.
      In serial mode the display controller  does not provide read-back facility
      and this test will always fail.
   */
   if (ghw_wr_rd_test() != ((GHWCOLOR) 0))
      {
      /* Controller memory write-readback error detected
      (Check the cable or power connections to the display) */
      G_WARNING("Hardware interface error\nCheck display connections\n");  /* Test Warning message output */
      glcd_err = 1;
      }
   #endif
   ghw_bufset( ghw_foreground );
  // ghw_bufset( ghw_background ); /* Clear screen */
   ghw_dispon();

   #ifndef GNOCURSOR
   ghw_cursor = GCURSIZE1;                 /* Cursor is off initially */
   /* ghw_cursor = GCURSIZE1 | GCURON; */  /* Uncomment to set cursor on initially */
   #endif

   return (glcd_err != 0) ? 1 : 0;
   }

/*
   Return last error state. Called from applications to
   check for LCD HW or internal errors.
   The error state is reset by ghw_init and all high_level
   LCD functions.

   Return == 0 : No errors
   Return != 0 : Some errors
*/
SGUCHAR ghw_err(void)
   {
   #if (defined(_WIN32) && defined( GHW_PCSIM))
   if (GSimError())
      return 1;
   #endif
   return (glcd_err == 0) ? 0 : 1;
   }

void ghw_exit(void)
   {
   #if (defined( GHW_ALLOCATE_BUF) && defined( GBUFFER ))
   if (gbuf != NULL)
      {
      if (gbuf_owner != 0)
         {
         /* Buffer is allocated by ginit, so release graphic buffer here */
         free(gbuf);
         gbuf_owner = 0;
         }
      gbuf = NULL;
      }
   #endif
   ghw_ctrl_exit();  /* Release any LCD hardware resources, if required */
   ghw_io_exit();    /* Release any target system specific resources, if required */
   }

/*
   Display a (fatal) error message.

   The LCD display module is always cleared and initialized to
   the system font in advance.
   The error message is automatically centered on the screen
   and any \n characters in the string is processed.

   str = ASCII string to write at display center

   Each character position and line is aligned to a storage unit boundary.
   (i.e. string output can take place independent of any read-modify-write
   limitations in hardware)
   Output is done directly on hardware, also in buffered mode.
*/
void ghw_puterr( PGCSTR str)
   {
   PGCSTR idx;
   GXT xp,w,xcnt;
   GYT yp,h,y,sidx;
   SGUCHAR val;
   #ifndef GHW_USING_VBYTE
   GHWCOLOR pval;
   #endif
   PGSYMBYTE psym;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   #endif

   if (ghw_init() != 0)  /* (Re-) initialize display */
      return;            /* Some initialization error */

   /* Count number of lines in string */
   idx=str;
   if (idx == NULL)
      return;
   xcnt = 1;
   while(*idx)
      {
      if (*(idx++) == '\n')
         xcnt++;
      }

   /* Align symbol frame to storage unit boundary (so look is independent of orientation) */
   w = GPIXMSK(SYSFONT.symwidth+(GHWPIXPSU-1));
   h = GPIXMSK(SYSFONT.symheight+(GHWPIXPSU-1));

   /* Set start character line, and align to vertical storage unit frame */
   yp = GPIXMSK((xcnt*h > GDISPH) ? 0 : ((GDISPH-1)-xcnt*h)/2);

   /* Set character height in pixel lines */
   idx=str;
   do
      {
      xcnt=0;  /* Set start x position so line is centered */
      while ((idx[xcnt]!=0) &&          /* not end of string &&*/
             (idx[xcnt]!='\n') &&       /* not new line character && */
             ((xcnt*GDISPCW) < GDISPW)) /* not screen row overflow */
         {
         xcnt++; /* Add character to current display line */
         }

      /* Calculate start position for centered line, and align to storage unit frame */
      xp = GPIXMSK((GDISPW-xcnt*w)/2);

      /* Display text line */
      while (xcnt-- > 0)
         {
         /* Point to graphic content for character symbol */
         psym = &(sysfontsym[(*idx) & 0x7f].b[0]);

         /* Display rows in symbol */
         for (y = 0; y < SYSFONT.symheight; y++)
            {
            #ifdef GHW_USING_VBYTE
            GHWCOLOR msk = pixmsk[GPIXEL((SGINT)yp+y)];
            #endif
            /* Get symbol row value */
            val = *psym++;

            /* Initiate LCD controller address pointer */
            #ifdef GBUFFER
            gbufidx = GINDEX(xp, (GBUFINT)yp+y );
            #endif
            ghw_set_ypos(yp+y);
            ghw_set_xpos(xp);

            /* Display colums in symbol row */
            #ifdef GHW_USING_VBYTE
            for (sidx = 0; sidx < SYSFONT.symwidth; sidx++)
               {
               if ((GPIXEL(y) == 0) || (y == 0))
                  /* New page row, or first line in symbol, clear to background */
                  ghw_tmpbuf[sidx] = ghw_background;

               if ((val & sympixmsk[sidx]) != 0)
                  {
                  /* bit != 1 -> insert foreground bits */
                  ghw_tmpbuf[sidx] = (ghw_tmpbuf[sidx] & ~msk) | (ghw_foreground & msk);
                  }

               if ((GPIXEL(y) == (GHWPIXPSU-1)) || (y==SYSFONT.symheight-1))
                  {
                  /* Last pixel line in page row, or last row in symbol */
                  #ifdef GBUFFER
                  gbuf[gbufidx++] = ghw_tmpbuf[sidx];
                  #endif
                  ghw_auto_wr(ghw_tmpbuf[sidx]);
                  }
               }
            #else  /* GHW_USING_VBYTE */
            for (sidx = 0, pval = ghw_background;;)
               {
               if ((val & sympixmsk[sidx]) != 0)
                  {
                  /* bit != 1 -> insert foreground bits */
                  pval = (pval & ~pixmsk[GPIXEL(sidx)]) | (ghw_foreground & pixmsk[GPIXEL(sidx)]);
                  }

               sidx++;
               if ((GPIXEL(sidx) == 0) || (sidx >= SYSFONT.symwidth))
                  {
                  /* End of symbol or end of byte reached */
                  #ifdef GBUFFER
                  gbuf[gbufidx++] = pval;
                  #endif
                  ghw_auto_wr(pval);
                  pval = ghw_background;
                  if (sidx >= SYSFONT.symwidth)
                     break;
                  }
               }
            #endif  /* GHW_USING_VBYTE */
            }

         idx++;
         xp += w;  /* Move to next character position*/
         }

      yp += h;     /* Move to next text line */
      if (*idx == '\n')
         idx++; /* skip line terminator (no symbol used) */
      }
   while ((*idx != 0) && (yp < GDISPH));

   #ifdef GBUFFER
   iltx = 0;
   ilty = 0;
   irbx = GDISPW-1;
   irby = GDISPH-1;
   ghw_upddelay = 0;
   #endif
   ghw_updatehw();  /* Flush to display hdw or simulator */
   }

#ifndef GNOCURSOR
/*
   Replace cursor type data (there is no HW cursor support in UC1611)
*/
void ghw_setcursor( GCURSOR type)
   {
   ghw_cursor = type;
   }
#endif

/*
   Turn display off
   (Minimize power consumption)
*/
void ghw_dispoff(void)
   {
   GBUF_CHECK();
   ghw_ctrl_dispoff(); /* Issue hardware specific commands */
   }

/*
   Turn display on
*/
void ghw_dispon(void)
   {
   GBUF_CHECK();
   ghw_ctrl_dispon(); /* Issue hardware specific commands */
   }

#ifdef GHW_INTERNAL_CONTRAST
/*
   Set contrast (Normalized value range [0 : 99] )
   Returns the old value.
*/
SGUCHAR ghw_cont_set(SGUCHAR contrast)
   {
   SGUCHAR tmp;
   GLIMITU(contrast,99);
   tmp = ghw_contrast;
   ghw_contrast = contrast;

   #if (defined( GHW_ALLOCATE_BUF) && defined( GBUFFER ))
   if (gbuf == NULL) {glcd_err = 1; return contrast;}
   #endif

   ghw_ctrl_cont_set(contrast); /* Issue hardware specific commands */

   return tmp;
   }

/*
   Change contrast (Normalized value range [-99 : +99] )
   Returns the old value.
*/
SGUCHAR ghw_cont_change(SGCHAR contrast_diff)
   {
   SGINT tmp;
   tmp =  (SGINT) ((SGUINT) ghw_contrast);
   tmp += (SGINT) contrast_diff;
   GLIMITU(tmp,99);
   GLIMITD(tmp,0);
   return ghw_cont_set((SGUCHAR)tmp);
   }
#endif /* GHW_INTERNAL_CONTRAST */

#ifdef GHW_ALLOCATE_BUF

/*
   Size of buffer requied to save the whole screen state
*/
GBUFINT ghw_gbufsize( void )
   {
   return (GBUFINT) (((GBUFINT)GBUFSIZE) * sizeof(GHWCOLOR) + (GBUFINT) sizeof(GHW_STATE));
   }

#ifdef GSCREENS
/*
   Check if screen buf owns the screen ressources.
*/
SGUCHAR ghw_is_owner( SGUCHAR *buf )
   {
   return (((void *)buf == (void *) gbuf) && (gbuf != NULL)) ? 1 : 0;
   }

/*
   Save the current state to the screen buffer
*/
SGUCHAR *ghw_save_state( SGUCHAR *buf )
   {
   GHW_STATE *ps;
   if (!ghw_is_owner(buf))
      return NULL;

   ps = (GHW_STATE *)(&gbuf[GBUFSIZE]);
   ps->upddelay = (ghw_upddelay != 0);
   #ifndef GNOCURSOR
   ps->cursor = ghw_cursor;
   #endif
   ps->foreground = ghw_def_foreground; /* Palette may vary, save it */
   ps->background = ghw_def_background;
   return (SGUCHAR *) gbuf;
   }

/*
   Set state to buf.
   If buffer has not been initiated by to a screen before, only
   the pointer is updated. Otherwise the buffer
*/
void ghw_set_state(SGUCHAR *buf, SGUCHAR doinit)
   {
   if (gbuf != NULL)
      {
      /* The LCD controller has been initiated before */
      if (gbuf_owner != 0)
         {
         /* Buffer was allocated by ginit, free it so screen can be used instead*/
         free(gbuf);
         gbuf_owner = 0;
         gbuf = NULL;
         }
      }

   if ((doinit != 0) && (buf != NULL))
      {
      /* First screen initialization, just set buffer pointer and
         leave rest of initialization to a later call of ghw_init() */
      gbuf = (GHWCOLOR *) buf;
      gbuf_owner = 0;
      }
   else
      {
      gbuf = (GHWCOLOR *) buf;
      if (gbuf != NULL)
         {
         GHW_STATE *ps;
         ps = (GHW_STATE *)(&gbuf[GBUFSIZE]);

         #ifndef GNOCURSOR
         ghw_cursor = ps->cursor;
         #endif
         ghw_upddelay = 0;        /* Force update of whole screen */
         iltx = 0;
         ilty = 0;
         irbx = GDISPW-1;
         irby = GDISPH-1;
         ghw_updatehw();

         /* Restore cursor */
         ghw_upddelay = (ps->upddelay != 0) ? 1 : 0;
         /* Restore drawing color */
         ghw_setcolor(ps->foreground, ps->background);
         }
      }
   }

#endif  /* GSCREENS */
#endif  /* GHW_ALLOCATE_BUF */
#endif  /* GBASIC_INIT_ERR */



