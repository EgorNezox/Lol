/******************************************************************

  SSD0323 LCD controller simulator for the RAMTEX LCD driver library

  The SSD0323 is simulated by the use of a buffer:

    controller_video_ram  simulates the RAM internally in controller

  Revision date:    03-03-2015
  Revision Purpose: Added support for both horizontal storage
                    and vertical storage units. New function interface.

  Version number: 2.0
  Copyright (c) RAMTEX Engineering Aps 2004-2015

******************************************************************/

#ifndef GHW_PCSIM
#error Only include this file in PC simulation mode
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <gdisphw.h>     /* swprintf */ /* GHW_FAST_SIM_UPDATE */
#include <ssd0323.h>      /* GHW_USING_VBYTE, GDISPHCW, GDISPPIXW, GHWPIXPSU */

#if ((GDISPPIXW != 1) && (GDISPPIXW != 2) && (GDISPPIXW != 4))
  #error GDISPPIXW not supported
#endif

#ifdef GHW_SSD1322
/* Default "hardware palette" to be loaded into display simulator */
static GPALETTE_GREY FCODE palette_gr[(1<<GDISPPIXW)] =
     #include <ggray_4.pal>
     ;
#endif

static int graph_init = 0; /* don't init graphic twice */

#ifdef _WIN32
   #include <gsimintf.h>
   void simputs( SGINT sgstream, const char *chp )
      {
      sgstream = sgstream; /* remove warning */
      GSimPuts(chp);
      }
   #define far /* nothing */
#else
   #error no simulation for this compiler
#endif

void simprintf( SGINT sgstream, const char far *fmt, ...)
   {
   va_list argptr;
   int form_len;
   static char printf_buf[100];

   va_start(argptr, fmt);
   form_len = vsprintf(printf_buf, fmt, argptr);
   va_end(argptr);

   if (form_len >= 100)
      simputs(-1,"\nERROR: Buffer overrun in simsprintf()");

   simputs( sgstream, printf_buf );
   }

/* Internal controller data simulation */
static GHWCOLOR *controller_video_ram; /* simulator module buffer */

/* Copy of user LCD size variables (allows the simulator to be a lib) */
static SGUINT gdisph;      /* Initiated with user gdisph */
static SGUINT gdispw;      /* Initiated with user gdispw */
static SGUINT gdispbh;     /* Buffer height (storage units) */
static SGUINT gdispbw;     /* Buffer width  (storage units) */

static SGUINT xpos, winxb, winxe;
static SGUINT ypos, winyb, winye;
static SGBOOL onoff;       /* Display on off */

static const GPALETTE_RGB gsim_palette[] =
  #if  (GDISPPIXW == 2)
     #include <ggray_2.pal>
  #elif (GDISPPIXW == 4)
     #include <ggray_4.pal>
  #elif (GDISPPIXW == 1)
     #include <ggray_1.pal>
  #endif
  ;

/*
   Write pixels using the pixel storage unit order defined by display controller settings

   shiftmsk[] controls pixel extraction from both MSB-LSB and LSB-MSB aligned
   storage units. Defined in ghwinit.c
*/
#ifdef GHW_USING_VBYTE

/* Write a vertical storage unit to simulator screen */
#define simwr( x, y, val ) \
   { \
   register SGUINT yb,i; \
   register GHWCOLOR dat; \
   yb = (y) * GHWPIXPSU; \
   dat = (val); \
   i=0; \
   do \
      { \
      GSimWrBit((unsigned short)x,(unsigned short)(yb++),(unsigned char) (dat >> shiftmsk[i]) & GPIXMAX); \
      }  \
   while ((++i < GHWPIXPSU) && (yb < gdisph));  \
   }
#else

/* Write a horizontal storage unit to simulator screen */
#define simwr( x, y, val ) \
   { \
   register SGUINT xb,i; \
   register GHWCOLOR dat; \
   xb = (x) * GHWPIXPSU; \
   dat = (val); \
   i=0; \
   do \
      { \
      GSimWrBit((unsigned short)xb++,(unsigned short)(y),(unsigned char) (dat >> shiftmsk[i]) & GPIXMAX); \
      }  \
   while ((++i < GHWPIXPSU) && (xb < gdispw));  \
   }
#endif

static void clr_screen(void)
   {
   SGUINT x;
   SGUINT y;
   if (graph_init == 0) return;
   /* using horizontal storage units */
   for (y = 0; y < gdispbh; y++)
      {
      for (x = 0; x < gdispbw; x++)
         simwr( x, y, 0 );
      }
   GSimFlush();
   }

/* Redraw screen content */
static void redraw_screen(void)
   {
   SGUINT x;
   SGUINT y;
   if (graph_init == 0) return;
   /* using storage units */
   for (y = 0; y < gdispbh; y++)
      {
      for (x = 0; x < gdispbw; x++)
         simwr( x, y, controller_video_ram[(GBUFINT)x + ((GBUFINT)y) * gdispbw] );
      }
   GSimFlush();
   }

/*
   Simulate display on / off
*/
void ghw_dispoff_sim( void )
   {
   /* Off */
   if (onoff)
      clr_screen();
   onoff = 0;
   }

void ghw_dispon_sim( void )
   {
   /* On */
   if (onoff == 0)
      {
      onoff = 1;
      redraw_screen();
      }
   }

/*
   Init cursor and autowrap limits
*/
void ghw_set_xrange_sim(GXT xb, GXT xe)
   {
   GLIMITU(xb,gdispw-1);
   GLIMITU(xe,gdispw-1);
   winxb = GXBYTE(xb);
   winxe = GXBYTE(xe);
   xpos = winxb;
   }

/*
   Init cursor and autowrap limits
*/
void ghw_set_yrange_sim(GYT yb, GYT ye)
   {
   GLIMITU(yb,gdisph-1);
   GLIMITU(ye,gdisph-1);
   winyb = GYBYTE(yb);
   winye = GYBYTE(ye);
   ypos = winyb;
   }

/*
   Simulate internal pointer handling with auto wrap
*/
static void addrinc(void)
   {
   xpos++;
   if (xpos > winxe)
      {
      xpos = winxb;
      if (++ypos > winye)
         ypos = winyb;
      }
   }


/*
   Write to update PC screen
   Simulate autoincrement
*/
void ghw_autowr_sim( GHWCOLOR cval )
   {
   if (graph_init == 0)
      return;   /* Display have not been initiated yet */

   controller_video_ram[xpos + ypos * gdispbw] = cval;

   if (onoff)
      {
      simwr( xpos, ypos, cval);    /* Update simulator */
      #ifndef GHW_FAST_SIM_UPDATE
      GSimFlush();
      #endif
      }
   addrinc();
   }

/*
   Read back data, increment address if inc !=0
*/
GHWCOLOR ghw_autord_sim( SGUCHAR inc )
   {
   GHWCOLOR cp;
   if (graph_init == 0)
      return 0;   /* Display have not been initiated yet */
   cp = controller_video_ram[xpos + ypos*gdispbw];
   if (inc)
      addrinc();
   return cp;
   }

/*
   Update palette on LCD simulator PC screen
*/
char ghw_update_palette_sim( GPALETTE_GREY *palette )
   {
   GPALETTE_RGB SSD0323_palette[(1<<GDISPPIXW)];
   SGUINT i;
   if (palette == NULL)
      return 1;
   for (i = 0; i < sizeof(SSD0323_palette)/sizeof(GPALETTE_RGB); i++, palette++ )
      {
      SSD0323_palette[i].r   = (palette->gr | 0x1f) & 0xff;
      SSD0323_palette[i].g   = (palette->gr | 0x1f) & 0xff;
      SSD0323_palette[i].b   = (palette->gr | 0x1f) & 0xff;  /* Using all colors simulates a grey color display */
      /*SSD0323_palette[i].b   = 0x0; */                     /* Using just red and green simulates a yellow led display */
      }
   return GSimWrPalette((GSIM_PALETTE_RGB *)SSD0323_palette, (1 << GDISPPIXW) ) == 0 ? 0 : 1;
   }

/*
   This function is called last in ghw_init after all display module
   parameters has been initiated.

   NOTE: This user code may cause that this function is in-worked more
   than once
*/
void ghw_init_sim( SGUINT dispw, SGUINT disph )
   {
   SGULONG allocsize;
   #ifdef GHW_USING_VBYTE
   /* Convert height to a whole number of storage units */
   gdispw  = dispw;
   gdispbw = dispw;
   gdispbh = (disph + (GHWPIXPSU-1))/ (GHWPIXPSU);
   gdisph  = gdispbh * GHWPIXPSU;
   allocsize = gdispw*gdispbh*sizeof(GHWCOLOR);
   #else
   /* Convert width to a whole number of storage units */
   gdisph =  disph;
   gdispbh = disph;
   gdispbw = (dispw + (GHWPIXPSU-1))/ GHWPIXPSU;
   gdispw =  gdispbw * GHWPIXPSU;
   allocsize = gdisph*gdispbw*sizeof(GHWCOLOR);
   #endif

   #ifdef _WIN32
   /* Blank display */
   if( graph_init==0 )
      {
      if (controller_video_ram != NULL)
         free( controller_video_ram );  /* The size may have changed */

      if ((controller_video_ram = (GHWCOLOR *)malloc(allocsize)) == NULL)
         {
         return; /* Simulator init error */
         }
      /* Init connection to LCD simulator (only once to optimize speed */
      if (!GSimInitC((unsigned short) gdispw, (unsigned short) gdisph, GDISPPIXW))
         {
         /* Initialization ok */
         #ifdef GHW_SSD1322
         /*  init grey palette */
        if (ghw_update_palette_sim(&palette_gr[0]))
            return; /* Simulator palette init error */
         #endif
         graph_init = 1;
         }
      }

   clr_screen();
   onoff = 0;
   xpos = 0;
   ypos = 0;
   winxb = 0;
   winyb = 0;
   winxe = gdispbw-1;
   winye = gdispbh-1;
   #else
   #error no simulation for this compiler
   #endif
   }

/*
   This function is activated via ghw_exit() when gexit() is called
*/
void ghw_exit_sim( void )
   {
   graph_init = 0;
   if (controller_video_ram != NULL)
      {
      free( controller_video_ram );  /* Deallocate video memory */
      controller_video_ram = NULL;
      }

   #if defined( _WIN32 )
   GSimClose();
   #endif
   }

