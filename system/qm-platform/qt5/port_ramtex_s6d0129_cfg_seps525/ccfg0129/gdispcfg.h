#ifndef GDISPCFG_H
#define GDISPCFG_H

#include "sgtypes.h"

/******************* gdispcfg.h *****************************

   CONFIGURATION FILE FOR THE GRAPHIC DISPLAY LIBRARY
   This header file define the driver feature set used by your
   application and the abstract display hardware properties.

   As a programmer you set these definements in order to adjust
   the display driver code to the needs of your application.
   In many cases the definements in this file is used to remove,
   modify, or replace sections of the underlying library source code.

   This header is included by the Graphic display library files.
   It should normally not be included by the user application.

   Revision data:    080808
   Revision Purpose: Configuration header simplified
                     Virtual font support added.

   Version number: 1.1
   Copyright (c) RAMTEX Engineering Aps 2006-2008

************************************************************/

/* Size of display module in pixels */
#define GDISPW 160     /* Width */
#define GDISPH 128     /* Height */

#define GHW_SEPS525

/* Define display controller bus size (select ony one).
   This must match the basic chip configuration after chip hardware reset */
  #define GHW_BUS8     /* 8 bit databus */
/*#define GHW_BUS16*/  /* 16 bit databus */

/* Define number of bits pr pixel for data storage (select only one) */
   #define GDISPPIXW 16
/* #define GDISPPIXW 24 */ /* 24 bit RGB mode (of which 18 bits are used by controller ) */

/* Define number of view-ports supported,
   See the function SGUCHAR gselvp( SGUCHAR s );
   At least 1 view-port must be defined */
#define GNUMVP 5

/* Feature optimization compilation keywords */
#define GBASIC_INIT_ERR      /* Enable Basic initalization and error handling */
#define GVIEWPORT            /* Enable high-level (viewport) functions */
#define GGRAPHICS            /* Enable Graphics */
#define GSOFT_SYMBOLS        /* Enable Software symbols */
#define GSOFT_FONTS          /* Enable Soft fonts */
/*#define GVIRTUAL_FONTS*/   /* Enable virtual font support */
#define GBASIC_TEXT          /* Enable Basic text */
#define GS_ALIGN             /* Enable extended string alignment */
/*#define GMULTIBYTE */      /* Enable multibyte support */
/*#define GMULTIBYTE_UTF8 */ /* Enable UTF-8 multibyte support */
/*#define GWIDECHAR */       /* Enable wide-char support */
/*#define GFUNC_VP */        /* Enable named viewport functions xxx_vp()*/
/*#define GSCREENS*/         /* Enable screens */
/*#define GEXTMODE*/         /* Enable application specific viewport data extentions */
                             /* (viewport data extensions are defined in gvpapp.h) */
#define GNOCURSOR            /* Turn visual cursor handling off or on*/
                             /* Define for max speed, undefine to have cursor support */
#define GNOTXTSPACE          /* Turn extra character or line space handling off and on */
                             /* Define for max speed, undefine to have line and character spacing */

/* Tabulator definitions */
#define GCONSTTAB            /* Tab table contain constants (undefine to use variable tabs) */

#ifdef  GCONSTTAB
 /* Define const tab increments in number of pixel */
 #define GTABSIZE  (GDISPW/6)
#else
 /* variable tab tabel is used */
 #define GMAXTABS 10           /* Max. number of tab positions (defines tabulator tabel size) */
#endif

/* Define value for switch between normal and multi-byte string chars
   If the char is above this value then this char plus the next char is
   used to form a 16 bit wide char (ex in the range 0x8000 to 0xffff) */
#define G_MULTIBYTE_LIMIT 0x80

/* Select buffered implementation (speed optimization with
   external display RAM buffer or use direct operation on
   module RAM. Define or undefine GBUFFER */
#if BUILD_OPTION_ENABLE_DISPLAY_BUFFER == 1
  #define GBUFFER /* Extern buffer for data manipulation, fast */
#endif

/* If GHW_ALLOCATE_BUF is defined the graphich buffer is allocated using malloc.
   instead of using a (faster) static buffer */
#ifdef GBUFFER
  /* #define GHW_ALLOCATE_BUF */ /* Allocate buffer on heap */
#endif

/* If GWARNING is defined, illegal runtime values will cause
   issue of a display message and stop of the system.
   The soft error handler function G_WARNING(str) defined in
   gdisphw.h is used for message output.
   If undefined parameters will be forced within a legal range
   and used afterwards. */
#define GWARNING

/* If GERROR is defined, states and situations which may result
   in a fatal runtime state will cause a display message to be
   issued and the system stopped. The soft error handler function
   G_ERROR(str) defined in gdisphw.h is used for message output.
   If undefined the situation is ignored or an exit is performed. */
#define GERROR

/* If GDATACHECK is defined the internal data is checked
   for errors. Maybe some faulty part of the main code overwrites
   the internal data of the LCD driver, such and error will be
   catched with this define. Undefine for max speed. */
#define GDATACHECK

/* Variable used for X and W */
#if (GDISPW <= 255)
#define GXT   unsigned char
#else
#define GXT   unsigned short
#endif
/* Variable used for Y and H */
#if (GDISPH <= 255)
#define GYT   unsigned char
#else
#define GYT   unsigned short
#endif

#ifdef GHW_PCSIM
   /* Simulator mode only switches */
   /* Define to minimize a console application when the LCD simulator is used */
   #define GHW_MINIMIZE_CONSOLE
   /* Define to limit simulator updates to the highlevel functions.
      The simulator operations is faster when defined */
   #define GHW_FAST_SIM_UPDATE
#endif

#ifndef GHW_PCSIM
   /* Memory type qualifiers only for target compilation mode (dont care in PC simulation mode) */
   /* The following definements allow you to optimize the parameter passing and to use target compiler specific
      memory allocation keywords (default will be used for PC simulator mode) */

   /* Keyword used for fast variables optimization (critical params)*/
   #define GFAST  /* nothing */
   /* Keyword used for very fast variables optimization (critical params)*/
   #define GVFAST /* nothing */
   /* Type qualifier used on pointer (parameter) to strings when the object
      is not modified by the function (part of C strong proto typing */
   #define GCONSTP const
   /* type qualifier used for fixed data (graphic tables etc) */
   #define GCODE  const
   /* Memory type qualifier used for fixed data (if GCODE setting is not enough) */
   #define FCODE  /* nothing */
   /* Memory type qualifier used for pointer to fixed data (if GCODE var * is not enough) */
   #define PFCODE /* nothing */
   /* Keyword used for generic pointers to data strings, if generic pointers is not default */
   #define PGENERIC /* nothing */
#endif

/** Color buffer handling types **/

/* Define type to hold color information for a pixel */
#if   (GDISPPIXW <= 8)
  #define GCOLOR  SGUCHAR
#elif (GDISPPIXW <= 16)
  #define GCOLOR  SGUINT
#else
  #define GCOLOR  SGULONG
#endif

/* Define integer optimized for buffer indexing and buffer size values */
#if (((GDISPPIXW > 8) && ((GDISPW*GDISPH*4) > 0xffff)) || \
     ((GDISPW * GDISPH *((GDISPPIXW+7)/8)) > 0xffff))
  #define GBUFINT SGULONG
#else
  #define GBUFINT SGUINT
#endif

#define  GHW_PALETTE_SIZE 16 /* Size of software palette */

/****************** LOW-LEVEL DRIVER CONFIGURATIONS *******************/

/* Adapt library to scan line layout selected by display module vendor */
/* #define GHW_MIRROR_VER */ /* Mirror the display vertically */
/* #define GHW_MIRROR_HOR */ /* Mirror the display horizontally */
  #define GHW_XOFFSET  0    /* Set display x start offset in on-chip video ram */
  #define GHW_YOFFSET  0    /* Set display y start offset in on-chip video ram */
/*#define  GHW_ROTATED */   /* Define to rotate display 90 (270) degrees (remember to swap values used in GDISPH,GDISPW definitions) */


/* Defined as the controller has contrast regulation on chip (do not modify) */
#define GHW_INTERNAL_CONTRAST

/* The chip does not support hardware or download fonts (do not modify) */
#define GHW_NO_HDW_FONT

/****************** COLOR DEFINITION *******************/
/* Enable code generation for color and gray-shade support */
#define GHW_USING_COLOR
#define GHW_INVERTGRAPHIC_SYM  /* Define to accept symbols created with using 0 as black */
#define GHW_USING_RGB    /* RGB color mode is used (do not modify) */

/* Define the default colors used for text foreground and background
  G_BLACK, G_RED ,G_GREEN, G_YELLOW, G_BLUE,
  G_MAGENTA, G_CYAN, G_WHITE, G_LIGHTGRAY, or G_DARKGRAY
  Note that the number of possible colors is limited by both on the
  palette mode and the display screen type
*/
#define GHW_PALETTE_BACKGROUND  G_WHITE
#define GHW_PALETTE_FOREGROUND  G_BLACK

/*
   Map "pure color names" to palette indexes.
   These definitions should mach the default palette file (*.pal) and
   should be updated if the color ordering in the palette files is
   changed.
   If a direct match is not possible (ex because of a small palette)
   then the closest color match should be defined.
   For gray scale display these definitions provide a "translation"
   from color to gray-shade
*/
#if (GDISPPIXW <= 16)

   #define G_BLACK       0x0000
   #define G_RED         0xf800
   #define G_GREEN       0x07e0
   #define G_YELLOW      0xffe0
   #define G_BLUE        0x001f
   #define G_MAGENTA     0xf81f
   #define G_CYAN        0x07ff
   #define G_WHITE       0xffff
   /* Names for extended colors */
   #define G_LLIGHTGREY  G_RGB_TO_COLOR(90,90,90)//0xd6ba
   #define G_LIGHTGREY   G_RGB_TO_COLOR(50,50,50)//0x7bcf
   #define G_GREY        0xad75
   #define G_DARKGREY    0x94b2
   #define G_DDARKGREY   0x7bef
   #define G_DARKBLUE    0x0018
   #define G_ORANGE      0xfc00
   #define G_OLHA_DBLUE  G_RGB_TO_COLOR(114,159,220)
   #define G_BLUE_LIGHT	 0x8d9c

#elif (GDISPPIXW <= 24)

   #define G_BLACK       0x000000
   #define G_RED         0xff0000
   #define G_GREEN       0x00ff00
   #define G_YELLOW      0xffff00
   #define G_BLUE        0x0000ff
   #define G_MAGENTA     0xff00ff
   #define G_CYAN        0x00ffff
   #define G_WHITE       0xffffff
   /* Names for extended colors */
   #define G_LLIGHTGREY  0xd0d0d0
   #define G_LIGHTGREY   0xb8b8b8
   #define G_GREY        0xa8a8a8
   #define G_DARKGREY    0x909090
   #define G_DDARKGREY   0x787878
   #define G_DARKBLUE    0x0000c0
   #define G_ORANGE      0xff8000

#endif
/* End of gdispcfg.h */
#endif
