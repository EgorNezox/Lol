#ifndef SSD0323_H
#define SSD0323_H
/****************************** SSD0323.H *****************************

   Definitions specific for the SSD0323 Graphic LCD controller.

   The SSD0323 controller is assumed to be used with a LCD module.
   The LCD module characteristics (width, height etc) must be correctly
   defined in GDISPCFG.H

   This header should only be included by the low-level LCD drivers

   Creation date:

   Revision date      030520
   Revision Purpose:  GHW_STATE ypos is GYT type
   Revision date:     051227
   Revision Purpose:  GBUFSIZE definition corrected
   Revision date:     080808
   Revision Purpose:  Updated for new switch handling
   Revision date:     041108
   Revision Purpose:  Made common for SSD1322
   Revision date:     270215
   Revision Purpose:  Support for vertical storage units (rotated sceen) added
                      New low-level internal ghw_xxx() functions

   Version number: 1.7
   Copyright (c) RAMTEX International Aps 2003-2015
   Web site, support and upgrade: www.ramtex.dk

***********************************************************************/
#include <gdisphw.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Internal functions and types used only by other ghw_xxx functions */

/* Define pixel storage manipulation types */
#if (GDISPHCW <= 8)
 #define  GHWCOLOR  SGUCHAR    /* hardware stores video data in a byte unit */
 #define  GHWCOLORD SGUINT     /* temporary type to hold GHWCOLOR data when shifted across storage units */
 #define  GHWCOLOR_NO_MSK 0xff
#else /* (GDISPHCW <= 16) */
 #define  GHWCOLOR SGUINT     /* hardware store video data in a word unit */
 #define  GHWCOLORD SGLONG    /* temporary type to hold a GHWCOLOR data when shifted one storage unit */
 #define  GHWCOLOR_NO_MSK 0xffff
#endif

#define  GHWPIXPSU    (GDISPHCW/GDISPPIXW)   /* Number of pixels in display controller storage unit */
#define  GPIXMAX      ((1<<GDISPPIXW)-1)     /* Maximum pixel value, or pixel mask, do not alter */
#define  GPIXEL(c)    ((c) &  (GHWPIXPSU-1))
#define  GPIXMSK(c)   ((c) & ~(GHWPIXPSU-1)) /* Compare mask */

#ifdef GHW_USING_VBYTE
 /* The controller uses vertical storage units */
 #define  GDISPSW     (GDISPW)     /* width of display in storage units */
 #define  GBUFSIZE    (((GDISPH+(GHWPIXPSU-1))/GHWPIXPSU)*GDISPSW)
 #define  GYBYTE(y)   (((SGUINT)(y))/GHWPIXPSU)
 #define  GXBYTE(x)   (x)
#else
 /* The controller uses horizontal storage units  */
 #define  GDISPSW     ((GDISPW*GDISPPIXW+(GDISPHCW-1))/GDISPHCW) /* width of display in storage units */
 #define  GBUFSIZE    (((GBUFINT)GDISPSW) * ((GBUFINT)GDISPH))
 #define  GYBYTE(y)   (y)
 #define  GXBYTE(x)   (((SGUINT)(x))/GHWPIXPSU)
#endif

#define  GINDEX(x,y)  (((GBUFINT)GXBYTE(x)) + ((GBUFINT)(GYBYTE(y)))*GDISPSW)

#ifdef GBASIC_INIT_ERR
extern GHWCOLOR ghw_foreground;   /* Prepared values with equal pixel colors */
extern GHWCOLOR ghw_background;

GHWCOLOR ghw_prepare_color(GCOLOR color);
void ghw_setcolor(GCOLOR fore, GCOLOR back);

extern GPALETTE_GREY ghw_palette_opr[(1<<GDISPPIXW)];

extern GCODE SGUCHAR FCODE sympixmsk[8];
extern GCODE GHWCOLOR FCODE pixmsk[GHWPIXPSU];
extern GCODE GHWCOLOR FCODE startmask[GHWPIXPSU];
extern GCODE GHWCOLOR FCODE stopmask[GHWPIXPSU];
extern GCODE GHWCOLOR FCODE shiftmsk[GHWPIXPSU];

#ifndef TMPBUFSIZE
 /* Define sizeof ghw_tmpbuf.
    Speed for some low-level operations can be improved in non-buffered
    mode (GBUFFER undefined) by increasing TMPBUFSIZE */
 #define TMPBUFSIZE 16
#elif (TMPBUFSIZE < 16)
 #undef TMPBUFSIZE
 #define TMPBUFSIZE 16
#endif

extern GHWCOLOR ghw_tmpbuf[TMPBUFSIZE];

#endif /* GBASIC_INIT_ERR */

/*
   Internal data types used only by ghw_xxx functions
   The data types are located in ghw_init
*/

extern SGBOOL glcd_err;        /* Internal hdw error */

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
      extern  GHWCOLOR *gbuf;                  /* Graphic buffer pointer */
      #define GBUF_CHECK()  {if (gbuf == NULL) {glcd_err=1;return;}}
   #else
      extern  GHWCOLOR gbuf[GBUFSIZE];         /* Graphic buffer */
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

#endif /* GBUFFER */

/** Hardware initialization and interface functions in ghwinitctrl.c **/
void ghw_ctrl_init(void);
void ghw_ctrl_exit(void);
void ghw_ctrl_dispoff(void);
void ghw_ctrl_dispon(void);
void ghw_ctrl_cont_set(SGUCHAR contrast);

void ghw_activate_palette(void);
void ghw_set_xrange(GXT xb, GXT xe);
void ghw_set_yrange(GYT xb, GYT xe);
#define ghw_set_xpos(xb) ghw_set_xrange((xb), GDISPW-1)
#define ghw_set_ypos(yb) ghw_set_yrange((yb), GDISPH-1)

void ghw_auto_wr(GHWCOLOR dat);
void ghw_auto_rd_start(void);
GHWCOLOR ghw_auto_rd(void);
//#ifdef GHW_SSD1322
//GHWCOLOR ghw_rd( GXT x, GYT y );
//#else
GHWCOLOR ghw_rd_x( GXT x );
//#endif

#ifdef __cplusplus
}
#endif


#endif /* SSD0323_H */
