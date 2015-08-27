/************************** ghwinit.c *****************************

   Low-level driver functions for the ili9320 LCD display controller
   initialization and error handling.


   The following LCD module characteristics MUST be correctly defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is copied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Revision date:    11-04-07
   Revision Purpose: Driver module modified and specialized for ili9320 family
                     (All other low-level driver modules are equal to the S6D0129 lib)
   Revision date:    03-09-07
   Revision Purpose: Minor optimizations. Corrections to 18 & 24 bit-pr-pixel modes
   Revision date:    06-10-07
   Revision Purpose: ILI9221,ILI9222 support added
   Revision date:    15-01-08
   Revision Purpose: Bug in 32 (18) bit bus mode corrected
   Revision date:    23-12-08
   Revision Purpose: ILI9225 support added
   Revision date:    07-05-09
   Revision Purpose: The symbol software palette (data and functions) can
                     be optimized away if not used by defining
                     GHW_PALETTE_SIZE as 0 in gdispcfg.h
   Revision date:    12-06-09
   Revision Purpose: ILI9225 support revised, "Secret" ILITECH timing commands added
   Revision date:    18-11-09
   Revision Purpose: SSD2119 support added
   Revision date:    10-12-09
   Revision Purpose: SSD1298 support added
   Revision date:    04-11-10
   Revision Purpose: ILI9320 ( R61505 and SPFD5408 ) initialization made more simple
   Revision date:    05-11-10
   Revision Purpose: SPFD5420 support added
   Revision date:    11-11-10
   Revision Purpose: ghw_rgb_to_color(..) updated to use G_RGB_TO_COLOR(r,g,b) macro.
   Revision date:    11-11-10
   Revision Purpose: ghw_rgb_to_color(..) updated to use G_RGB_TO_COLOR(r,g,b) macro.
   Revision date:    28-03-11
   Revision Purpose: Separate GHW_R61505 and GHW_ILI8331 switch added,
                     Software RGB,BGR color swapping made controller specific via GHW_USE_SWRD_COLORSWAP
   Revision date:    15-05-11
                     GHW_ILI9325 switch now adapted for new ILI9325C where color swapping during read is
                     done in hardware (condition for GHW_USE_SWRD_COLORSWAP).
                     GHW_INV_VDATA configuration now possible for ILI9325
   Revision date:    27-07-11
                     Support for OTM2201A
   Revision date:    27-12-11
                     Support for R61580

   Version number: 1.16
   Copyright (c) RAMTEX Engineering Aps 2008-2011

*********************************************************************/

#ifdef SGPCMODE
#include <windows.h> /* Sleep(..) function used by ghw_cmd_wait(..)*/
#else
#include "hal_timer.h"
#endif
#include <s6d0129.h>  /* controller specific definements */

/*#define WR_RD_TEST*/ /* Define to include write-read-back test in ghw_init() */

/* Most controllers in the ILI9320 family only do RGB-BGR swapping during write. Read swapping must be done in software.
   Here exclude software BGR swapping for defined controllers where hardware RGB,BGR swapping is provided for both
   read and write operations. */
#if defined ( GHW_COLOR_SWAP ) && \
    !( defined(GHW_ILI9331) || defined(GHW_ILI9325) || defined(GHW_SSD2119) || \
       defined(GHW_SSD1298) || defined(GHW_OTM2201) || defined(GHW_R61580) )
   #define GHW_USE_SWRD_COLORSWAP
#endif

/* Command registers used by drawing and display handling function */
#define GCTRL_DISP_CTRL       0x07  /*  */
#define GCTRL_DEVICE_CODE     0x00  /* rd only */

#ifdef GHW_SPFD5420
   #define CMDTYPE SGUINT
   #define GCTRL_RAMWR       0x202  /* Set or get GRAM data */
   #define GCTRL_RAMRD       0x202  /* Set or get GRAM data */
#else
   #define CMDTYPE SGUCHAR
   #define GCTRL_RAMWR        0x22  /* Set or get GRAM data */
   #define GCTRL_RAMRD        0x22  /* Set or get GRAM data */
#endif

#ifdef GHW_ILI9225

 #define GCTRL_RAM_ADR_H      0x21  /* Set GRAM address y,x  */
 #define GCTRL_RAM_ADR_L      0x20  /* */

 #define GCTRL_H_WIN_ADR_END  0x36  /* Horizontal range address end */
 #define GCTRL_H_WIN_ADR_STRT 0x37  /* Horizontal range address begin */
 #define GCTRL_V_WIN_ADR_END  0x38  /* Vertical range address end */
 #define GCTRL_V_WIN_ADR_STRT 0x39  /* Vertical range address begin */

 /* GCTRL_DISP_CTRL */
 #ifdef GHW_INV_VDATA
  #define  GDISP_ON         0x0037
  #define  GDISP_OFF        0x0017
 #else
  #define  GDISP_ON         0x0033
  #define  GDISP_OFF        0x0013
 #endif

#define GSBIT_ON            0x0200  /* in register 0x0001 */


#elif defined( GHW_OTM2201 )

 #ifdef GHW_ROTATED
  #error GHW_ROTATED Rotated mode not supported with GHW_OTM2201A
  /* Reason: When using verical cursor increment (AM=1 = cursor movement for rotated screen line mode)
     the internal cursor wraps ok, however data is not written to video ram when internal cursor
     is >= position 176 on line. */
  #endif

 #define GCTRL_RAM_ADR_H      0x21  /* Set GRAM address y,x  */
 #define GCTRL_RAM_ADR_L      0x20  /* */

 #define GCTRL_H_WIN_ADR_END  0x36  /* Horizontal range address end */
 #define GCTRL_H_WIN_ADR_STRT 0x37  /* Horizontal range address begin */
 #define GCTRL_V_WIN_ADR_END  0x38  /* Vertical range address end */
 #define GCTRL_V_WIN_ADR_STRT 0x39  /* Vertical range address begin */

 /* GCTRL_DISP_CTRL */
 #ifdef GHW_INV_VDATA
  #define  GDISP_ON         0x0017
  #define  GDISP_OFF        0x0004
 #else
  #define  GDISP_ON         0x0013
  #define  GDISP_OFF        0x0000
 #endif

#define GSBIT_ON            0x0200  /* in register 0x0001 */

#elif defined( GHW_ILI9222 )

 #define GCTRL_RAM_ADR        0x21  /* Set GRAM address y,x */
 #define GCTRL_H_WIN_ADR      0x44  /* Horizontal range address end,begin */
 #define GCTRL_V_WIN_ADR      0x45  /* Vertical range address end,begin */

 /* GCTRL_DISP_CTRL */
 #ifdef GHW_INV_VDATA
  #define  GDISP_ON         0x0037
  #define  GDISP_OFF        0x0017
 #else
  #define  GDISP_ON         0x0033
  #define  GDISP_OFF        0x0013
 #endif

 #define  GSBIT_ON          0x0200 /* in register 0x0001 */

#elif defined( GHW_ILI9320 ) || defined( GHW_ILI9325 ) || defined( GHW_ILI9331 ) || defined( GHW_R61505 ) || defined( GHW_R61580)

 #define GCTRL_RAM_ADR_L      0x20  /* Set GRAM address x */
 #define GCTRL_RAM_ADR_H      0x21  /* Set GRAM address y */
 #define GCTRL_H_WIN_ADR_STRT 0x50  /* begin */
 #define GCTRL_H_WIN_ADR_END  0x51  /* end */
 #define GCTRL_V_WIN_ADR_STRT 0x52  /* begin */
 #define GCTRL_V_WIN_ADR_END  0x53  /* end */

 /* GCTRL_DISP_CTRL */
 #if defined( GHW_ILI9325 )
 #define  GDISP_ON           0x0133
 #define  GDISP_OFF          0x0130
 #elif defined( GHW_R61505 )
 #define  GDISP_ON           0x0100
 #define  GDISP_OFF          0x0000
 #else
 #define  GDISP_ON           0x0133
 #define  GDISP_OFF          0x0113
 #endif
 #define  GSBIT_ON           0x8000  /* in register 0x0061 */

#elif defined( GHW_SSD2119 )

 #define GCTRL_V_WIN_ADR       0x44  /* Vertical range address end,begin */
 #define GCTRL_H_WIN_ADR_STRT  0x45  /* begin */
 #define GCTRL_H_WIN_ADR_END   0x46  /* end */
 #define GCTRL_RAM_ADR_L       0x4E  /* Set GRAM address x */
 #define GCTRL_RAM_ADR_H       0x4F  /* Set GRAM address y */

 /* GCTRL_DISP_CTRL */
 #define  GDISP_ON           0x0033
 #define  GDISP_OFF          0x0030

#elif defined( GHW_SSD1298 )

 #define GCTRL_H_WIN_ADR        0x44  /* Horizontal range address end,begin */
 #define GCTRL_V_WIN_ADR_STRT   0x45  /* begin */
 #define GCTRL_V_WIN_ADR_END    0x46  /* end */
 #define GCTRL_RAM_ADR_L        0x4E  /* Set GRAM address x */
 #define GCTRL_RAM_ADR_H        0x4F  /* Set GRAM address y */

 /* GCTRL_DISP_CTRL */
 #define  GDISP_ON            0x0033
 #define  GDISP_OFF           0x0030

#elif defined( GHW_SPFD5420 )
   #define GCTRL_RAM_ADR_L      0x200  /* Set GRAM address x */
   #define GCTRL_RAM_ADR_H      0x201  /* Set GRAM address y */
   #define GCTRL_H_WIN_ADR_STRT 0x210  /* begin */
   #define GCTRL_H_WIN_ADR_END  0x211  /* end */
   #define GCTRL_V_WIN_ADR_STRT 0x212  /* begin */
   #define GCTRL_V_WIN_ADR_END  0x213  /* end */

   #define  GDISP_ON           0x0173
   #define  GDISP_OFF          0x0010

   #define  GSBIT_ON           0x8000  /* in register 0x0400 */
#else
 #error Unknown controller. Controller and bustype must be selected in gdispcfg.h
#endif

/*
   Define pixel width, height of internal video memory ( only used for the overflow check below)
   Note: If another display controller variant is used the adjust the GCTRLW, GCTRLH definitions
         below accordingly to match the size of the pixel video RAM in the controller.
   Note: If the physical memory range limits are exceeded at runtime then some controllers stop working.
*/
#if (defined( GHW_ILI9222) || defined(GHW_ILI9225))
  /* ILI9221 ILI9222, ILI9225 */
  #define  GCTRLW 176
  #define  GCTRLH 220
#elif defined( GHW_SSD2119 )
  #define  GCTRLW 320
  #define  GCTRLH 240
#elif defined( GHW_SSD1298 )
  #define  GCTRLW 240
  #define  GCTRLH 320
#elif defined( GHW_SPFD5420 )
  #define  GCTRLW 240
  #define  GCTRLH 432
#else
  /* ILI9320 , ILI9325 (240x320) */
  #define  GCTRLW 240
  #define  GCTRLH 320
#endif

/* Check display size settings */
#ifdef GHW_ROTATED
  #if (((GDISPH+GHW_YOFFSET) > GCTRLW) || ((GDISPW+GHW_YOFFSET) > GCTRLH))
//    #error (GDISPW, GDISPH, GHW_XOFFSET, GHW_YOFFSET, GHW_ROTATED configuration exceeds controller memory limits)
  #endif
#else
  #if (((GDISPW+GHW_XOFFSET) > GCTRLW) || ((GDISPH+GHW_YOFFSET) > GCTRLH))
//    #error (GDISPW, GDISPH, GHW_XOFFSET, GHW_YOFFSET, GHW_ROTATED configuration exceeds controller memory limits)
  #endif
#endif


/********************* Chip access definitions *********************/

#ifndef GHW_NOHDW
   #if defined( GHW_SINGLE_CHIP)
      /* User defined access types and simulated register address def */
      #include <bussim.h>
      #ifdef GHW_BUS8
        #define  sgwrby(a,d) simwrby((a),(d))
        #define  sgrdby(a)   simrdby((a))
      #elif defined (GHW_BUS32)
        #define  sgwrdw(a,d) simwrdw((a),(d))
        #define  sgrddw(a)   simrddw((a))
      #else
        #define  sgwrwo(a,d) simwrwo((a),(d))
        #define  sgrdwo(a)   simrdwo((a))
      #endif
   #else
      /* Portable I/O functions + hardware port def */
      #include <sgio.h>
   #endif
#else
   #undef GHW_SINGLE_CHIP /* Ignore single chip mode */
#endif

/* Fix missing definitions in gdispcfg.h */
#ifndef GHW_XOFFSET
   #define GHW_XOFFSET 0
#endif
#ifndef GHW_YOFFSET
   #define GHW_YOFFSET 0
#endif

/***********************************************************************/
/** All static LCD driver data is located here in this ghwinit module **/
/***********************************************************************/

#ifdef GBASIC_INIT_ERR

/* Active foreground and background color */
GCOLOR ghw_def_foreground;
GCOLOR ghw_def_background;

#if (GHW_PALETTE_SIZE > 0)
/* Default palette
   The palette file can be edited directly with the ColorIconEdit program
*/
static GCODE GPALETTE_RGB FCODE ghw_palette[16] =
     #include <gcolor_4.pal>
     ;

/* Operative palette (current palette used for color lookup) */
GCOLOR ghw_palette_opr[16];
#endif

/* Use software font */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR  b[8];           /* Symbol data, fixed size = 8 bytes */
   }
GCODE FCODE sysfontsym[0x80] =
   {
   /* The default font MUST be a monospaced black & white (two-color) font */
   #include <sfs0129.sym> /* System font symbol table */
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

#ifdef GBUFFER
   #ifdef GHW_ALLOCATE_BUF
      /* <stdlib.h> is included via gdisphw.h */
      GCOLOR *gbuf = NULL;           /* Graphic buffer pointer */
      static SGBOOL gbuf_owner = 0;  /* Identify pointer ownership */
   #else
      GCOLOR gbuf[GBUFSIZE];         /* Graphic buffer */
   #endif
   GXT GFAST iltx,irbx;     /* "Dirty area" speed optimizers in buffered mode */
   GYT GFAST ilty,irby;
   SGBOOL  ghw_upddelay;    /* Flag for delayed update */
#else
   GCOLOR ghw_tmpbuf[GDISPW]; /* Row line buffer (for block read-modify-write) */
#endif /* GBUFFER */

#ifdef GHW_INTERNAL_CONTRAST
static SGUCHAR ghw_contrast;/* Current contrast value */
#endif

SGBOOL glcd_err;            /* Internal error */
#ifndef GNOCURSOR
GCURSOR ghw_cursor;         /* Current cursor state */
#endif

#ifdef GHW_PCSIM
/* PC simulator declaration */
void ghw_init_sim( SGUINT dispw, SGUINT disph );
void ghw_exit_sim(void);
void ghw_set_xyrange_sim(GXT xb, GYT yb, GXT xe, GYT ye);
void ghw_set_xy_sim(GXT xb, GYT yb);
void ghw_autowr_sim( GCOLOR cval );
GCOLOR ghw_autord_sim( void );
void ghw_dispon_sim( void );
void ghw_dispoff_sim( void );
#endif
/****************************************************************/
/** Low level interface functions used only by ghw_xxx modules **/
/****************************************************************/

/* Bit mask values */
GCODE SGUCHAR FCODE sympixmsk[8] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};

#if defined( GHW_SSD2119 ) || defined( GHW_SSD1298 )
   #if   ( defined( GHW_MIRROR_VER ) &&  defined( GHW_MIRROR_HOR ))
      #define TB_BIT  0x0200
      #define REV_BIT 0x4000
   #elif ( defined( GHW_MIRROR_VER ) && !defined( GHW_MIRROR_HOR ))
      #define TB_BIT  0x0200
      #define REV_BIT 0x0000
   #elif (!defined( GHW_MIRROR_VER ) &&  defined( GHW_MIRROR_HOR ))
      #define TB_BIT  0x0000
      #define REV_BIT 0x4000
   #else
      #define TB_BIT  0x0000
      #define REV_BIT 0x0000
   #endif

   #ifdef GHW_COLOR_SWAP
      #define  RGB_BIT 0x0800
   #else
      #define  RGB_BIT 0x0000
   #endif
#else
   /* Mirror and rotation definition bits (to simplify initialization below) */
   #if   ( defined( GHW_MIRROR_VER ) &&  defined( GHW_MIRROR_HOR ))
      #define GS_BIT GSBIT_ON
      #define SS_BIT 0x0100
   #elif ( defined( GHW_MIRROR_VER ) && !defined( GHW_MIRROR_HOR ))
      #define GS_BIT GSBIT_ON
      #define SS_BIT 0x0000
   #elif (!defined( GHW_MIRROR_VER ) &&  defined( GHW_MIRROR_HOR ))
      #define GS_BIT 0x0000
      #define SS_BIT 0x0100
   #else
      #define GS_BIT 0x0000
      #define SS_BIT 0x0000
   #endif

   #ifdef GHW_COLOR_SWAP
      /* Entry mode bit 12 */
      #define  RGB_BIT 0x1000
   #else
      #define  RGB_BIT 0x0000
   #endif

 /* Define bus / data formats */
 #if   ((GDISPPIXW == 18) && defined( GHW_BUS8 ))
  /* 6+6+6 */
  #define  TRI_DFM 0xc000
 #elif ((GDISPPIXW == 24) && defined( GHW_BUS8 ))
  /* 6+6+6 + format conversion  */
  #define  TRI_DFM 0xC000
 #elif ((GDISPPIXW == 18) && defined( GHW_BUS16 ))
  /* 16+2, 18 bit color */
  #define  TRI_DFM 0x8000
 #elif ((GDISPPIXW == 24) && defined( GHW_BUS16 ))
  /* 16+2 bit, split to 24 bit rgb*/
  #define  TRI_DFM 0x8000
 #else
  #define  TRI_DFM 0x0000
 #endif

#endif

#ifdef GHW_ROTATED
   #define AM_BIT 0x0038
   /* Note: Register size setting uses same values independent of rotation */
   #define DISPW  GDISPH
   #define DISPH  GDISPW
#else
   #define AM_BIT 0x0030
   #define DISPW  GDISPW
   #define DISPH  GDISPH
#endif

typedef struct
   {
   CMDTYPE index;
   SGUCHAR delay;
   SGUINT value;
   } S1D_REGS;

/* Array of configuration descriptors, the registers are initialized in the order given in the table */
static GCODE S1D_REGS FCODE as1dregs[] =
   {
   #if defined( GHW_R61580 )
     {0x0000,  0, 0x0000},             /* Exit deep sleep */
     {0x0000,  0, 0x0000},             /* Exit deep sleep */
     {0x0000,  2, 0x0000},             /* Exit deep sleep */
     {0x0000,  0, 0x0000},             /* Exit deep sleep */
     {0x0000,  0, 0x0000},             /* Exit deep sleep */
     {0x0000,  0, 0x0000},             /* Exit deep sleep */
     {0x0000,  2, 0x0000},             /* Exit deep sleep */
     {0x00A4,  2, 0x0001},             /* Restore NVM settings to register */

     {0x0060,  0, GS_BIT|0x2700},      /* Gate Scan Line (320 lines) */

     /* Gamma */
//     {0x0030,  0, 0x0000},
//     {0x0031,  0, 0x0000},
//     {0x0032,  0, 0x0000},
//     {0x0033,  0, 0x0000},
//     {0x0034,  0, 0x0000},
//     {0x0035,  0, 0x0000},
     {0x0036,  0, 0x001f},
//     {0x0037,  0, 0x0000},
//     {0x0038,  0, 0x0000},
//     {0x0039,  0, 0x0000},

     {0x0001,  0, SS_BIT},
     {0x0002,  0, 0x0200},              /* set 1 line inversion */
     {0x0003,  0, TRI_DFM | RGB_BIT | AM_BIT}, /* Set busmode, GRAM write direction and BGR.*/
     {0x0060,  0, GS_BIT|0x2700},      /* Gate Scan Line (320 lines) */
     #ifdef GHW_INV_VDATA
     {0x0061,  0, 0x0001},             /* NDL,VLE, REV*/
     #else
     {0x0061,  0, 0x0000},             /* NDL,VLE, !REV*/
     #endif

     /* Power up */
     {0x0012, 50, 0x0000},             /* Off */
     {0x0013,200, 0x0000},             /* VDV[4:0] for VCOM amplitude, delay 200 to Dis-charge capacitor power voltage */
     {0x0010,  0, 0x0530},             /* SAP=0, BT[3:0], AP, DSTB, SLP, STB */
     {0x0011, 50, 0x0337},             /* DC1[2:0], DC0[2:0], VC[2:0], delayms(50},  0, delay 50ms */
     {0x0012, 50, 0x01Bf},             /* VREG1OUT voltage, Delay 50ms */
     {0x0029, 50, 0x000E},             /* VCM[4:0] for VCOMH, delay 50 ms */

   #elif GHW_OTM2201
     {0x28, 255, 0x00ce},
     {0x11, 0, 0x0018},  /* Power Control 2 Booster circuits not started automatically, controlled through PON0-3. Generate VCI1, VCI1 = +2.58v */
     {0x12, 0, 0x0000},  /* Power Control 3 Generate VGH VCI1 x 5 (13.75v) / VGL VCI1 x -3 (-8.25v). Freq. of step up 1 = 1:4, Freq. of step up 2 = 1:2, Freq. of step up 3 = 1:4 */
     {0x13, 0, 0x0063},  /* Power Control 4 Clock cycle of external (RGB) interface (as default, unused). Gamma voltage (GVDD < AVDD (VCI1*2)-0.3v) 1100011 = GVDD Voltage +4.45v */
     {0x14, 0, 0x556A},  /* Power Control 5 VCOMG = 0 ( Amplitude of VCOM = |VCOMH-VCOML| ), VCOMH = GVDD x 0.8690. VCMR = 0 (VCOMH determined by VCM6-0), VCOMH = GVDD x 1.074 */
     {0x10,50, 0x0800},  /* Power Control 1 Constant current in op-amp Medium Fast 1. Not in stand by, not in deep stand by */
     {0x11,50, 0x0118},  /* Power Control 2 Start booster circuit 1. Generate VCI1, VCI1 = +2.58v */
     {0x11,50, 0x0318},  /* Power Control 2 Booster circuit 1 on, start VGH circuit. Generate VCI1, VCI1 = +2.58v */
     {0x11,50, 0x0718},  /* Power Control 2 Booster circuit 1, VGH on, start VGL circuit Generate VCI1, VCI1 = +2.58v */
     {0x11,50, 0x0F18},  /* Power Control 2 Booster circuit 1, VGH, VGL on, Start VCL circuit Generate VCI1, VCI1 = +2.58v */
     {0x11,50, 0x0F38},  /* Power Control 2 Booster circuit 1, VGH, VGL on, Start VCL circuit. Start amplifier circuit, Generate VCI1, VCI1 = +2.58v */
     {0x07, 0, 0x0012},  /* Display Control FLM output disabled Gate output Enabled, Normally black */
     {0x07, 0, 0x001A},  /* Display Control FLM output disabled Gate output Enabled, 8 color mode selected, Normally black */
     {0x01, 0, GS_BIT|SS_BIT|((DISPH+7)/8)}, /* Scan direction, and scan size */
     {0x03, 0, RGB_BIT|AM_BIT}, /* set busmode, GRAM write direction and BGR. */
     {0x07, 0, 0x0000},  /* Display Control, FLM output disabled Gate output disabled */
     {0x08, 0, 0x0808},  /* Blanking Control, 8 lines for the front porch 8 lines for the back porch */
     {0x15, 0, 0x0020},  /* VCI Period,Sn=2, Vcom1=1/2, Vcom2=2/1, RGB=16dot clock */

     {0x50, 0, 0x0001},  /* Gamma Control */
     {0x51, 0, 0x0208},  /* Gamma Control */
     {0x52, 0, 0x0805},  /* Gamma Control */
     {0x53, 0, 0x0404},  /* Gamma Control */
     {0x54, 0, 0x0c0c},  /* Gamma Control */
     {0x55, 0, 0x000c},  /* Gamma Control */
     {0x56, 0, 0x0100},  /* Gamma Control */
     {0x57, 0, 0x0400},  /* Gamma Control */
     {0x58, 0, 0x1108},  /* Gamma Control */
     {0x59, 0, 0x050c},  /* Gamma Control */

     {0x0F, 0, 0x0F01},  /* Oscillator Control, Freq = 453kHz x 1.29, Oscillator on */

   #elif defined( GHW_ILI9325 )
     {0xe3, 0 ,0x3008},
     {0xe7, 0 ,0x0012},
     {0xef, 0 ,0x1231},

     /* Standard configuration. Some values are equal to reset defaults */
     {0x01, 0, SS_BIT},
     {0x02, 0, 0x0700},            /* set 1 line inversion */
     {0x03, 0, TRI_DFM|RGB_BIT|AM_BIT}, /* set busmode, GRAM write direction and BGR. */
     {0x04, 0,0x0000},             /*  No resizing */
     {0x08, 0 ,(0x2<<8)|0x2},      /*  Set the front porch (0-0xf) and back porch (0-0xf) */
     {0x09, 0,0x0000},             /*  Disable use of non-display area, normal grey scale levels (reset default) */
     {0x0A, 0,0x0000},             /*  1 frame mark (reset default) */
     {0x0C, 0,0x0000},             /*  RGB interface off (reset default) */
     {0x0D, 0,0x0000},             /*  Frame marker 0 (reset default) */
     {0x0F, 0,0x0002},             /*  RGB interface config (not used, reset default) */

     /*** Power On sequence ***/
     {0x10, 0,0x0000},             /*  Exit sleep or standby modes */
     {0x11, 0,0x0007},             /*  Voltage ratio x1, Oscillator divide x1 (reset default) */
     {0x12, 0,0x0000},             /*  Halt vout generation */
     {0x13, 200,0x0000},           /*  Reset VCOM amplitude, discharge capacitor power voltage */

     {0x10, 0,0x1690},             /*  Enable TFT drivers, Set voltage and current levels */
     {0x11,50,0x0222},             /*  Vout ration 0.75. Stepup1=osc, Stepup2=osc/16 */

     {0x12,50,0x0087},             /* Power drive on, Internal reference voltage * 2=5V*/

     {0x13, 0,0x1F00},             /* Set VDV[4:0] for VCOM amplitude = x1.08 */
     {0x29, 0,0x0035},             /* Set VcomH voltage (0-0x1f) VREG1OUTx0.895 */

     {0x2B,50,0x000D},             /* Set frame rate */

     /*** adjust gamma curve ***/
     {0x30, 0,0x0000},
     {0x31, 0,0x0003},
     {0x32, 0,0x0400},
     {0x35, 0,0x0001},
     {0x36, 0,0x1C00},
     {0x37, 0,0x0703},
     {0x38, 0,0x0407},
     {0x39, 0,0x0707},
     {0x3C, 0,0x0100},
     {0x3D, 0,0x000F},

     /*** Set GRAM area (default setting here, are modified later) ***/
     {0x20, 0 ,0x0000},            /* set GRAM horizontal address */
     {0x21, 0 ,0x0000},            /* set GRAM vertical address   */

     {0x50, 0 ,0x0000},            /* Horizontal Address Start Position */
     {0x51, 0 ,DISPW-1},           /* Horizontal Address end Position (239) */
     {0x52, 0 ,0x0000},            /* Vertical Address Start Position */
     {0x53, 0 ,DISPH-1},           /* Vertical Address end Position (319) */

     {0x60, 0,GS_BIT|0x2700},      /* Gate Scan Line (320 lines) */
     #ifdef GHW_INV_VDATA
     {0x61, 0,0x0001},             /* Reverse video */
     #else
     {0x61, 0,0x0000},
     #endif
     {0x6A,50,0x0000},             /* set scrolling line */

     /*** Panel control ***/
     {0x90, 0,0x0010},
     {0x92, 0,0x0600},
     {0x93, 0, 0x0003},
     {0x95, 0, 0x0110},
     {0x97, 0, 0x0000},
     {0x98, 0, 0x0000},

   #elif defined( GHW_ILI9225 )
     {0x0028,  0, 0x00CE},             /* Software Reset */
     {0x000f,  0, 0x0b01},             /* OSC ctrl*/
     {0x0000,  0, 0x0001},             /* Start Oscillator */
     {0x0001,  0, GS_BIT|SS_BIT|((DISPH+7)/8)},
     {0x0002,  0, 0x0100},             /* */
     {0x0003,  0, TRI_DFM|RGB_BIT|AM_BIT}, /* set busmode, GRAM write direction and BGR. */
     {0x0007,  0, 0x0000},             /* */
     {0x0008,  0, 0x0202},             /* */
     {0x000b,  0, 0x4405},             /* */
     {0x000c,  0, 0x0003},             /* */
     {0x0061,  0, 0x0000},             /* */
     {0x0010,  0, 0x0700},             /* */
     {0x0011,  0, 0x1037},             /* */
     {0x0012,  0, 0x6212},             /* */
     {0x0013,  0, 0x0066},             /* */
     {0x0014,  0, 0x5d5e},             /* */
     {0x0015,  0, 0x0040},             /* */
     {0x0030,  0, 0x0000},             /* */
     {0x0036,  0, DISPW-1},            /* */
     {0x0037,  0, 0x0000},             /* */
     {0x0038,  0, DISPH-1},            /* */
     {0x0039,  0, 0x0000},             /* */

     {0x0050,  0, 0x0906},             /* */
     {0x0051,  0, 0x030f},             /* */
     {0x0052,  0, 0x0c07},             /* */
     {0x0054,  0, 0x070c},             /* */
     {0x0055,  0, 0x0f03},             /* */
     {0x0056,  0, 0x0609},             /* */
     {0x0053,  0, 0x030c},             /* */
     {0x0057,  0, 0x0c03},             /* */
     {0x0058,  0, 0x0000},             /* */
     {0x0059,  0, 0x0007},             /* */
     {0x0007,  0, 0x1017},             /* GON = 1 */

   #elif defined( GHW_R61505 )  /* R61505 R61505W etc */

     {0x0000 ,0  ,0x0000}, /* Interface sync (serial mode, just in case) */
     {0x0000 ,0  ,0x0000},
     {0x0000 ,0  ,0x0000},
     {0x0000 ,0  ,0x0000},
     {0x00A4 ,1  ,0x0001}, /* Recall factory setup */
     {0x0008 ,0  ,0x2C2C}, /* Front porch, back porch */

     {0x0030 ,0  ,0x0605}, /* Gamma adjustment */
     {0x0031 ,0  ,0x4608},
     {0x0032 ,0  ,0x0705},
     {0x0033 ,0  ,0x0209},
     {0x0034 ,0  ,0x0000},
     {0x0035 ,0  ,0x0902},
     {0x0036 ,0  ,0x4705},
     {0x0037 ,0  ,0x0806},
     {0x0038 ,0  ,0x0506},
     {0x0039 ,0  ,0x0000},

     {0x0060 ,0  ,GS_BIT|((((DISPH-240)/8)+0x1D)<<8)}, /* Number of drive lines (320-240)*/
     {0x0061 ,0  ,0x0000},  /* (= reset default) */
     {0x006A ,0  ,0x0000},  /* (= reset default) */

     {0x0050 ,0  ,0x0000},  /* Window range, (overwritten later) */
     {0x0051 ,0  ,DISPW-1},
     {0x0052 ,0  ,0x0000},
     {0x0053 ,0  ,DISPH-1},

     {0x0080 ,0  ,0x0000},  /* Disable partial fram (= reset default) */
     {0x0081 ,0  ,0x0000},
     {0x0082 ,0  ,0x001F},

     {0x0090 ,0  ,0x0015},  /* Panel control, clocks pr line */
     {0x0092 ,0  ,0x0400},  /* Panel control, 4 clock overlap */
     {0x0093 ,0  ,0x0402},  /* Panel control, 4 clock equ period, sync display with int clock  */

     /*{0x0007 ,0  ,0x0000},*/ /* (= reset default) */
     {0x0010 ,0  ,0x0530}, /* Power ctrl 1 (= reset default) */
     {0x0011 ,0  ,0x0237}, /* Power ctrl 2 (stepup clk = linefreq/4), fosc/8 */
     {0x0012 ,0  ,0x118F}, /* Power ctrl 3 (Set levels, psu off) */
     {0x0013 ,0  ,0x0F00}, /* Power ctrl 3 (VDV = Max) */
     {0x0001 ,0  , SS_BIT},
     {0x0002 ,0  ,0x0200}, /* Line inversion waveform */
     {0x0003 ,0  , TRI_DFM | RGB_BIT | AM_BIT},
     /* {0x0009 ,0  ,0x0001},*/ /* (= reset default) */
     {0x000A ,0  ,0x0008}, /* Enable output FMARK signal (only needed if used by host) */
     /*{0x000D ,0  ,0x0000},*/ /* (= reset default) */
     {0x000E ,150,0x0030}, /* VCOM low power ctrl */
     {0x0012 ,0  ,0x11BF}, /* Power ctrl 3 (Set levels, psu on) */

   #elif (defined( GHW_ILI9320 ) || defined( GHW_ILI9331 ))
      {0x0001,  0, SS_BIT},
      {0x0002,  0,0x0700},              /* set 1 line inversion */
      {0x0003,  0, TRI_DFM | RGB_BIT | AM_BIT}, /* Set busmode, GRAM write direction and BGR.*/
      {0x0004,  0, 0x0000},             /* Resize register */
      {0x0008,  0, 0x0305},             /* set the back porch and front porch */
      {0x0009,  0, 0x0000},             /* set non-display area refresh cycle ISC[3:0] */
      {0x000A,  0, 0x0000},             /* FMARK function */
      {0x000C,  0, 0x0000},             /* RGB interface setting */
      {0x000D,  0, 0x0000},             /* Frame marker Position */
      {0x000F,  0, 0x0000},             /* RGB interface polarity */
      /* **************Power On sequence *****************/
      {0x0010,  0, 0x0000},             /* SAP, BT[3:0], AP, DSTB, SLP, STB */
      {0x0011,  0, 0x0000},             /* DC1[2:0], DC0[2:0], VC[2:0] */
      {0x0012,  0, 0x0000},             /* VREG1OUT voltage */
      {0x0013,200, 0x0000},             /* VDV[4:0] for VCOM amplitude, delay 200 to Dis-charge capacitor power voltage */

      {0x0010,  0, 0x12B0},             /* SAP=0, BT[3:0], AP, DSTB, SLP, STB */
      {0x0011, 50, 0x0007},             /* DC1[2:0], DC0[2:0], VC[2:0], delayms(50},  0, delay 50ms */

      {0x0012, 50, 0x01BD},             /* VREG1OUT voltage, Delay 50ms */

      {0x0013,  0, 0x1400},             /* VDV[4:0] for VCOM amplitude */
      {0x0029, 50, 0x000E},             /* VCM[4:0] for VCOMH, delay 50 ms */

      {0x0020,  0, 0x0000},             /* GRAM horizontal Address */
      {0x0021,  0, 0x0000},             /* GRAM Vertical Address */

      /* Adjust the Gamma Curve */
      {0x0030,  0, 0x0504},
      {0x0031,  0, 0x0703},
      {0x0032,  0, 0x0702},
      {0x0035,  0, 0x0101},
      {0x0036,  0, 0x0A1F},
      {0x0037,  0, 0x0504},
      {0x0038,  0, 0x0003},
      {0x0039,  0, 0x0706},
      {0x003C,  0, 0x0707},
      {0x003D,  0, 0x091F},
      {0x003E,  0, 0x0403},
      {0x003F,  0, 0x0607},

   /* ------------------ Set GRAM area ---------------*/

      {0x0050,  0, 0x0000},             /* Horizontal GRAM Start Address */
      {0x0051,  0, DISPW-1},            /* Horizontal GRAM End Address */
      {0x0052,  0, 0x0000},             /* Vertical GRAM Start Address */
      {0x0053,  0, DISPH-1},            /* Vertical GRAM Start Address */

      {0x0060,  0, GS_BIT|0x2700},      /* Gate Scan Line (320 lines) */
      {0x0061,  0, 0x0001},             /* NDL,VLE, REV*/
      {0x006A,  0, 0x0000},             /* set scrolling line*/

   /* -------------- Partial Display Control ---------*/
      {0x0080,  0, 0x0000},
      {0x0081,  0, 0x0000},
      {0x0082,  0, 0x0000},
      {0x0083,  0, 0x0000},
      {0x0084,  0, 0x0000},
      {0x0085,  0, 0x0000},
   /*-------------- Panel Control -------------------*/
      {0x0090,  0, 0x0010},
      {0x0092,  0, 0x0000},
      {0x0093,  0, 0x0003},
      {0x0095,  0, 0x0110},
      {0x0097,  0, 0x0000},
      {0x0098,  0, 0x0000},

   #elif (defined( GHW_SSD2119 ) || defined( GHW_SSD1298 ))

     /****  SSD2119 *****/
      {GCTRL_DISP_CTRL, 0, GDISP_OFF},  /* Reg 0x0007, turn dispoff during ram clear */
      {0x0000,  0, 0x0001},         /* start Oscillator */
      {0x0010,  0, 0x0000},         /* Sleep mode         */
      #ifdef GHW_SSD1298
      {0x0001,  0, REV_BIT|0x3000|RGB_BIT|TB_BIT|(DISPH & 0x1ff)},  /* Driver Output Control */
      #else
      {0x0001,  0, REV_BIT|0x3000|RGB_BIT|TB_BIT|(DISPH & 0xff)},  /* Driver Output Control */
      #endif
      {0x0002,  0, 0x0600},         /* LCD Driving Waveform Control */
      {0x0003,  0, 0x6A38},         /* Power Control 1              */
      #if (GDISPPIXW >= 18)
      {0x0011,  0, 0x4840|AM_BIT},  /* Entry Mode (256K color, format mode B when 16 bit bus */
      #else
      {0x0011,  0, 0x6840|AM_BIT},  /* Entry Mode (64K color) */
      #endif
      {0x000F,  0, 0x0000},         /* Gate Scan Position           */
      {0x000B,  0, 0x5308},         /* Frame Cycle Control          */
      {0x000C,  0, 0x0003},         /* Power Control 2              */
      {0x000D,  0, 0x000A},         /* Power Control 3              */
      {0x000E,  0, 0x2E00},         /* Power Control 4              */
      {0x001E,  0, 0x00BE},         /* Power Control 5              */
      {0x0025,  0, 0x8000},         /* Frame Frequency Control      */
      {0x0026,  0, 0x7800},         /* Analog setting               */
      {0x0048,  0, 0x0000},         /* Window 1 */
      {0x0049,  0, DISPH-1},
      {0x004A,  0, 0x0000},         /* Window 2 */
      {0x004B,  0, DISPH-1},
      {0x004E,  0, 0x0000},         /* Ram Address Set              */
      {0x004F,  0, 0x0000},         /* Ram Address Set              */
      {0x0012,  0, 0x08D9},         /* Sleep mode                 */
      /* Gamma Control (R30h to R3Bh) */
      {0x0031,  0, 0x0104},
      {0x0032,  0, 0x0100},
      {0x0033,  0, 0x0305},
      {0x0034,  0, 0x0505},
      {0x0035,  0, 0x0305},
      {0x0036,  0, 0x0707},
      {0x0037,  0, 0x0300},
      {0x003A,  0, 0x1200},
      {0x003B,  0, 0x0800},

   #elif defined( GHW_SPFD5420 )

   {0x0606,50, 0x0000},           /* Reset data receive order */
   {GCTRL_DISP_CTRL,50, 0x0001},  /* Display Control 1 Power on, no output*/

   /*--------------- Power control 1-6 --------------- */
   {0x0100, 0, 0x16B0},           /* Power Control 1 */
   {0x0101, 0, 0x0147},           /* Power Control 2 */
   {0x0102, 0, 0x01BD},           /* Power Control 3 */
   {0x0103, 0, 0x2d00},           /* Power Control 4 */
   {0x0107, 0, 0x0000},           /* Power Control 5 */
   {0x0110, 0, 0x0001},           /* Power Control 6 */
   {0x0280, 0, 0x0000},           /* NVM read data 1 */
   {0x0281, 0, 0x0006},           /* NVM read data 2 */
   {0x0282, 0, 0x0000},           /* NVM read data 3 */

   /*------- Gamma 2.2 control  ------ */
   {0x0300, 0, 0x0101},
   {0x0301, 0, 0x0b27},
   {0x0302, 0, 0x132a},
   {0x0303, 0, 0x2a13},
   {0x0304, 0, 0x270b},
   {0x0305, 0, 0x0101},
   {0x0306, 0, 0x1205},
   {0x0307, 0, 0x0512},
   {0x0308, 0, 0x0005},
   {0x0309, 0, 0x0003},
   {0x030A, 0, 0x0f04},
   {0x030B, 0, 0x0f00},
   {0x030C, 0, 0x000f},
   {0x030D, 0, 0x040f},
   {0x030E, 0, 0x0300},
   {0x030F, 0, 0x0500},
   #ifdef GHW_MIRROR_VER
   {0x0400, 0, GS_BIT | ((DISPH/8-1)<<8) | ((GCTRLH-DISPH)/8)},  /* Base Image Number of Line */
   #else
   {0x0400, 0, GS_BIT | ((DISPH/8-1)<<8) },  /* Base Image Number of Line */
   #endif
   {0x0401, 0, 0x0001},           /* Base Image Display Control */

   /*--------------- Normal set --------------- */
   {0x0001, 0, SS_BIT},           /* Driver Output Control Register */
   {0x0002, 0, 0x0100},           /* LCD Driving Waveform Control */
   {0x0003, 0, TRI_DFM|RGB_BIT|AM_BIT}, /* set busmode, GRAM write direction and BGR. */
   {0x0006, 0, 0x0000},           /* Display Control 1 */
   {0x0008, 0, 0x0808},           /* Display Control 2 */
   {0x0009, 0, 0x0001},           /* Display Control 3 */
   {0x000B, 0, 0x0010},           /* Low Power Control */
   {0x000C, 0, 0x0000},           /* External Display Interface Control 1 */
   {0x000F, 0, 0x0000},           /* External Display Interface Control 2 */
   {GCTRL_DISP_CTRL, 0, 0x0001},           /* Display Control 1 */

   /*--------------- Panel interface control 1-6 --------------- */
   {0x0010, 0, 0x0012},           /* Panel Interface Control 1 */
   {0x0011, 0, 0x0202},           /* Panel Interface Control 2 */
   {0x0012, 0, 0x0300},           /* Panel Interface control 3 */
   {0x0020, 0, 0x021E},           /* Panel Interface control 4 */
   {0x0021, 0, 0x0202},           /* Panel Interface Control 5 */
   {0x0022, 0, 0x0100},           /* Panel Interface Control 6 */
   {0x0090, 0, 0x8000},           /* Frame Marker Control */

   /*--------------- Partial display --------------- */
   {0x0500, 0, 0x0000},           /* Display Position - Partial Display 1 */
   {0x0501, 0, 0x0000},           /* RAM Address Start - Partial Display 1 */
   {0x0502, 0, 0x0000},           /* RAM Address End - Partail Display 1 */
   {0x0503, 0, 0x0000},           /* Display Position - Partial Display 2 */
   {0x0504, 0, 0x0000},           /* RAM Address Start – Partial Display 2 */
   {0x0505, 0, 0x0000},           /* RAM Address End – Partial Display 2 */
   {0x0606, 0, 0x0000},           /* Pin Control */
   {0x06F0, 0, 0x0000},           /* NVM Access Control */

   {GCTRL_DISP_CTRL,50, 0x0173},  /* Display Control 1 */
   {GCTRL_DISP_CTRL,10, 0x0171},  /* Display Control 1 */
   {GCTRL_DISP_CTRL, 0, 0x0173},  /* Display Control 1 */

   #else
      /* ILI9222, ILI9221 */
     {0x0000, 50, 0x0001},              /* Start internal OSC. */
     {0x0002,  0, 0x0700},              /* set 1 line inversion */
     {0x0001,150, GS_BIT|SS_BIT|((DISPH+7)/8)},
     {0x0003,  0, TRI_DFM|RGB_BIT|AM_BIT}, /* set busmode, GRAM write direction and BGR. */
     {0x0004,  0, 0x0000},             /* Compare register 1 (not used) */
     {0x0005,  0, 0x0000},             /* Compare register 2 (not used) */
     {0x0008,  0, 0x0202},             /* set the back porch and front porch */
     {0x0009,  0, 0x0001},             /* set non-display area refresh cycle ISC[3:0] */
     {0x000B,  0, 0x0000},             /* Frame cycle control */
     {0x000C,  0, 0x0000},             /* RGB interface setting */

/**************** Power On sequence *****************/

     {0x0010,  0, 0x0000},             /* SAP, BT[3:0], AP, DSTB, SLP, STB */
     {0x0011,  0, 0x0007},             /* DC1[2:0], DC0[2:0], VC[2:0] */
     {0x0012,  0, 0x0000},             /* VREG1OUT voltage */
     {0x0013,200, 0x0000},             /* VDV[4:0] for VCOM amplitude, delay 200 to Dis-charge capacitor power voltage */

     {0x0010,  0, 0x6740},             /* SAP=0, BT[3:0], AP, DSTB, SLP, STB */
     {0x0011, 50, 0x0036},             /* DC1[2:0], DC0[2:0], VC[2:0], delayms(50},  0, Delay 50ms */
     {0x0012, 50, 0x001A},             /* VREG1OUT voltage, Delay 50ms */
     {0x0013, 50, 0x1600},             /* VDV[4:0] for VCOM amplitude */

  /*  Better grey scale */
       {0x0030,  0, 0x0007},
  /*   {0x0031,  0, 0x0703}, */
  /*   {0x0032,  0, 0x0702}, */
       {0x0033,  0, 0x0700},
  /*   {0x0034,  0, 0x0A1F}, */
  /*   {0x0035,  0, 0x0504}, */
  /*     {0x0036,  0, 0x0303}, */
       {0x0037,  0, 0x0007},
       {0x0038,  0, 0x1f00},
       {0x0039,  0, 0x081f},

   #endif

  /* {GCTRL_DISP_CTRL, 0, GDISP_ON} */ /* Reg 0x0007, turn disp on, to ease debug */
   {GCTRL_DISP_CTRL, 0, GDISP_OFF}     /* Reg 0x0007, turn dispoff during ram clear */
   };

/*
   Send a command
*/
static void ghw_cmd_dat_wr(CMDTYPE cmd, SGUINT cmddat)
   {
   #ifndef GHW_NOHDW
   #ifdef GHW_BUS8
   sgwrby(GHWCMD,(SGUCHAR)(cmd>>8));    /* Msb */
   sgwrby(GHWCMD,(SGUCHAR) cmd);        /* Lsb */
   sgwrby(GHWWR, (SGUCHAR)(cmddat>>8)); /* Msb */
   sgwrby(GHWWR, (SGUCHAR) cmddat);     /* Lsb */
   #elif defined( GHW_BUS16 )
   sgwrwo(GHWCMDW, (SGUINT) cmd);
   sgwrwo(GHWWRW,  cmddat);
   #else
   #ifdef GHW_SPFD5420
   /* 32 bit data bus (display connected to 18 lsb bits) Cmd bit placement CCCCCCCCxCCCCCCCCx */
   sgwrdw(GHWCMDDW,(SGULONG)(((((SGUINT)cmd) << 2) & 0x3fc0) | ((((SGUINT)cmd) << 1) & 0x1fe)));
   #else
   sgwrdw(GHWCMDDW,(SGULONG)((((SGUINT)cmd) << 1) & 0x1fe));
   #endif
   sgwrdw(GHWWRDW, (SGULONG)(((cmddat << 2) & 0x3fc0) | ((cmddat << 1) & 0x1fe)));
   #endif
   #else  /* GHW_NOHDW */
   cmd++; /* silience 'not used' warning */
   cmddat++;
   #endif
   }

static void ghw_cmd_wr(CMDTYPE cmd)
   {
   #ifndef GHW_NOHDW
   #if defined(GHW_BUS8)
   /* 8 bit data bus */
   sgwrby(GHWCMD,(SGUCHAR)(cmd>>8));    /* Msb */
   sgwrby(GHWCMD,(SGUCHAR) cmd);        /* Lsb */
   #elif defined( GHW_BUS16 )
   /* 16 bit data bus */
   sgwrwo(GHWCMDW, cmd);
   #else
   /* 32 bit data bus (display connected to 18 lsb bits) Cmd bit placement CCCCCCCCxCCCCCCCCx */
   #ifdef GHW_SPFD5420
   sgwrdw(GHWCMDDW,(SGULONG)(((((SGUINT)cmd) << 2) & 0x3fc0) | ((((SGUINT)cmd) << 1) & 0x1fe)));
   #else
   sgwrdw(GHWCMDDW, (SGULONG)(((SGUINT) cmd)<< 1));
   #endif
   #endif
   #else  /* GHW_NOHDW */
   cmd++; /* silience 'not used' warning */
   #endif
   }

/*
   Set the y range.
   The row position is set to y.
   After last write on row y2 the write position is reset to y
   Internal ghw function
*/
void ghw_set_xyrange(GXT xb, GYT yb, GXT xe, GYT ye)
   {
   #ifdef GHW_PCSIM
   ghw_set_xyrange_sim( xb, yb, xe, ye);
   #endif

   #ifndef GHW_NOHDW

   #ifdef GHW_ILI9222

     #ifdef GHW_ROTATED
     /* Set range (rotated display) */

     /* Set window range */
     ghw_cmd_dat_wr(GCTRL_H_WIN_ADR, (((SGUINT)ye+GHW_YOFFSET)<<8) + ((SGUINT)yb+GHW_YOFFSET));
     ghw_cmd_dat_wr(GCTRL_V_WIN_ADR, (((SGUINT)xe+GHW_XOFFSET)<<8) + ((SGUINT)xb+GHW_XOFFSET));
     /* Set address pointer to start of range */
     ghw_cmd_dat_wr(GCTRL_RAM_ADR,   (((SGUINT)xb+GHW_XOFFSET)<<8) + ((SGUINT)yb+GHW_YOFFSET));

     #else
     /* Set range (normal display) */

     /* Set window range */
     ghw_cmd_dat_wr(GCTRL_H_WIN_ADR, (((SGUINT)xe+GHW_XOFFSET)<<8) + ((SGUINT)xb+GHW_XOFFSET));
     ghw_cmd_dat_wr(GCTRL_V_WIN_ADR, (((SGUINT)ye+GHW_YOFFSET)<<8) + ((SGUINT)yb+GHW_YOFFSET));
     /* Set address pointer to start of range */
     ghw_cmd_dat_wr(GCTRL_RAM_ADR,   (((SGUINT)yb+GHW_YOFFSET)<<8) + ((SGUINT)xb+GHW_XOFFSET));
     #endif /* GHW_ROTATED */

   #elif defined( GHW_SSD2119 )
      #ifdef GHW_ROTATED
      /* Set window range */
      ghw_cmd_dat_wr(GCTRL_V_WIN_ADR, (((SGUINT)xe+GHW_XOFFSET)<<8) + ((SGUINT)xb+GHW_XOFFSET));
      ghw_cmd_dat_wr(GCTRL_H_WIN_ADR_STRT,(SGUINT)yb+GHW_YOFFSET);
      ghw_cmd_dat_wr(GCTRL_H_WIN_ADR_END, (SGUINT)ye+GHW_YOFFSET);
      /* Set address pointer to start of range */
      ghw_cmd_dat_wr(GCTRL_RAM_ADR_L, (SGUINT)yb+GHW_YOFFSET);
      ghw_cmd_dat_wr(GCTRL_RAM_ADR_H, (SGUINT)xb+GHW_XOFFSET);
      #else
      /* Set window range */
      ghw_cmd_dat_wr(GCTRL_V_WIN_ADR,   (((SGUINT)ye+GHW_YOFFSET)<<8) + ((SGUINT)yb+GHW_YOFFSET));
      ghw_cmd_dat_wr(GCTRL_H_WIN_ADR_STRT,(SGUINT)xb+GHW_XOFFSET);
      ghw_cmd_dat_wr(GCTRL_H_WIN_ADR_END, (SGUINT)xe+GHW_XOFFSET);
      /* Set address pointer to start of range */
      ghw_cmd_dat_wr(GCTRL_RAM_ADR_L, (SGUINT)xb+GHW_XOFFSET);
      ghw_cmd_dat_wr(GCTRL_RAM_ADR_H, (SGUINT)yb+GHW_YOFFSET);
      #endif /* GHW_ROTATED */

   #elif defined( GHW_SSD1298 )
      #ifdef GHW_ROTATED
      /* Set window range */
      ghw_cmd_dat_wr(GCTRL_H_WIN_ADR, (((SGUINT)ye+GHW_XOFFSET)<<8) + ((SGUINT)yb+GHW_XOFFSET));
      ghw_cmd_dat_wr(GCTRL_V_WIN_ADR_STRT,(SGUINT)xb+GHW_YOFFSET);
      ghw_cmd_dat_wr(GCTRL_V_WIN_ADR_END, (SGUINT)xe+GHW_YOFFSET);
      /* Set address pointer to start of range */
      ghw_cmd_dat_wr(GCTRL_RAM_ADR_L, (SGUINT)yb+GHW_YOFFSET);
      ghw_cmd_dat_wr(GCTRL_RAM_ADR_H, (SGUINT)xb+GHW_XOFFSET);
      #else
      /* Set window range */
      ghw_cmd_dat_wr(GCTRL_H_WIN_ADR,   (((SGUINT)xe+GHW_YOFFSET)<<8) + ((SGUINT)xb+GHW_YOFFSET));
      ghw_cmd_dat_wr(GCTRL_V_WIN_ADR_STRT,(SGUINT)yb+GHW_XOFFSET);
      ghw_cmd_dat_wr(GCTRL_V_WIN_ADR_END, (SGUINT)ye+GHW_XOFFSET);
      /* Set address pointer to start of range */
      ghw_cmd_dat_wr(GCTRL_RAM_ADR_L, (SGUINT)xb+GHW_XOFFSET);
      ghw_cmd_dat_wr(GCTRL_RAM_ADR_H, (SGUINT)yb+GHW_YOFFSET);
      #endif /* GHW_ROTATED */

   #else  /* GHW_ILI9320, ILI9325, GHW_ILI9225, GHW_ILI9331, GHW_R61580 */

     #ifdef GHW_ROTATED
     /* Set range (rotated display) */

     /* Set window range */
     ghw_cmd_dat_wr(GCTRL_H_WIN_ADR_STRT,(SGUINT)yb+GHW_YOFFSET);
     ghw_cmd_dat_wr(GCTRL_H_WIN_ADR_END, (SGUINT)ye+GHW_YOFFSET);
     ghw_cmd_dat_wr(GCTRL_V_WIN_ADR_STRT,(SGUINT)xb+GHW_XOFFSET);
     ghw_cmd_dat_wr(GCTRL_V_WIN_ADR_END, (SGUINT)xe+GHW_XOFFSET);
     /* Set address pointer to start of range */
     ghw_cmd_dat_wr(GCTRL_RAM_ADR_L, (SGUINT)yb+GHW_YOFFSET);
     ghw_cmd_dat_wr(GCTRL_RAM_ADR_H, (SGUINT)xb+GHW_XOFFSET);

     #else
     /* Set range (normal display) */

     /* Set window range */
     ghw_cmd_dat_wr(GCTRL_H_WIN_ADR_STRT,(SGUINT)xb+GHW_XOFFSET);
     ghw_cmd_dat_wr(GCTRL_H_WIN_ADR_END, (SGUINT)xe+GHW_XOFFSET);
     ghw_cmd_dat_wr(GCTRL_V_WIN_ADR_STRT,(SGUINT)yb+GHW_YOFFSET);
     ghw_cmd_dat_wr(GCTRL_V_WIN_ADR_END, (SGUINT)ye+GHW_YOFFSET);

     /* Set address pointer to start of range */
     ghw_cmd_dat_wr(GCTRL_RAM_ADR_L, (SGUINT)xb+GHW_XOFFSET);
     ghw_cmd_dat_wr(GCTRL_RAM_ADR_H, (SGUINT)yb+GHW_YOFFSET);

     #endif /* GHW_ROTATED */
   #endif /* controller */

   /* Prepare for auto write */
   ghw_cmd_wr(GCTRL_RAMWR);

   #endif /* GHW_NOHDW */
   }

/*
   Write databyte to controller (at current position) and increment
   internal xadr.

   Internal ghw function
*/
void ghw_auto_wr(GCOLOR dat)
   {
   #ifdef GHW_PCSIM
   ghw_autowr_sim( dat );
   #endif

   #ifndef GHW_NOHDW
    #if   ((GDISPPIXW == 16) && defined( GHW_BUS8 ))
    sgwrby(GHWWR, (SGUCHAR )(dat >> 8));
    sgwrby(GHWWR, (SGUCHAR )(dat));
    #elif ((GDISPPIXW == 18) && defined( GHW_BUS8 ))
    /* 6+6+6 */
    sgwrby(GHWWR, (SGUCHAR)(dat>>10)); /* Red (D7 alignment) */
    sgwrby(GHWWR, (SGUCHAR)(dat>>4));  /* Green (D7 alignment) */
    sgwrby(GHWWR, (SGUCHAR)(dat<<2));  /* Blue (D7 alignment) */
    #elif ((GDISPPIXW == 24) && defined( GHW_BUS8 ))
    /* 6+6+6 + format conversion  */
    sgwrby(GHWWR, (SGUCHAR)(dat>>16)); /* MSB */
    sgwrby(GHWWR, (SGUCHAR)(dat>>8));
    sgwrby(GHWWR, (SGUCHAR)(dat));     /* LSB */
    #elif ((GDISPPIXW == 16) && defined( GHW_BUS16 ))
    /* 16 bit bus mode, 16 bit color */
    sgwrwo(GHWWRW, dat);           /* 16 bit color */
    #elif ((GDISPPIXW == 18) && defined( GHW_BUS16 ))
    /* 18 bit color */
    #ifdef defined(GHW_ILI9331) || defined(GHW_R61580)
     /* MSB (RRRRRRGGGGGGBBBB) */
     sgwrwo(GHWWRW, (SGUINT)(dat >> 2));
     /* MSB (BB**************) */
     sgwrwo(GHWWRW, (SGUINT)(dat << 14));
    #else
     /* MSB (RRRRRR**GGGGGG**) */
     sgwrwo(GHWWRW, (SGUINT)( ((dat >> 2) & 0xff00) |((dat >> 4) & 0xff) )); /* 16 Msb */
     /* MSB (********BBBBBB**) */
     sgwrwo(GHWWRW, (SGUINT)(dat << 2));
    #endif
    #elif ((GDISPPIXW == 24) && defined( GHW_BUS16 ))
    /* 24 bit color */
    /* MSB (RRRRRR**GGGGGG**) */
    sgwrwo(GHWWRW, (SGUINT)(dat >> 8));
    /* MSB (********BBBBBB**) */
    sgwrwo(GHWWRW, (SGUINT)dat);
    #elif ((GDISPPIXW == 18) && defined( GHW_BUS32 ))
    /* 32 bit bus mode, 18 bit color */
    sgwrdw(GHWWRDW, dat);           /* 18 bit color */
    #elif ((GDISPPIXW == 24) && defined( GHW_BUS32 ))
    /* 32 bit bus mode, 24 bit color */
    /* 24 bit rgb collected to 18 */
    /* dat = generic 24 bit RGB = RRRRRR**GGGGGG**BBBBBB** */
    dat = ((dat & 0xfc0000) >> 6) |
          ((dat & 0x00fc00) >> 4) |
          ((dat & 0x0000fc) >> 2);
    sgwrdw(GHWWRDW, dat);           /* 18 bit color */
    #else
    #error Illegal GDISPPIXW / GHW_BUSn combination in gdispcfg.h
    #endif
   #endif /* GHW_NOHDW */
   }

/*
   Read databyte from controller at specified address
   (controller does not auto increment during read)
   The address must be within the update window range

   ILI9320 always uses 16 bit color resolution for 8 and 16 bit bus
   modes during read back, independent of the color mode setting.
   Color format conversion is done in software here.

   ILI9320 only support RGB color swap in hardware during write,
   Swaps R,B colors in software during read when GHW_COLOR_SWAP is defined

   Internal ghw function
*/
GCOLOR ghw_rd(GXT xb, GYT yb)
   {
   GCOLOR ret;

   #ifdef GHW_PCSIM
   ghw_set_xy_sim( xb, yb);
   #endif

   #if defined( GHW_ILI9222 )
     #ifdef GHW_ROTATED
     ghw_cmd_dat_wr(GCTRL_RAM_ADR,   (((SGUINT)xb+GHW_XOFFSET)<<8) + ((SGUINT)yb+GHW_YOFFSET));
     #else
     ghw_cmd_dat_wr(GCTRL_RAM_ADR,   (((SGUINT)yb+GHW_YOFFSET)<<8) + ((SGUINT)xb+GHW_XOFFSET));
     #endif
   #else /* GHW_ILI9320 etc */
     #ifdef GHW_ROTATED
     ghw_cmd_dat_wr(GCTRL_RAM_ADR_L, (SGUINT)(yb+GHW_YOFFSET));
     ghw_cmd_dat_wr(GCTRL_RAM_ADR_H, (SGUINT)(xb+GHW_XOFFSET));
     #else
     ghw_cmd_dat_wr(GCTRL_RAM_ADR_L, (SGUINT)(xb+GHW_XOFFSET));
     ghw_cmd_dat_wr(GCTRL_RAM_ADR_H, (SGUINT)(yb+GHW_YOFFSET));
     #endif
   #endif

   /* Prepare for auto read */
   ghw_cmd_wr(GCTRL_RAMRD);

   #ifndef GHW_NOHDW
    #if defined( GHW_SSD2119 ) || defined( GHW_SSD1298 )
       #if   ((GDISPPIXW == 16) && defined( GHW_BUS8 ))
       ret =   (GCOLOR) sgrdby(GHWRD); /* Dummy */
       ret = (((GCOLOR) sgrdby(GHWRD)) << 8); /* MSB*/
       ret |=  (GCOLOR) sgrdby(GHWRD);        /* LSB*/

       #elif ((GDISPPIXW == 18) && defined( GHW_BUS8 ))
       /* 8 bit bus mode */
       /* 18 bit color mode  */
       /* Left aligned color info */
       ret =    (GCOLOR)  sgrdby(GHWRD); /* Dummy */
       ret =  (((GCOLOR) (sgrdby(GHWRD) & 0xfc)) << 10);  /* MSB (RRRRRR**) */
       ret |= (((GCOLOR) (sgrdby(GHWRD) & 0xfc)) << 4);   /*     (GGGGGG**) */
       ret |= (((GCOLOR) (sgrdby(GHWRD) & 0xfc)) >> 2);   /* LSB (BBBBBB**) */

       #elif ((GDISPPIXW == 24) && defined( GHW_BUS8 ))
       /* 8 bit bus mode */
       /* 24 bit color mode (3 lsb bytes is r,g,b) */
       /* Left aligned color info */
       ret =    (GCOLOR) sgrdby(GHWRD); /* Dummy */
       ret =  (((GCOLOR) sgrdby(GHWRD)) << 16);  /* MSB (RRRRRR**) */
       ret |= (((GCOLOR) sgrdby(GHWRD)) << 8);   /*     (GGGGGG**) */
       ret |=  ((GCOLOR) sgrdby(GHWRD));         /* LSB (BBBBBB**) */

       #elif ((GDISPPIXW == 16) && defined( GHW_BUS16 ))
       /* 16 bit bus mode, 16 bit color */
       ret = (GCOLOR) sgrdwo(GHWRDW); /* Dummy */
       ret = (GCOLOR) sgrdwo(GHWRDW); /* 16 bit color */

       #elif ((GDISPPIXW == 18) && defined( GHW_BUS16 ))
       /* 18 color bits read in 2 words, assemble to 18 bit rgb*/
       ret = (GCOLOR) sgrdwo(GHWRDW); /* Dummy */
       ret = (GCOLOR) sgrdwo(GHWRDW);                   /* MSB (RRRRRR**GGGGGG**) */
       ret = ((ret & 0xfc00) << 2) + ((ret & 0xfc)<< 4);
       ret |= (GCOLOR)((sgrdwo(GHWRDW) & 0xfc) >> 2);   /* LSB (********BBBBBB**) */

       #elif ((GDISPPIXW == 24) && defined( GHW_BUS16 ))
       /* 18 color bits read in 2 words, assemble to 24 bit rgb*/
       ret = (GCOLOR) sgrdwo(GHWRDW); /* Dummy */
       ret =  ((GCOLOR)(sgrdwo(GHWRDW) & 0xfcfc)) << 8; /* MSB (RRRRRR**GGGGGG**) */
       ret |= (GCOLOR)(sgrdwo(GHWRDW)  & 0x00fc);      /* LSB (********BBBBBB**) */

       #elif ((GDISPPIXW == 18) && defined( GHW_BUS32 ))
       /* 32 bit bus mode, 18 bit color */
       ret = (GCOLOR) sgrddw(GHWRDDW);  /* Dummy */
       ret = (GCOLOR) sgrddw(GHWRDDW);  /* 18 bit color */

       #elif ((GDISPPIXW == 24) && defined( GHW_BUS32 ))
       /* 18 bit collective, split to 24 bit rgb*/
       ret = (GCOLOR) sgrddw(GHWRDDW); /* Dummy */
       ret = (GCOLOR) sgrddw(GHWRDDW); /* 18 bit color */
       ret = ((ret << 6) & 0xfc0000) + ((ret << 4) & 0xfc00) + ((ret << 2)&0xfc);
       #else
       #error Illegal GDISPPIXW / GHW_BUSn combination in gdispcfg.h
       #endif

    #else

       #if   ((GDISPPIXW == 16) && defined( GHW_BUS8 ))
       ret = (GCOLOR) sgrdby(GHWRD); /* Dummy */
       ret = (GCOLOR) sgrdby(GHWRD); /* Dummy */
       ret = (((GCOLOR) sgrdby(GHWRD)) << 8); /* MSB*/
       ret |=  (GCOLOR) sgrdby(GHWRD);        /* LSB*/
       #ifdef GHW_USE_SWRD_COLORSWAP
       ret = ((ret << 11) & 0xf800)  | (ret & 0x07e0) | ((ret >> 11) & 0x001f);
       #endif

       #elif ((GDISPPIXW == 18) && defined( GHW_BUS8 ))
       /* Read as 16 bit */
       ret = (GCOLOR) sgrdby(GHWRD); /* Dummy */
       ret = (GCOLOR) sgrdby(GHWRD); /* Dummy */
       ret = (((GCOLOR) sgrdby(GHWRD)) << 8); /* MSB*/
       ret |=  (GCOLOR) sgrdby(GHWRD);        /* LSB*/
       #ifdef GHW_USE_SWRD_COLORSWAP
       ret = ((ret << 13) & 0x3e000) + ((ret << 1) & 0xfc0) + ((ret >> 10) & 0x3e);
       #else
       ret = ((ret << 2) & 0x3e000) + ((ret << 1) & 0xfc0) + ((ret << 1) & 0x3e);
       #endif

       #elif ((GDISPPIXW == 24) && defined( GHW_BUS8 ))
       /* read as 16 bit + format conversion  */
       ret = (GCOLOR) sgrdby(GHWRD); /* Dummy */
       ret = (GCOLOR) sgrdby(GHWRD); /* Dummy */
       ret = (((GCOLOR) sgrdby(GHWRD)) << 8); /* MSB*/
       ret |=  (GCOLOR) sgrdby(GHWRD);        /* LSB*/
       #ifdef GHW_USE_SWRD_COLORSWAP
       ret = ((ret << 19) & 0xf80000) + ((ret << 5) & 0xfc00) + ((ret >> 8) & 0xf8);
       #else
       ret = ((ret << 8) & 0xf80000) + ((ret << 5) & 0xfc00) + ((ret << 3) & 0xf8);
       #endif

       #elif ((GDISPPIXW == 16) && defined( GHW_BUS16 ))
       /* 16 bit bus mode, 16 bit color */
       ret = (GCOLOR) sgrdwo(GHWRDW);  /* Dummy */
       ret = (GCOLOR) sgrdwo(GHWRDW); /* 16 bit color */
       #ifdef GHW_USE_SWRD_COLORSWAP
       ret = ((ret << 11) & 0xf800)  | (ret & 0x07e0) | ((ret >> 11) & 0x001f);
       #endif

       #elif ((GDISPPIXW == 18) && defined( GHW_BUS16 ))
       /* 16 color read, split to 18 bit rgb*/
       ret = (GCOLOR) sgrdwo(GHWRDW); /* Dummy */
       ret = (GCOLOR) sgrdwo(GHWRDW); /* 16 bit color read */
       #ifdef GHW_USE_SWRD_COLORSWAP
       ret = ((ret << 13) & 0x3e000) + ((ret << 1) & 0xfc0) + ((ret >> 10) & 0x3e);
       #else
       ret = ((ret << 2) & 0x3e000) + ((ret << 1) & 0xfc0) + ((ret << 1) & 0x3e);
       #endif

       #elif ((GDISPPIXW == 24) && defined( GHW_BUS16 ))
       /* 16 color read, split to 24 bit rgb*/
       ret = (GCOLOR) sgrdwo(GHWRDW); /* Dummy */
       ret = (GCOLOR) sgrdwo(GHWRDW); /* 16 bit color read */
       #ifdef GHW_USE_SWRD_COLORSWAP
       ret = ((ret << 19) & 0xf80000) + ((ret << 5) & 0xfc00) + ((ret >> 8) & 0xf8);
       #else
       ret = ((ret << 8) & 0xf80000) + ((ret << 5) & 0xfc00) + ((ret << 3) & 0xf8);
       #endif

       #elif ((GDISPPIXW == 18) && defined( GHW_BUS32 ))
       /* 32 bit bus mode, 18 bit color */
       ret = (GCOLOR) sgrddw(GHWRDDW);  /* Dummy */
       ret = (GCOLOR) sgrddw(GHWRDDW);  /* 18 bit color */
       #ifdef GHW_USE_SWRD_COLORSWAP
       ret = ((ret << 12) & 0x3f00)  | (ret & 0x0fe0) | ((ret >> 12) & 0x003f);
       #endif

       #elif ((GDISPPIXW == 24) && defined( GHW_BUS32 ))
       /* 18 bit collective  split to 24 bit rgb*/
       ret = (GCOLOR) sgrddw(GHWRDDW); /* Dummy */
       ret = (GCOLOR) sgrddw(GHWRDDW); /* 18 bit color */
       #ifdef GHW_USE_SWRD_COLORSWAP
       ret = ((ret << 18) & 0xfc0000) + ((ret << 4) & 0xfc00) + ((ret >> 10)&0xfc);
       #else
       ret = ((ret << 6) & 0xfc0000) + ((ret << 4) & 0xfc00) + ((ret << 2)&0xfc);
       #endif
       #else
       #error Illegal GDISPPIXW / GHW_BUSn combination in gdispcfg.h
       #endif

   #endif

   #else /* GHW_NOHDW */

    #ifdef GHW_PCSIM
     ret = ghw_autord_sim();
    #else
     ret = 0;
    #endif

   #endif /* GHW_NOHDW */
   return ret;
   }

/***********************************************************************/
/**        s6d0129 Initialization and error handling functions       **/
/***********************************************************************/

/*
   Change default (palette) colors
*/
void ghw_setcolor(GCOLOR fore, GCOLOR back)
   {
   /* Update active colors */
   #if ((GDISPPIXW == 18) && !defined(GHW_BUS32))
   /* 16 bit pr pixel always returned. Limit so symmetrical read,write values are used */
   ghw_def_foreground = fore & 0x3effe;
   ghw_def_background = back & 0x3effe;
   #elif (GDISPPIXW == 24)
   /* 16 bit pr pixel always returned. Limit so symmetrical read,write values are used */
   ghw_def_foreground = fore & 0xf8fcf8;
   ghw_def_background = back & 0xf8fcf8;
   #else
   ghw_def_foreground = fore;
   ghw_def_background = back;
   #endif
   }

/*
   Convert an RGB structure to a color value using the current color mode
*/
GCOLOR ghw_rgb_to_color( GCONSTP GPALETTE_RGB *palette )
   {
   if (palette == NULL)
      return 0;
   return G_RGB_TO_COLOR(palette->r,palette->g,palette->b);
   }


#if (GHW_PALETTE_SIZE > 0)
/*
   Load a new palette or update the existing palette
   (Palette is only used with symbols using 2 or 4 bits pr pixel)
*/
SGBOOL ghw_palette_wr(SGUINT start_index, SGUINT num_elements, GCONSTP GPALETTE_RGB PFCODE *palette)
   {
   if ((num_elements == 0) ||
       ((start_index + num_elements) > 16) ||
       (palette == NULL))
      {
      glcd_err = 1;
      return 1;
      }
   glcd_err = 0;

   /* (Partial) update of operative palette values */
   while(num_elements-- > 0)
      {
      /* Make local palette copy here to be compatible with compilers
         having a non-standard conforming handling of pointer
         (i.e when PFCODE memory qualifer is used) */
      GPALETTE_RGB pal;
      pal.r = palette->r;
      pal.g = palette->g;
      pal.b = palette->b;
      ghw_palette_opr[start_index++] = ghw_rgb_to_color(&pal);
      palette++;

      /* ghw_palette_opr[start_index++] = ghw_rgb_to_color(&palette++); */
      }

   return glcd_err;
   }
#endif

/*
   Fast set or clear of LCD module RAM buffer
   Internal ghw function
*/
static void ghw_bufset(GCOLOR color)
   {
   /* Use hardware accelerator logic */
   GBUFINT cnt;
   cnt = 0;
   ghw_set_xyrange(0,0,GDISPW-1,GDISPH-1);
   do
      {
      /* Clear using X,Y autoincrement */
      ghw_auto_wr(color);  /* Set LCD buffer */
      #ifdef GBUFFER
      gbuf[cnt] = color; /* Set ram buffer as well */
      #endif
      }
   while (++cnt < ((GBUFINT) GDISPW) * ((GBUFINT) GDISPH)); /* Loop until x+y wrap */
   }


#if (defined( WR_RD_TEST ) && !defined(GHW_NO_LCD_READ_SUPPORT))

/*
   Make write-readback test on controller memory.

   This test will fail if some databus and control signals is not connected correctly.

   This test will fail if 16/8 bit bus mode selection in the configuration settings
   does not match the actual bus configuration for the hardware (display and processor
   16/8 bit bus width, 8080/6800 bus type settings, word / byte address offsets, etc).

   This test may fail if illegal GCTRLW, GCTRLH, GHW_XOFFSET, GHW_YOFFSET
   configuration settings cause overrun of the on-chip video RAM.

   This test may fail if GHW_COLOR_SWAP is defined AND RGB,BGR color lane swapping during is not
   done in hardware with read operations (i.e. the controller variant has assymetric write -
   read data formats) Defining GHW_USE_SWRD_COLORSWAP in the top of this module will enable code
   generation for doing read RGB,BGR color swapping in software.

   This test can be executed correctly with only logic power on the display module.
   No high-level voltages are necessary for the test to run (although nothing then can
   be shown on the display)

   Return 0 if no error,
   Return != 0 if some readback error is detected (the bit pattern may give information
   about connector pins in error)
*/

/*#define  GPRINTF( format, data ) printf((format), (data) )*/  /* Info via printf */
#define  GPRINTF( format, data ) /* Use no info */

static GCOLOR ghw_wr_rd_test(void)
   {
   #ifndef GHW_NOHDW
   int i;
   GCOLOR msk,result;
   ghw_set_xyrange(0,0,GDISPW-1,GDISPH-1);

   #if (GDISPPIXW > 16)
   /* 24 (18) bit color mode */
   GPRINTF("\n%s","");
   for (i = 0, msk = 1; i < GDISPPIXW; i++)
      {
      ghw_auto_wr(msk);
      GPRINTF("0x%06x ", (unsigned long) msk);
      ghw_auto_wr(~msk);
      GPRINTF(" 0x%06x\n", (unsigned long) (~msk));
      msk <<= 1;
      }
   GPRINTF("\n%s","");
   for (i=0, msk=1, result=0; i < GDISPPIXW; i++)
      {
      GCOLOR val;
      val = ghw_rd(i*2,0);
      result |= (val ^ msk);
      GPRINTF("0x%06lx ",  (unsigned long) val);
      val = ghw_rd(i*2+1,0);
      GPRINTF(" 0x%06lx\n", (unsigned long) val );
      result |= (val ^ (~msk));
      msk <<= 1;
      }
   result &= GHW_COLOR_CMP_MSK; /* Mask bits unused by controller during read back */

   #else
   /* 16 bit color mode */
   GPRINTF("\n%s","");
   for (i = 0, msk = 1; i < GDISPPIXW; i++)
      {
      ghw_auto_wr(msk);
      GPRINTF("0x%04x ", (unsigned int) msk);
      ghw_auto_wr(~msk);
      GPRINTF(" 0x%04x\n", (unsigned int) (~msk & 0xffff));
      msk <<= 1;
      }
   GPRINTF("\n%s","");
   for (i=0, msk=1, result=0; i < GDISPPIXW; i++)
      {
      GCOLOR val;
      val = ghw_rd(i*2,0);
      result |= (val ^ msk);
      GPRINTF("0x%04x ",   (unsigned short) val);
      val = ghw_rd(i*2+1,0);
      GPRINTF(" 0x%04x\n", (unsigned short) val );
      result |= (val ^ (~msk));
      msk <<= 1;
      }
   #endif
   return result;  /* 0 = Nul errors */
   #else
   return 0; /* 0 = Nul errors */
   #endif
   }

#endif /* WR_RD_TEST */

/*
   Wait a number of milli seconds
*/
static void ghw_cmd_wait(SGUCHAR ms)
   {
   #ifdef SGPCMODE
   Sleep(ms); /* delay x 1 ms */
   #else
   hal_timer_delay(ms);
   #endif
   }

/*
   Initialize display, clear ram  (low-level)
   Clears glcd_err status before init

   Return 0 if no error,
   Return != 0 if some error
*/
SGBOOL ghw_init(void)
   {
   short i;
   #ifdef GBUFFER
   iltx = 1;
   ilty = 1;
   irbx = 0;
   irby = 0;
   ghw_upddelay = 0;
   #endif

   glcd_err = 0;
   ghw_io_init(); /* Set any hardware interface lines, controller hardware reset */

   #if (defined( GHW_ALLOCATE_BUF) && defined( GBUFFER ))
   if (gbuf == NULL)
      {
      /* Allocate graphic ram buffer */
      if ((gbuf = (GCOLOR *)calloc(ghw_gbufsize(),1)) == NULL)
         glcd_err = 1;
      else
         gbuf_owner = 1;
      }
   #endif

   if (glcd_err != 0)
      return 1;

   #ifdef GHW_PCSIM
   /* Tell simulator about the visual LCD screen organization */
   ghw_init_sim( GDISPW, GDISPH );
   #endif
   /* Set default colors */
   ghw_setcolor( GHW_PALETTE_FOREGROUND, GHW_PALETTE_BACKGROUND );

   #if (GHW_PALETTE_SIZE > 0)
   /* Load palette */
   ghw_palette_wr(0, sizeof(ghw_palette)/sizeof(GPALETTE_RGB), (GCONSTP GPALETTE_RGB PFCODE *)&ghw_palette[0]);
   #endif

   /* Initialize controller according to configuration file */
   for (i=0; i < sizeof(as1dregs)/sizeof(S1D_REGS); i++)
      {
      ghw_cmd_dat_wr(as1dregs[i].index,as1dregs[i].value);
      /*printf("\n{0x%02x, 0x%04x},",as1dregs[i].index,as1dregs[i].value);*/
      if (as1dregs[i].delay != 0)
         {
         ghw_cmd_wait( as1dregs[i].delay );
         /*printf("\nDelay( %u );",as1dregs[i].delay);*/
         }
      }

   /*
   #ifdef GHW_BUS8
   {
   // Communication test. Read of chip ID number
   static SGUCHAR dat1,dat2;
   for(;;)
      {
      sgwrby(GHWCMD, 0x00);
      sgwrby(GHWCMD, 0x00);
      dat1 = sgrdby(GHWRD);
      dat2 = sgrdby(GHWRD);
      printf("\nID %02x, %02x", (unsigned int) dat1, (unsigned int) dat2);
      }
   }
   #endif
   #ifdef GHW_BUS16
   {
   // Communication test. Read of chip ID number
   static SGUINT dat;
   for(;;)
      {
      sgwrwo(GHWCMDW, 0x0000);
      dat = sgrdwo(GHWRDW);
      printf("\nID %04x", (unsigned int) dat);
      }
   }
   #endif
   */

   /*
      Stimuli test loops for initial oscilloscope test of display interface bus signals
      Uncomment to use the test loop for the given data bus width.
      It is recommended to check all display bus signals with each of the I/O access
      statements in the loop one by one.
   */
   /*
   #ifdef GHW_BUS8
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
   /*
   #ifdef GHW_BUS16
   for(;;)
      {
      SGUINT dat;
      sgwrwo(GHWCMDW,0xffff);
      sgwrwo(GHWWRW,0x0000);
      dat = sgrdwo(GHWSTAW);
      dat = sgrdwo(GHWRDW);
      }
   #endif
   */

   #if (defined( WR_RD_TEST ) && !defined(GHW_NO_LCD_READ_SUPPORT))
   /*
      NOTE:
      The call of ghw_wr_rd_test() should be commented out in serial mode.
      In serial mode the display controller  does not provide read-back facility
      and this test will always fail.
   */
   if (ghw_wr_rd_test() != ((GCOLOR) 0))
      {
      /* Controller memory write-readback error detected
      (Check the cable or power connections to the display) */
      G_WARNING("Hardware interface error\nCheck display connections\n");  /* Test Warning message output */
      glcd_err = 1;
      return 1;
      }
   #endif

   ghw_dispon();     /* Turn on here to show buffer data at once, ease initial hardware debug */
   /*ghw_bufset( G_RED ); */ /* Test color order */
   /*ghw_bufset( G_GREEN );*/
   /*ghw_bufset( G_BLUE ); */
   ghw_bufset( ghw_def_background );
  /* ghw_dispon(); */ /* Normally turn on here to hide the buffer clear */


   #ifndef GNOCURSOR
   ghw_cursor = GCURSIZE1;    /* Cursor is off initially */
   /* ghw_cursor = GCURSIZE1 | GCURON; */ /* Uncomment to set cursor on initially */
   #endif

   ghw_updatehw();  /* Flush to display hdw or simulator */

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


/*
   Display a (fatal) error message.
   The LCD display module is always cleared and initialized to
   the system font in advance.
   The error message is automatically centered on the screen
   and any \n characters in the string is processed.

   str = ASCII string to write at display center
*/
void ghw_puterr( PGCSTR str )
   {
   PGCSTR idx;
   SGUINT xcnt;
   GXT xp;
   GYT yp,h,y, sidx;
   PGSYMBYTE psym;
   GCOLOR pval;
   SGUCHAR val;
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

   /* Set start character line */
   h = SYSFONT.symheight;
   yp = (xcnt*h > GDISPH) ? 0 : ((GDISPH-1)-xcnt*h)/2;
   /* Set character height in pixel lines */

   idx=str;
   do
      {
      xcnt=0;  /* Set start x position so line is centered */
      while ((idx[xcnt]!=0) && (idx[xcnt]!='\n') && (xcnt < GDISPBW))
         {
         xcnt++;
         }

      /* Calculate start position for centered line */
      xp = (GDISPW-xcnt*SYSFONT.symwidth)/2;

      /* Display text line */
      while (xcnt-- > 0)
         {
         /* Point to graphic content for character symbol */
         psym = &(sysfontsym[(*idx) & 0x7f].b[0]);
         ghw_set_xyrange(xp,yp,xp+SYSFONT.symwidth-1,yp+(h-1));

         /* Display rows in symbol */
         for (y = 0; y < h; y++)
            {
            /* Get symbol row value */
            val = *psym++;
            /* Initiate LCD controller address pointer */
            #ifdef GBUFFER
            gbufidx = GINDEX(xp, (GBUFINT)yp+y );
            #endif

            /* Display colums in symbol row */
            for (sidx = 0; sidx < SYSFONT.symwidth; sidx++)
               {
               if ((val & sympixmsk[sidx]) != 0)
                  pval = ghw_def_foreground;
               else
                  pval = ghw_def_background;

               /* End of symbol or end of byte reached */
               #ifdef GBUFFER
               gbuf[gbufidx++] = pval;
               #endif
               ghw_auto_wr(pval);
               }
            }

         idx++;
         xp += SYSFONT.symwidth; /* Move to next symbol in line */
         }

      /* Next text line */
      yp += h;
      if (*idx == '\n')
         idx++;
      }
   while ((*idx != 0) && (yp < GDISPH));

   ghw_updatehw();  /* Flush to display hdw or simulator */
   }

void ghw_exit(void)
   {
   #if defined( GHW_ALLOCATE_BUF)
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
   ghw_io_exit();         /* Release any LCD hardware resources, if required */
   #ifdef GHW_PCSIM
   ghw_exit_sim(); /* Release simulator resources */
   #endif
   }

#ifndef GNOCURSOR
/*
   Replace cursor type data (there is no HW cursor support in s6d0129)
*/
void ghw_setcursor( GCURSOR type)
   {
   ghw_cursor = type;
   #ifdef GHW_ALLOCATE_BUF
   if (gbuf == NULL)
      glcd_err = 1;
   #endif
   }
#endif


/*
   Turn display off
   (Minimize power consumption)
*/
void ghw_dispoff(void)
   {
   #ifdef GHW_PCSIM
   ghw_dispoff_sim();
   #endif
   ghw_cmd_dat_wr(GCTRL_DISP_CTRL, GDISP_OFF);     /* Blank display */
   }

/*
   Turn display on
*/
void ghw_dispon(void)
   {
   #ifdef GHW_PCSIM
   ghw_dispon_sim();
   #endif
   ghw_cmd_dat_wr(GCTRL_DISP_CTRL, GDISP_ON);     /* Restore display */
   }

#if defined( GHW_ALLOCATE_BUF)
/*
   Size of buffer requied to save the whole screen state
*/
GBUFINT ghw_gbufsize( void )
   {
   return (GBUFINT) GBUFSIZE * sizeof(GCOLOR) + (GBUFINT) sizeof(GHW_STATE);
   }

#ifdef GSCREENS
/*
   Check if screen buf owns the screen ressources.
*/
SGUCHAR ghw_is_owner( SGUCHAR *buf )
   {
   return (((GCOLOR *)buf == gbuf) && (gbuf != NULL)) ? 1 : 0;
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
   the pointer is updated. Otherwise the the buffer
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

   if (doinit != 0)
      {
      /* First screen initialization, just set buffer pointer and
         leave rest of initialization to a later call of ghw_init() */
      gbuf = (GCOLOR *) buf;
      gbuf_owner = 0;
      }
   else
      {
      if ((gbuf = (GCOLOR *) buf) != NULL)
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
         ghw_upddelay = (ps->upddelay != 0) ? 1 : 0;
         /* Restore drawing color */
         ghw_setcolor(ps->foreground, ps->background);
         }
      }
   }
#endif  /* GSCREENS */
#endif  /* GHW_ALLOCATE_BUF */

#endif /* GBASIC_INIT_ERR */


