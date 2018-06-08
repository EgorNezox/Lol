#ifndef S6D0129_H
#define S6D0129_H
/****************************** S6D0129.H *****************************

   Definitions specific for the S6D0129 Graphic LCD controller.

   The S6D0129 controller is assumed to be used with a LCD module.
   The LCD module characteristics (width, height etc) must be correctly
   defined in GDISPCFG.H

   This header should only be included by the low-level LCD drivers

   Revision date:    11-04-07
   Revision Purpose: SSD1289 register definitions added

   Revision date:    28-08-07
   Revision Purpose: Register definitions made local to ghwinit.c modules
                     and removed from this module

   Revision date:    23-12-08
   Revision Purpose: ILI9225 support added

   Version number: 1.1
   Copyright (c) RAMTEX Engineering 2006-2008

***********************************************************************/

#include <gdisphw.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define color compare mask for controller
   Mask bits which either are unused by controller or are asymmetrical
   with respect to write / read back for the given color pixel resolution.
*/
#if (defined( GHW_HX8325_CMDINTF ) || defined( GHW_HX8325_REGINTF ))
  /* HX8325 has full 24 bit color resolution */
  #if (GDISPPIXW == 24)
   #define GHW_COLOR_CMP_MSK  0xffffff
  #elif (GDISPPIXW == 18)
   #define GHW_COLOR_CMP_MSK   0x3ffff
  #else
   #define GHW_COLOR_CMP_MSK    0xffff
  #endif
#elif (defined( GHW_ILI9320) || defined(GHW_S1D13742) || defined(GHW_UC1682) || defined(GHW_ILI9331))
  /* GHW_ILI9320 only returns 16 bit pr pixel during read back */
  #if (GDISPPIXW == 24)
   #define GHW_COLOR_CMP_MSK  0xf8fcf8
  #elif (GDISPPIXW == 18)
   #define GHW_COLOR_CMP_MSK   0x3effe
  #else
   #define GHW_COLOR_CMP_MSK    0xffff
  #endif
#else
  #if (GDISPPIXW == 24)
   #define GHW_COLOR_CMP_MSK  0xfcfcfc
  #elif (GDISPPIXW == 18)
   #define GHW_COLOR_CMP_MSK   0x3ffff
  #else
   #define GHW_COLOR_CMP_MSK    0xffff
  #endif
#endif

/* These controllers do not support address auto increment during data read */
#if (defined( GHW_S6D0129 ) || defined( GHW_S6D0118 ) || \
     defined( GHW_R61505  ) || defined( GHW_R61506  ) || defined( GHW_TL1771)   || \
     defined( GHW_SSD1286 ) || defined( GHW_SSD1285 ) || defined( GHW_SSD1283 ) || \
     defined( GHW_ILI9320)  || defined( GHW_ILI9222)  ||defined( GHW_ILI9325)  || defined( GHW_ILI9225)  || defined( GHW_ILI9331)  ||\
     defined( GHW_SSD1289)  || defined( GHW_SSD1288 ) || defined( GHW_SSD1298 ) || defined( GHW_SSD2119 ) || \
     defined( GHW_HX8345)   || defined( GHW_SPFD5420 ) || \
     defined( GHW_S6E63D6)  || defined( GHW_SEPS525 ) || defined( GHW_R61509) || defined(GHW_R61580) || \
     defined( GHW_S1D13742) || defined( GHW_OTM2201 ))
   #define GHW_NO_RDINC
#endif


/* Internal functions and types used only by other ghw_xxx functions */

#ifdef GBASIC_INIT_ERR

#if (GHW_PALETTE_SIZE > 0)
extern GCOLOR ghw_palette_opr[16];
#endif

void ghw_set_xyrange(GXT xb, GYT yb, GXT xe, GYT ye);
void ghw_auto_wr(GCOLOR dat);
SGUINT ghw_sta(void);

#ifdef GHW_NO_RDINC
GCOLOR ghw_rd(GXT xb, GYT yb);
#else
void ghw_setxypos(GXT xb, GYT yb);
void ghw_auto_rd_start(void);
GCOLOR ghw_auto_rd(void);
#endif

#define  GPIXMASKSIZE (GDISPCW/GDISPPIXW)

extern GCODE SGUCHAR FCODE sympixmsk[8];

#endif /* GBASIC_INIT_ERR */

/*
   Internal data types used only by ghw_xxx functions
   The data types are located in ghw_init
*/
extern SGBOOL glcd_err;        /* Internal hdw error */

#define GBUFSIZE (((GBUFINT)GDISPW) * ((GBUFINT)GDISPH))

#ifdef GBUFFER

   extern SGBOOL ghw_upddelay;

   /* "Dirty area" buffer controls for ghw_update speed optimization */
   extern GXT GFAST iltx,irbx;
   extern GYT GFAST ilty,irby;

   #define invalx( irx ) { \
      register GXT rirx; \
      rirx = (GXT)((irx)); \
      if(  irbx < iltx ) iltx = irbx = rirx; \
      else if( rirx < iltx ) iltx = rirx; \
      else if( rirx > irbx ) irbx = rirx; \
      }

   #define invaly( iry ) { \
      register GYT riry; \
      riry = (GYT)((iry)); \
      if( irby < ilty) ilty = irby = riry; \
      else if( riry < ilty ) ilty = riry; \
      else if( riry > irby ) irby = riry; \
      }

   #define invalrect( irx, iry ) { \
      invalx( irx ); \
      invaly( iry ); \
      }

   #ifdef GHW_ALLOCATE_BUF
      extern   GCOLOR *gbuf;                  /* Graphic buffer pointer */
      #define GBUF_CHECK()  {if (gbuf == NULL) {glcd_err=1;return;}}
   #else
      extern   GCOLOR gbuf[GBUFSIZE];         /* Graphic buffer */
      #define GBUF_CHECK()  { /* Nothing */ }
   #endif

   /* Structure to save the low-level state information */
   typedef struct _GHW_STATE
      {
      SGUCHAR upddelay;  /* Store for ghw_update */
      #ifndef GNOCURSOR
      GCURSOR cursor;    /* Store for ghw_cursor */
      #endif
      GCOLOR foreground; /* Store for current foreground and background color */
      GCOLOR background;
      } GHW_STATE;

#else  /* GBUFFER */

   #ifdef GHW_ALLOCATE_BUF
     #undef GHW_ALLOCATE_BUF /* Allocation must only be active in buffered mode */
   #endif
   #define GBUF_CHECK()  { /* Nothing */ }

   extern GCOLOR ghw_tmpbuf[GDISPW]; /* Row line buffer (to speed scroll and invert) */

#endif /* GBUFFER */

#define  GDISPBW (GDISPW*(GDISPPIXW/GDISPCW)) /* width of display in bytes, do not alter */
#define  GPIXMAX ((1<<GDISPPIXW)-1)           /* Maximum pixel value, or pixel mask, do not alter */
#define  GINDEX(x,y) (((GBUFINT)(x)) + ((GBUFINT)(y))*GDISPW)
#define  GPIXEL(x)   ((x) % 8) /* b&w pixel */

#ifdef __cplusplus
}
#endif


#endif /* S6D0129_H */
