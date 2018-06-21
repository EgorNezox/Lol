/************************** ghwinitctrl.c *****************************

   Low-level driver functions for the SSD0323 LCD display controller
   initialization and error handling.

   Revision date:     02-03-2015
   Revision Purpose:  New module for device variant specific initialization
                      and control functions. Functionality moved to this module
                      from ghwinit.c and customized for use with either vertical and
                      horizontal storage units. Now support for 0-90-(180-270) degree
                      screen rotation at compile time.
                      New simulator interface.
   Revision date:
   Revision Purpose:

   Version number: 1.0
   Copyright (c) RAMTEX Engineering Aps 2015

*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

#ifdef SGPCMODE
 #include <stdio.h>    /* printf */
#endif

/*#define TEST_SET_VIDEO_RAM*/ /* Define to fill physical video ram with a line pattern before clear */
                               /* Ease screen offset adjustment settings GHW_XOFFSET GHW_YOFFSET */

/********************* Chip access definitions *********************/
#ifndef GHW_NOHDW
   #ifdef GHW_SINGLE_CHIP
      /* Use access function interface in bussim.h bussim.c */
      #include <bussim.h>
      #define  sgwrby(a,d) simwrby((a),(d))
      #define  sgrdby(a)   simrdby((a))
   #else
      /* Portable I/O functions + hardware port def */
      #include <sgio.h>
   #endif
#else
   #undef GHW_SINGLE_CHIP /* Ignore single chip mode */
#endif

/************* Simulator interface ************************/

#ifdef GHW_PCSIM
/* Simulator functions */
void ghw_init_sim( GXT w, GYT h );
void ghw_exit_sim(void);
void ghw_dispon_sim(void);
void ghw_dispoff_sim(void);
char ghw_update_palette_sim( GPALETTE_GREY *palette );

void ghw_set_xrange_sim( GXT xb, GXT xe);
void ghw_set_yrange_sim( GYT yb, GYT ye);
void ghw_autowr_sim( GHWCOLOR dat);
GHWCOLOR ghw_autord_sim( SGUCHAR inc );
#endif

/***** Display controller commands ****/

/* Controller status bits */
#define GSTA_ON          0x40

#define GCTRL_XADR       0x15  /* Set X adr cmd + xstart + xstop */
#define GCTRL_YADR       0x75  /* Set Y adr cmd + ystart + ystop */

#define GCTRL_CONTRAST   0x81  /* Set reference voltage, next byte = (0-63) */
#define GCTRL_CURRENT    0x84  /* Current cmd | (1-3) */

#define GCTRL_REMAP      0xA0  /* Set remap cmd + setup */
#define GCTRL_DSTART     0xA1  /* Set Initial display line cmd + 0 */
#define GCTRL_DOFFSET    0xA2  /* Set Display offset + (0-127) */
#define GCTRL_DMODE      0xA4  /* Set Display mode | (0-7) */

#define GCTRL_MUX_RATIO  0xA8  /* Set multiplexer ratio cmd + (16-80) */

#define GCTRL_OFF        0xAE  /* Display off cmd */
#define GCTRL_ON         0xAF  /* Display on cmd */

#define GCTRL_PHASE_LEN  0xB1  /* Set phase length */
#define GCTRL_ROW_PERIOD 0xB2  /* Set Display clock ration + 1 byte */
#define GCTRL_CLK_DIVIDE 0xB3  /* Set Display clock ration + 1 byte */

#define GCTRL_VCOMH      0xBE  /* Set segment high + (0-31) */

#define GCTRL_DEF_GREY_SCALE 0xB7  /* Set default grey scale */
#define GCTRL_GREY_SCALE 0xB8      /* Set grey scale + 8 bytes */

#if defined( GHW_SSD0323 ) || defined( GHW_SSD1328)
#define GCTRL_NOP        0xE3  /* NOP instruction (serial terminate) */

/* Controller commands ( 1 byte ) */

#define GCTRL_DC_DC      0xAD  /* Set dc converter + 3 (on) or + 2 (off) */

#define GCTRL_VSL        0xBF  /* Set segment low (Vls) + (8-14) */
#define GCTRL_NOP        0xE3  /* NOP instruction (serial terminate) */

#ifdef GHW_SSD1328
 #define GCTRL_TWOSECTIONS   0xAA  /* Set optimize brightness */
 #define GCTRL_ROWRIGHTNESS  0xB0  /* Set optimize brightness */
 #define GCTRL_PRECHARGE     0xBB  /* Set precharge cmd + level */
#else
 #define GCTRL_PRECHARGE     0xBC  /* Set precharge cmd + level */
#endif

#elif defined (GHW_SSD1329)
/* SSD1329, SSD1327,  SSD1326 */

/* Controller commands ( 1 byte ) */
//#define GCTRL_PRE_SPEED  0x82  /* Set precharge speed */

//#define GCTRL_ICON       0x90  /* Icon Control cmd + 0 reset icon */
//#define GCTRL_ICON_CUR   0x91  /* Icon current */
//#define GCTRL_ICON_CURSET 0x92 /* Set individual icon current */
//#define GCTRL_ICON_ONOFF 0x93  /* Individual Icon On off (+ mode | number) */
//#define GCTRL_ICON_ALL   0x94  /* All Icon On off (+ mode | number) */
//#define GCTRL_ICON_BLINK 0x95  /* Set icon blinkin duty cycle and frequency */
//#define GCTRL_ICON_AC    0x96  /* Set icon ac drive duty cycle */

//#define GCTRL_PRECHARGE2 0xBB  /* Set second precharge cmd + level */
//#define GCTRL_PRECHARGE1 0xBC  /* Set first precharge cmd + level */
//#define GCTRL_VCOMH      0xBE  /* Set output high level for com signal + 1 byte */
#define GCTRL_PROTECTION 0xFD  /* Command / data change protection + 0x12 = 0n, 0x16=off*/

#else
  #error  Missing or illegal display controller selection switch for this module.
#endif
/***** Check and clean up some definitions ****/

#ifndef GHW_XOFFSET
  #define GHW_XOFFSET 0
#endif
#ifndef GHW_YOFFSET
  #define GHW_YOFFSET 0
#endif


#if (GDISPPIXW != 4)
  #error Illegal GDISPPIXW setting for this display controller
#endif

/* Configuration bits. Ease implementation below */
#ifdef GBASIC_INIT_ERR

/* Let physical nipple swap follow physical horizontal remap */
#ifdef GHW_MIRROR_HOR
   #ifdef GHW_MIRROR_ML
      #define  MX_BIT 0x03
   #else
      #define  MX_BIT 0x01
   #endif
#else
   #ifdef GHW_MIRROR_ML
      #define  MX_BIT 0x00
   #else
      #define  MX_BIT 0x02
   #endif
#endif

#ifdef GHW_USING_VBYTE
   #define AUTOINC_BIT 0x04    /* Display controller use page of vertical bytes */
#else
   #define AUTOINC_BIT 0x00
#endif

#ifdef GHW_MIRROR_VER
   #define  MY_BIT 0x00
#else
   #define  MY_BIT 0x10
#endif

#ifdef GHW_COMSPLIT
   #define  COM_SPLIT_BIT 0x40
#else
   #define  COM_SPLIT_BIT 0x00
#endif

/*
   Initialization table.
   (Only changes from hardware reset default is needed)
*/
static GCODE SGUCHAR FCODE initdata[] =
   {
   #ifdef GHW_SSD1329
   /* Changes from reset default */
   GCTRL_OFF,         /* Blank display */

   GCTRL_DSTART,      /* Set screen orientation */
   0x00,
   GCTRL_DOFFSET,
   0x00,
   #ifdef GHW_INVERSE_DISP
   GCTRL_DMODE | 0x07,  /* | (4=normal,5=on,6=off,7=inverse */
   #else
   GCTRL_DMODE | 0x04,  /* | (4=normal,5=on,6=off,7=inverse */
   #endif

   GCTRL_REMAP,        /* Define display scan order, storage unit pixel order and increment orientations */
   COM_SPLIT_BIT|MY_BIT|AUTOINC_BIT|MX_BIT,

   //GCTRL_DEF_GREY_SCALE, /* Default grey scale (comment out for b&w) */

   GCTRL_CONTRAST,
   0xc0,                 /* Set contrast level (1-255) (is overwritten later) */

   #else
   /* SSD0323 and GHW_SSD1328 */
   GCTRL_OFF,         /* Blank display */

   GCTRL_DSTART,      /* Set screen orientation */
   0x00,
   #ifdef GHW_INVERSE_DISP
   GCTRL_DMODE | 0x07,  /* | (4=normal,5=on,6=off,7=inverse */
   #else
   GCTRL_DMODE | 0x04,  /* | (4=normal,5=on,6=off,7=inverse */
   #endif

   GCTRL_REMAP,        /* Define display scan order, storage unit pixel order and increment orientations */
   COM_SPLIT_BIT|MY_BIT|AUTOINC_BIT|MX_BIT,

   GCTRL_DEF_GREY_SCALE, /* Default grey scale (comment out for b&w) */
 
   /* Set contrast and brightnes */
   GCTRL_CURRENT |  0x02,     /* | (1,2,3) */
   GCTRL_CONTRAST,  64,       /* Set contrast level (1-127) */
   GCTRL_PHASE_LEN, 0x22,     /* [7:4] 1-16, [3:0] 1-16 */
   GCTRL_ROW_PERIOD,59,       /* 2-187 */
   GCTRL_CLK_DIVIDE,0x21,     /* [7:4] 1-16, [3:0] 1-16 */
   GCTRL_PRECHARGE, 0x1B,     /* [7:0] Precharge */
   GCTRL_VSL,       0x0B,     /* [3:0] VSL */
   GCTRL_VCOMH,     0x0B,     /* [7:0] VCOMH */
   #ifdef GHW_SSD0323
   GCTRL_DC_DC,     0x02,     /* 2 = DCoff (external supply), 3 = DCon */
   #endif
   #ifdef GHW_SSD1328
   GCTRL_TWOSECTIONS, 0xf0,   /* Turn off two section display (POR) */
   GCTRL_ROWRIGHTNESS,0x00,   /* Row brightness optimize (0 = Normal, 0x10 Enable) */
   #endif
   #endif

 //  GCTRL_CONTRAST,
 //  0xc0                 /* Set contrast level (1-255) */
   };

/* Default grey scale palette
   The palette file can be edited directly with the ColorIconEdit program
*/
static GCODE GPALETTE_GREY FCODE ghw_palette[] =
    #include <ggray_4.pal>
    ;


/****************************************************************/
/** Low level interface functions used only by ghw_xxx modules **/
/****************************************************************/

/*
   Send a command to SSD0323
   Wait for controller + data ready

   set ghw_err = 0 if Ok
   set ghw_err = 1 if Timeout error

   Internal ghw function
*/
static void ghw_cmd( SGUCHAR cmd )
   {
   #ifndef GHW_NOHDW
   sgwrby(GHWCMD, cmd);
   #else
   cmd++; /* Silence warning */
   #endif
   }

void ghw_set_xrange(GXT xb, GXT xe)
   {
   #ifdef GHW_PCSIM
   ghw_set_xrange_sim( xb, xe);
   #endif
   #ifndef GHW_NOHDW

   /* conversion from logical coordinates to physical page coordinates */
   #ifndef GHW_USING_VBYTE
   xb /= GHWPIXPSU;
   xe /= GHWPIXPSU;
   #endif /*GHW_USING_VBYTE*/

   #ifdef GHW_ROTATED
   sgwrby(GHWCMD,GCTRL_YADR);
   sgwrby(GHWCMD,xb+GHW_YOFFSET);  /* Physical x start */
   sgwrby(GHWCMD,xe+GHW_YOFFSET);
   #else
   sgwrby(GHWCMD,GCTRL_XADR);
   sgwrby(GHWCMD,xb+GHW_XOFFSET);  /* Set X min koordinate */
   sgwrby(GHWCMD,xe+GHW_XOFFSET);  /* Set X max koordinate */
   #endif
   #endif
   }

void ghw_set_yrange(GYT yb, GYT ye)
   {
   #ifdef GHW_PCSIM
   ghw_set_yrange_sim( yb, ye);
   #endif
   #ifndef GHW_NOHDW
   /* conversion from logical coordinates to physical page coordinates */
   #ifdef GHW_USING_VBYTE
   yb /= GHWPIXPSU;
   ye /= GHWPIXPSU;
   #endif /*GHW_USING_VBYTE*/
   #ifdef GHW_ROTATED
   sgwrby(GHWCMD,GCTRL_XADR);
   sgwrby(GHWCMD,yb+GHW_XOFFSET);  /* Physical y start */
   sgwrby(GHWCMD,ye+GHW_XOFFSET);
   #else
   sgwrby(GHWCMD,GCTRL_YADR);
   sgwrby(GHWCMD,yb+GHW_YOFFSET);  /* Physical x start */
   sgwrby(GHWCMD,ye+GHW_YOFFSET);
   #endif
   #endif
   }

/*
   Write databyte to controller (at current position) and increment
   internal xadr. Wait for controller + data ready

   set ghw_err = 0 if Ok
   set ghw_err = 1 if Timeout error

   Internal ghw function
*/
void ghw_auto_wr(GHWCOLOR dat)
   {
   #ifndef GHW_NOHDW
   sgwrby(GHWWR,dat);  /* Write byte directly */
   #endif /* GHW_NOHDW */
   #ifdef GHW_PCSIM
   ghw_autowr_sim( dat );
   #endif

   }

#ifndef GBUFFER

/* Perform required dummy reads after column position setting */
void ghw_auto_rd_start(void)
   {
   #ifndef GHW_NOHDW
   volatile SGUCHAR ret = 0;   /* Dummy var to assure that code for sgrdby(..) is not optimized away */
   ret = sgrdby(GHWRD);        /* Dummy read needed after internal pointer setting */
   ret++;                      /* Operation on dummy var to assure sgrdby() operation is performed */
   #endif
   }

/*
   Read databyte from controller at current pointer and
   inclement pointer
   Wait for controller + data ready

   set ghw_err = 0 if Ok
   set ghw_err = 1 if Timeout error

   Internal ghw function
*/
GHWCOLOR ghw_auto_rd(void)
   {
   #ifndef GHW_NOHDW
   return sgrdby(GHWRD);
   #else

    #ifdef GHW_PCSIM
    return ghw_autord_sim( 1 );
    #else
    return 0;
    #endif

   #endif /* GHW_NOHDW */
   }

/*
   Read byte at line X postion without changing position so
   next operation can be a write (with optional wrap within window range)
   xb is the page position to be preserved.
   The xb position is always within the current window range.
*/

GHWCOLOR ghw_rd_x(GXT xb)
   {
   #ifndef GHW_NOHDW
   GHWCOLOR ret;
   ghw_auto_rd_start();   /* Prepare for read (at current position) */
   ret = ghw_auto_rd();   /* Read data */
   ghw_set_xrange(xb, GDISPW-1);  /* Restore position for use by next write or auto read */
   return ret;
   #else
   #ifdef GHW_PCSIM
   return ghw_autord_sim( 0 );
   #else
   return 0;
   #endif
   #endif
   }

#endif  /* GBUFFER */


/*****************************************************************/
/**         Initialization and error handling functions         **/
/*****************************************************************/

/*
   Copy operative palette to controller so it become visually active
*/
void ghw_activate_palette(void)
   {
   ghw_palette_opr[0].gr = 0; /* Will always be 0 in hardware */

   /* Swap palette table to hardware while converting to SSD0323 format */
   #ifndef GHW_NOHDW
   {
   SGUINT start_index;

   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsset(); /* Activate chip select in advance (if needed)*/
   #endif

   ghw_cmd(GCTRL_GREY_SCALE);
   #if (defined( GHW_SSD0323 ) || defined( GHW_SSD1328 ) )
   /* SSD0323, SSD1325, SSD1328 use 15 levels defined in 1 + 7 bytes */
   ghw_cmd(ghw_palette_opr[1].gr >> 5);
   for (start_index = 0; start_index < sizeof(ghw_palette_opr)/sizeof(ghw_palette_opr[0]); start_index+=2 )
      {
      ghw_cmd(((ghw_palette_opr[start_index+1].gr >> 1) & 0x70) | ((ghw_palette_opr[start_index].gr >> 5) & 0x7));
      }
   #else
   /* SSD1329, SSD1327, SSD1326 use 15 grey levels over 6 bits */
   for (start_index = 1; start_index < sizeof(ghw_palette_opr)/sizeof(ghw_palette_opr[0]); start_index++ )
      {
      ghw_cmd(ghw_palette_opr[start_index].gr >> 2);
      }
   #endif

   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsclr(); /* Deactivate chip select after flush (if needed) */
   #endif
   }
   #endif

   #ifdef GHW_PCSIM
   glcd_err |= ghw_update_palette_sim(ghw_palette_opr);
   #endif
   }


/*
   Load a new palette to the controller or update the existing palette
*/
SGBOOL ghw_palette_grey_wr(SGUINT start_index, SGUINT num_elements, GCONSTP GPALETTE_GREY PFCODE *palette)
   {
   if ((num_elements == 0) ||
       ((start_index + num_elements) > sizeof(ghw_palette_opr)/sizeof(ghw_palette_opr[0])) ||
       (palette == NULL))
      {
      glcd_err = 1;
      return 1;
      }
   glcd_err = 0;

   /* (Partial) update of palette values */
   while(num_elements-- > 0)
      {
      ghw_palette_opr[start_index++].gr = palette->gr;
      palette++;
      }

   ghw_activate_palette();
   return glcd_err;
   }

/*
   Initialize display, clear ram  (low-level)
   Clears glcd_err status before init

   Return 0 if no error,
   Return != 0 if some error
*/
void ghw_ctrl_init(void)
   {
   SGUCHAR i;

   ghw_io_init();  /* Perform target system specific initialization */

   /* Output command setup prologue (needed after reset for some variants) */
   #if (defined( GHW_SSD1328 ) || defined( GHW_SSD1329 ))
     /* Start initialization on command */
     #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
     simcsset(); /* Activate chip select in advance (if needed)*/
     #endif
     #ifdef GHW_SSD1328
        ghw_cmd(GCTRL_DC_DC);       /* SSD1328 require this to be first command after reset */
        ghw_cmd(0x02);
     #elif !defined( GHW_SSD0323 )
        /* SSD1329, SSD1327, SSD1326 use command protection */
        ghw_cmd(GCTRL_PROTECTION);  /* Remove any command update locks */
        ghw_cmd(0x12);
     #endif
     #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
     simcsclr(); /* Deactivate chip select */
     #endif
   #endif /* GHW_SSD1328 || GHW_SSD1329 */

   #ifdef GHW_PCSIM
   /* Tell simulator about the visual LCD screen organization before hardware init */
   ghw_init_sim( GDISPW, GDISPH );
   #endif
   /*
     Write grey palette to hardware (and prepare PC simulator colors)
   */
   ghw_palette_grey_wr(0, sizeof(ghw_palette)/sizeof(GPALETTE_GREY), &ghw_palette[0]);

   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsset(); /* Activate chip select in advance (if needed)*/
   #endif

   for (i=0; i < sizeof(initdata)/sizeof(SGUCHAR); i++)
      {
      ghw_cmd(initdata[i]); /* First command is reset system */
      #ifdef IOTESTER_USB
      iot_sync(IOT_SYNC);
      #endif
      }

   /*
      Stimuli test loop for initial oscilloscope test of display interface bus signals
      Uncomment to use the test loop.
      It is recommended to check all display bus signals with each of the I/O access
      statements in the loop one by one.
   */
   /*
   #ifndef GHW_NOHDW
   for(;;)
      {
      SGUCHAR dat;
      sgwrby(GHWCMD,0xff);
      sgwrby(GHWWR,0x00);
      dat = sgrdby(GHWSTA);
      dat = sgrdby(GHWRD);
      }
   #endif
   */

   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsclr(); /* Deactivate chip select after init (if needed) */
   #endif
   }

/*
   Perform actions needed to deallocate display controller ressources (if any)
*/
void ghw_ctrl_exit(void)
   {
   #ifdef GHW_PCSIM
   ghw_exit_sim();/* Release simulator resources */
   #endif
   }

/*
   Turn display off
   (Minimize power consumption)
*/
void ghw_ctrl_dispoff(void)
   {
   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsset(); /* Activate chip select in advance (if needed)*/
   #endif

   ghw_cmd(GCTRL_OFF);     /* Blank display */

   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsclr(); /* Deactivate chip select after flush (if needed) */
   #endif

   #ifdef GHW_PCSIM
   ghw_dispoff_sim();
   #endif
   #ifdef IOTESTER_USB
   iot_sync(0); /* Make sure PC and hardware are in sync, so we get the timing */
   #endif
   }

/*
   Turn display on
*/
void ghw_ctrl_dispon(void)
   {
   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsset(); /* Activate chip select in advance (if needed)*/
   #endif

   ghw_cmd(GCTRL_ON);

   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsclr(); /* Deactivate chip select after flush (if needed) */
   #endif

   #ifdef GHW_PCSIM
   ghw_dispon_sim();
   #endif
   #ifdef IOTESTER_USB
   iot_sync(0); /* Make sure PC and hardware are in sync, so we get the timing */
   #endif
   }

#ifdef GHW_INTERNAL_CONTRAST
/*
   Set contrast (Normalized value range [0 : 99] )
*/
void ghw_ctrl_cont_set(SGUCHAR contrast)
   {
   #if (defined( GHW_SSD0323 ) || defined( GHW_SSD1328 ) )
   /* SSD0323, SSD1325 SSD1328 */
   contrast = (SGUCHAR)((((SGUINT) contrast) *127) / 100); /* Set contrast level (1-127) */
   #else
   /* SSD1329, SSD1327 SSD1326 */
   contrast = (SGUCHAR)((((SGUINT) contrast) *255) / 100); /* Set contrast level (1-255) */
   #endif

   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsset(); /* Activate chip select in advance (if needed)*/
   #endif

   ghw_cmd(GCTRL_CONTRAST);
   ghw_cmd(contrast);

   #if (defined( GHW_SINGLE_CHIP ) && defined( GBUFFER ))
   simcsclr(); /* Deactivate chip select after flush (if needed) */
   #endif
   }
#endif /* GHW_INTERNAL_CONTRAST */

#endif  /* GBASIC_INIT_ERR */




