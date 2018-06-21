#ifndef GDISPHW_H
#define GDISPHW_H
#define memory_hack 1/************************ GDISPHW.H *******************************

   Prototypes and definitions for hardware related functions
   and test messages

   Creation date: 980101

   Revision date:
   Revision Purpose:  Selective support of hardware text fonts
   Revision date:     02-01-23
   Revision Purpose:  symsize parameter added to gi_putsymbol(..)
   Revision date:     020123
   Revision Purpose:  update of invalid-rectancle handling
   Revision date:     020139
   Revision Purpose:  Vertical soft cursor support
   Revision date:     020319
   Revision Purpose:  ghw_invert added
   Revision date:     020326
   Revision Purpose:  ghw_rdsym ghw_wrsym added
   Revision date:     020728
   Revision Purpose:  ghw_setupdate & GHW_ALLOCATED_BUF and PSCREENS functions added
   Revision date:     031204
   Revision Purpose:  Support for PGENERIC i.e. intrinsic generic pointers
   Revision date:     041027
   Revision Purpose   GPALETTE_GREY definition added
   Revision date:     14-04-05
   Revision Purpose:  GHW_NO_LCD_READ_SUPPORT optimization switch added
   Revision date:     260405
   Revision Purpose:  Include of gi_fonts.h added to simlify user handling.
   Revision date:     270405
   Revision Purpose:  The bw parameter to ghw_wrsym(..) and ghw_rdsym(..) changed.
   Revision date:     050505
   Revision Purpose:  Check for the error that gdispcfg.h is a b&W file
   Revision date:     130105
   Revision Purpose:  The GFONT symsize parameter changed to GBUFINT type to
                      to be adaptive to large (color) symbols
   Revision date:     290606
   Revision Purpose:  GHW_TRANSPERANT support added
   Revision date:     090207
   Revision Purpose:  GHW_BLK_SIZE macro corrected for GDISPPIXW > 16
   Revision date:     190407
   Revision Purpose:  GMULTIBYTE_UTF8 support added
   Revision date:     191207
   Revision Purpose:  Use of WCHAR_MAX introduced
   Revision date:     070608
   Revision Purpose:  Virtual fonts introduced
   Revision date:     210708
   Revision Purpose:  Major redesign of gdispcfg.h file layout. Most compilation switch
                      checking moved to this module. Extended switch checking added.
                      GNOTXTSPACE introduced.
   Revision date:     041108
   Revision Purpose:  GDISPHCW introduced to support controllers using multiple pixels
                      in a hardware storage unit larger than 8 bits. Default = GDISPCW
   Revision date:     230209
   Revision Purpose:  RGB format conversion functions added, RGB color masks added
   Revision date:     17-03-09
   Revision Purpose:  Prototypes for ghw_xx read operations enabled when
                      GHW_NO_LCD_READ_SUPPORT and buffered mode
   Revision date:     26-03-09
                      GHW_BLK_SIZE definitions update for more precise size
                      calculation with multibyte video storage units.
   Revision date:     17-04-2009
   Revision Purpose:  Virtual font structures split in RAM dependent and constant
                      data structures (to handle compilers using non-standard C conformant pointers)
   Revision date:     19-07-2009
   Revision Purpose:  GHW_RGB8_PAL switch introduced. For controllers simulating RGB8 mode
   Revision date:     13-01-2010
   Revision Purpose:  GHW_LARGE_VIDEOBUF switch introduced. For controllers supporting view into
                      a larger video buffer than the physical screen
   Revision date:     10-09-10
   Revision Purpose:  IOTester-USB sync flush support added.
   Revision date:     11-11-10
   Revision Purpose:  GPOS definition moved here from gi_disp.h to be "public".
                      G_RGB_TO_COLOR(r,g,b) macro added.
   Revision date:     09-3-11
   Revision Purpose:  Added G_RGB_TO_COLOR(r,g,b) macro convertion to lumisence with grey-level displays.
   Revision date:     15-4-11
   Revision Purpose:  GHW_BLK_SIZE definitions updated. New GSYM_SIZE(w,h) macro
                      gsymsize(..) gsymsize_vp(..) converted to macros defined here
   Revision date:     14-11-12
   Revision Purpose:  GVIRTUAL_FONTS_DYN support added
   Revision date:     11-07-13
   Revision Purpose:  General optimization of ghw_rdsym interface.
   Revision date:     02-02-15
   Revision Purpose:  G_COLOR_TO_INTENSITY(color) macro added to convert a color value
                      to perceptual luminance value, i.e. to a normalized value in the range {0-255}
                      The conversion algorithm tracks the GDISPIXW setting.
   Revision date:     04-01-16
   Revision Purpose:  GXT and GYT is GXYT type when GHW_LARGE_VIDEOBUF is defined
                      GCODEPAGEV is corrected.
   Revision date:     11-02-16
   Revision Purpose:  gsimintf.h PC simulator header always included.
                      Facilitate that PC mode debug info messages are left in the
                      source modules as part of source documentation.
                      When GHW_PCSIM is undefined GSimxxx function calls now just generates
                      no machine code or maps to neutral constants.
   Revision date:     05-11-16
   Revision Purpose:  _GTOI(..) _GTOX(..) _GTOY(..) signed / unsigned convertion macros
                      moved from gi_disph.h to here to be made public.
   Revision date:     13-10-2017
   Revision Purpose:  Macros for individual R,G,B color lane information extraction and
                      conversion added.

   Version number: 4.19
   Copyright (c) RAMTEX International Aps 1998-2017
   Web site, support and upgrade: www.ramtex.dk


*******************************************************************/

#include <stdlib.h>   /* Compiler NULL definition, abs, malloc */
#include <sgtypes.h>  /* STIMGATE type definitions */

#ifdef __cplusplus
extern "C" {
#endif

/* Avoid double definitions. gdispcfg.h must only be included here */

#include <gdispcfg.h> /* Configurations file for LCD driver */

/********* Clean up any conflicting configuration switch settings ********/

/*
    Verify soft switch setup and assure valid combinations
    Basic configuraton setting and switches
      GDISPW
      GDISPH
      GDISPPIXW
      GXT
      GYT
      GHW_USING_RGB
      GHW_USING_GREY_LEVEL

    Application level features
      GVIEWPORT      Enable high-level functions and features (viewports)
      GGRAPHICS      Graphic drawing features (primary switch)
      GSOFT_SYMBOLS  Graphic symbol drawing, like gputsym() (primary switch)
      GSOFT_FONTS    Enable soft font handling (either with text or softsymbols)
      GBASIC_TEXT    Enable text handling (either with soft font or hard fonts)

      GCONSTTAB       No tabulator table generated if defined
      GNOCURSOR       No cursor handling genrated if defined
      GNOTXTSPACE     No extra character or line space handling enabled

      These text string handling features can be used without viewports
      GMULTIBYTE
      GMULTIBYTE_UTF8
      GWIDECHAR
      GBASIC_INITERR

   If GWARNING is defined in gdispcfg.h then diagnostic error messages
   is shown here at compile time if seriously conflicting switches are detected,
   else the conflicting switch setting is just corrected silently
*/

#ifdef GBASIC_INIT_ERR
 #ifndef GCOLOR
  #error Illegal gdispcfg.h file. B&W configuration files can not be used with Color / Gray-scale libs
 #endif

 /* Validate basic display definitions */
 #ifndef GDISPW
  #error Illegal gdispcfg.h file. GDISPW must be defined
 #elif   (GDISPW < 8)
  #error Illegal gdispcfg.h file. GDISPW must be >= 8
 #endif
 #ifndef GDISPH
  #error Illegal gdispcfg.h file. GDISPH must be defined
 #elif   (GDISPH < 8)
  #error Illegal gdispcfg.h file. GDISPH must be >= 8
 #endif
 #ifndef GDISPPIXW
  #error Illegal gdispcfg.h file. GDISPPIXW must be defined
 #elif (GDISPPIXW <= 0)
  #error Illegal gdispcfg.h file. GDISPPIXW must be > 0
 #endif
#endif /* GBASIC_INIT_ERR */

/* Clean up missing type definitions in gdispcfg.h */

/* Optimized type able to hold both a X and Y coordnate types */
#ifndef GXYT
  #if !defined(GDISPW) || !defined(GDISPH) || ((GDISPW >= 256) || (GDISPH >= 256))
     /* Max size, default */
     #define GXYT SGUINT
  #else
     #define GXYT SGUCHAR
  #endif
#endif

#ifdef GHW_LARGE_VIDEOBUF
 /* Set GXT GYT unconditionally to the largest coordinate type
   (as physical screen content may rotate, independent of the logical screen processing) */
 #ifdef GXT
   #undef GXT
 #endif
 #ifdef GYT
   #undef GYT
 #endif
 #define GXT GXYT
 #define GYT GXYT

#else  /* GHW_LARGE_VIDEOBUF */

 /* A variable type used for X and W must be defined */
 #ifndef GXT
   #ifndef GDISPW
    /* = default */
    #define GXT   GXYT
   #elif (GDISPW <= 254)
    #define GXT   SGUCHAR
   #else
    #define GXT   SGUINT
   #endif
 #endif

 /* A variable type used for Y and H must be defined */
 #ifndef GYT
   #ifndef GDISPH
    /* = default */
    #define GYT   GXYT
   #elif (GDISPH <= 254)
    #define GYT   SGUCHAR
   #else
    #define GYT   SGUINT
   #endif
 #endif

#endif /* GHW_LARGE_VIDEOBUF */


#ifndef GBUFINT
 /* A variable type used for buffer and symbol indexing must be defined */
 #define GBUFINT SGULONG
#endif

/* Validate pixel resolution and color settings */
#if (GDISPPIXW < 8)
   #ifdef GHW_USING_RGB
      /* RGB mode require at least 8 bit (default to palette mode) */
      #undef GHW_USING_RGB
   #endif
   #if (GDISPPIXW == 1)
      #ifdef GHW_USING_GREY_LEVEL
         /* Grey mode require at least 2 bits (default to b&w mode) */
         #undef GHW_USING_GREY_LEVEL
      #endif
   #endif
#elif (GDISPPIXW > 8)
   #ifndef GHW_USING_RGB
      /* RGB mode must be used for > 8 bit pr pixel modes */
      #define GHW_USING_RGB
   #endif
#endif

#if ((GDISPPIXW == 8) && defined( GHW_RGB8_PAL ))
   #ifndef GHW_USING_RGB
      /* RGB mode must be used for 8 bit pr pixel RGB palette modes */
      #define GHW_USING_RGB
   #endif
#else
   #ifdef GHW_RGB8_PAL
      /* 8 bit RGB (hardware) palette mode require 8 bit pr pixel mode */
      #error Illegal gdispcfg.h file. GHW_RGB8_PAL require GDISPPIXW defined as 8 bit pr pixel
   #endif
#endif

#if (defined( GHW_USING_RGB ) && defined( GHW_USING_GREY_LEVEL ))
   /* RGB modes takes precedence over grey modes */
   #undef GHW_USING_GREY_LEVEL
#endif

/* Validate GSCREENS */
#ifdef GSCREENS
  #ifndef GVIEWPORT
    #ifdef GWARNING
       #error GVIEWPORT must be defined when GSCREENS is enabled in gdispcfg.h
    #endif
    #define GVIEWPORT
  #endif
  #ifndef GBUFFER
    #define GBUFFER
  #endif
  #ifndef GHW_ALLOCATE_BUF
    #define GHW_ALLOCATE_BUF
  #endif
#else
  #ifdef GHW_ALLOCATE_BUF
    #ifndef GBUFFER
      /* GBUFFER must be defined when GHW_ALLOCATE_BUF is enabled in gdispcfg.h */
      #define GBUFFER
    #endif
  #endif
#endif

/* Validate GGRAPHICS */
#ifdef GGRAPHICS
   #ifndef GVIEWPORT
     #ifdef GWARNING
        #error GVIEWPORT must be defined when GGRAPHICS is enabled in gdispcfg.h
     #endif
     #define GVIEWPORT
   #endif
#endif

/* Validate GSOFT_SYMBOLS */
#ifdef GSOFT_SYMBOLS
   #ifndef GVIEWPORT
     #ifdef GWARNING
        #error GVIEWPORT must be defined when GSOFT_SYMBOLS is enabled in gdispcfg.h
     #endif
     #define GVIEWPORT
   #endif
#endif

/* Validate GVIRTUAL_FONTS, GVIRTUAL_FONTS_DYN */
#ifdef GVIRTUAL_FONTS_DYN
   #ifndef GVIRTUAL_FONTS
      /* GVIRTUAL_FONTS must also be defined when GVIRTUAL_FONTS_DYN is enabled in gdispcfg.h */
      #define GVIRTUAL_FONTS
   #endif
#endif
#ifdef GVIRTUAL_FONTS
  #ifndef GSOFT_FONTS
    #ifdef GWARNING
       #error GSOFT_FONTS must be defined when GVIRTUAL_FONTS is enabled in gdispcfg.h
    #endif
    #define GSOFT_FONTS
  #endif
#endif

/* Validate GSOFT_FONTS */
#ifdef GSOFT_FONTS
   #ifndef GSOFT_SYMBOLS
     #ifdef GWARNING
        #error GSOFT_SYMBOLS must be defined when GSOFT_FONTS is enabled in gdispcfg.h
     #endif
     #define GSOFT_SYMBOLS
     #ifndef GVIEWPORT
       #ifdef GWARNING
          #error GVIEWPORT must be defined when GSOFT_FONTS is enabled in gdispcfg.h
       #endif
       #define GVIEWPORT
     #endif
   #endif
#endif

/* Validate GBASIC_TEXT */
#ifdef GBASIC_TEXT
   #ifndef GVIEWPORT
     #ifdef GWARNING
        #error GVIEWPORT must be defined when GBASIC_TEXT is enabled in gdispcfg.h
     #endif
     #define GVIEWPORT
   #endif
   #ifdef GHW_NO_HDW_FONT
      /* No hardware font so soft font & soft symbols MUST be used */
      #ifndef GSOFT_FONTS
         #ifdef GWARNING
            #error GSOFT_FONTS must be defined when GBASIC_TEXT is enabled in gdispcfg.h and there is no hardware fonts
         #endif
         #define GSOFT_FONTS
      #endif
      #ifndef GSOFT_SYMBOLS
        #ifdef GWARNING
           #error GSOFT_SYMBOLS must be defined when GBASIC_TEXT is enabled in gdispcfg.h and there is no hardware fonts
        #endif
        #define GSOFT_SYMBOLS
      #endif
   #endif
   #ifndef GSOFT_FONTS
      /* Only hardware font is used, no pixel level spacing */
      #ifndef GNOTXTSPACE
         #define GNOTXTSPACE
      #endif
   #endif
   #ifndef GS_ALIGN
      #ifndef GNOTXTSPACE
         #define GNOTXTSPACE
      #endif
   #endif
#else
   /* No text handling, so no need for tabs, cursor control and text alignment */
   #ifndef GCONSTTAB
      #define GCONSTTAB
   #endif
   #ifndef GNOCURSOR
      #define GNOCURSOR
   #endif
   #ifdef GS_ALIGN
      #undef GS_ALIGN
   #endif
   #ifndef GNOTXTSPACE
      #define GNOTXTSPACE
   #endif
#endif

/* Validate GVIEWPORT */
#ifdef GVIEWPORT
   #ifndef GNUMVP
    #ifdef GWARNING
       #error Missing GNUMVP setting in gdispcfg.h
    #endif
    #define GNUMVP 1  /* Assure that at least one view port is defined */
   #else
    #if ((GNUMVP <= 0) || (GNUMVP > 255))
     #ifdef GWARNING
        #error Illegal GNUMVP setting in gdispcfg.h. Must be in range {255:1}
     #endif
     #undef GNUMVP
     #define GNUMVP 1  /* Assure that at least one view port is defined */
    #endif
   #endif
#else
   /* No high level features used */
   #ifdef GNUMVP
     #undef GNUMVP
   #endif
   #define GNUMVP 1  /* Just to avoid compilation errors */
   #ifdef GFUNC_VP
      #undef GFUNC_VP
   #endif
   #ifdef GEXTMODE
      #undef GEXTMODE
   #endif
#endif

#ifndef GBASIC_INIT_ERR
  /* No back light and contrast available if no init */
 #ifdef GHW_INTERNAL_BACKLIGHT
  #undef GHW_INTERNAL_BACKLIGHT
 #endif
 #ifdef GHW_INTERNAL_CONTRAST
  #undef GHW_INTERNAL_CONTRAST
 #endif
#endif

/* Insert default hw character width and height if not allready defined in gdispcfg.h */
#ifndef GDISPCW
  #define GDISPCW 8
#endif
#ifndef GDISPCH
  #define GDISPCH 8
#endif
/* Size of default hardware storage unit (used if GDISPIXW <= size of hardware storage unit and hardware storage unit > 8) */
#ifndef GDISPHCW
  #define GDISPHCW GDISPCW
#endif

/* Size of symbol or hardware color palette (0 = palette not used) */
#ifndef GHW_PALETTE_SIZE
  #define GHW_PALETTE_SIZE 0
#endif

/* Clean up memory type qualifiers.
   Change to defaults in PC simulator mode */
#ifdef GHW_PCSIM
  /* Set defaults for target mode only swirches */
  #ifndef   _WIN32
    #define _WIN32  /* _WIN32 is used by the library */
  #endif
  /* Remove existing definitions (compatibility with old configurations ) */
   #ifdef   GFAST
     #undef GFAST
   #endif
   #ifdef   GVFAST
     #undef GVFAST
   #endif
   #ifdef   GCONSTP
     #undef GCONSTP
   #endif
   #ifdef   GCODE
     #undef GCODE
   #endif
   #ifdef   FCODE
     #undef FCODE
   #endif
   #ifdef   PFCODE
     #undef PFCODE
   #endif
   #ifdef   PGENERIC
     #undef PGENERIC
   #endif

   #define GFAST    /* nothing */
   #define GVFAST   /* nothing */
   #define GCONSTP  const
   #define GCODE    const
   #define FCODE    /* nothing */
   #define PFCODE   /* nothing */
   #define PGENERIC /* nothing */
#else
 /* Undef simulator mode only switches */
 #ifdef GHW_MINIMIZE_CONSOLE
  #undef GHW_MINIMIZE_CONSOLE
 #endif
 #ifdef GHW_FAST_SIM_UPDATE
  #undef GHW_FAST_SIM_UPDATE
 #endif
#endif

/* Clean up incomplete multibyte configuration settings */
#ifdef GMULTIBYTE_UTF8
  #ifndef GMULTIBYTE
    #define GMULTIBYTE
  #endif
#endif

/* Macros for safe conversion to/from signed and unsigned coordinate values */
#define _GTOI(xy) ((SGINT)((SGUINT)(xy)))
#define _GTOX(ix)  ((GXT)((SGUINT)(ix)))
#define _GTOY(iy)  ((GYT)((SGUINT)(iy)))

/* Assure that application level type qualifiers are not undefined */
/* Define generic string data pointers type qualifer (if needed) */
#ifndef PGENERIC
  #define PGENERIC /* nothing */
#endif
/* Define pointer to fixed data or fonts in code memory
  (used if ROM memory type qualifier if is different from const) */
#ifndef PFCODE
  #define PFCODE /* nothing */
#endif
/* Define default const type qualifier used on string pointer types */
#ifndef GCONSTP
  #define GCONSTP const
#endif

/* Define internal (wide) character index types later in this file */
#ifdef  GWCHAR
 #undef GWCHAR
#endif
#ifdef PGCWCHAR
 #undef PGCWCHAR
#endif

/************ End of gdispcfg.h clean up ******************/

/* Viewport mode type (hold combination of mode flags defined in gdisp.h) */
typedef SGUINT GMODE;

/* Pointers used for string (RAM or ROM) data manipulation */
typedef GCONSTP char PGENERIC * PGCSTR;           /* Pointer to constant string (may be in ROM or RAM) */
typedef char PGENERIC * PGSTR;                    /* Pointer to variable string (must be RAM) */
typedef GCONSTP unsigned char PGENERIC * PGCUCHAR;/* Pointer to constant buffer (may be in ROM or RAM) */
typedef unsigned char PGENERIC * PGUCHAR;         /* Pointer to variable buffer (must be RAM) */

#if (defined(GMULTIBYTE) || defined(GWIDECHAR))

 #ifndef G_MALFORMED_RETURNCHAR
   /* Define default value to use as replacement char in case of multi-byte encoding / decoding errors */
   /* (convinient during multibyte text debugging) */
   #define  G_MALFORMED_RETURNCHAR '?'
 #endif

 #ifdef GWIDECHAR

   #ifdef __cplusplus
     }
   #endif
   #include <wchar.h>      /* Get compiler definition of wchar_t and WCHAR_MAX */

   #if defined( _MSC_VER ) || defined(__BORLANDC__)
     /* Fix C99 standard conformance error in MSVC 6.0 and Borland 5 */
     #ifdef swprintf
       #undef swprintf
     #endif
     #define   swprintf   _snwprintf
   #endif

   #ifdef __cplusplus
     extern "C" {
   #endif
   #define GWCHAR wchar_t  /* Use wide char */
 #else
   #define GWCHAR SGUINT   /* Use local wide char definition */
 #endif

 /* WCHAR_MAX = Max-wide-char-limit is defined in the stadard header <wchar.h> */
 #ifndef WCHAR_MAX
   /* Make a default definitions here if GWIDECHAR is not used or if
      the compiler is not in compliance with the C standard */
   #define WCHAR_MAX 0xffff
 #endif

 #ifdef GMULTIBYTE
   #ifndef G_MULTIBYTE_LIMIT
     #define G_MULTIBYTE_LIMIT 0x80  /* Shift multi-byte state if larger or equal */
   #endif
 #endif
#else  /* GMULTIBYTE ||  GWIDECHAR */
  #define GWCHAR SGUCHAR       /* Use narrow char */
  #ifndef WCHAR_MAX
    #define WCHAR_MAX 0xff     /* Max "wide char" limit */
  #endif
#endif /* GMULTIBYTE ||  GWIDECHAR */

typedef GWCHAR PGENERIC * PGWSTR;          /* Pointer to variable widechar string (must be RAM) */
typedef GCONSTP GWCHAR PGENERIC * PGCWSTR;   /* Pointer to constant widechar string (may be in ROM or RAM) */

#ifdef GHW_PCSIM
   #if _WIN32
      void simputs( SGINT sgstream, GCONSTP char *chp );
      void simprintf( SGINT sgstream, GCONSTP char *fmt, ...);
      #pragma warning ( disable : 4761 ) /* remove : "integral size mismatch in argument : conversion supplied" */
      #pragma warning ( disable : 4244 ) /* remove : "converting from 'unsigned int' to 'unsigned char'" caused by integer */
                                         /* promotion rules if aritmetric operations is done in a function call */
      //#pragma pack(1)
      #if _MSC_VER >= 1400
         #pragma warning ( disable : 4996 ) /* remove : "function or variable may be unsafe" for pre C99 C syntax */
      #endif
      #ifndef _CRT_SECURE_NO_WARNINGS
         #define _CRT_SECURE_NO_WARNINGS
      #endif
   #else
   #error Compiler not defined
   #endif

   #ifdef GWARNING
      #define G_WARNING( str ) simputs(0,"WARNING " str " in " __FILE__ )
   #else
      #define G_WARNING( str ) /* nothing */
   #endif
   #ifdef GERROR
      #define G_ERROR( str ) simputs(-1,"ERROR " str " in " __FILE__ )
   #else
      #define G_ERROR( str )   /* nothing */
   #endif

#else /* PC-mode or Target-mode */
      #ifdef _WIN32
         /* 1 byte alignment.  For backward compatibility. Can be removed with the newer full standard conforming compilers */
         #pragma warning ( disable : 4761 ) /* remove : integral size mismatch in argument : conversion supplied */
         #pragma warning ( disable : 4103 ) /* used #pragma pack to change alignment */
         #pragma pack(1)
         #ifndef _CRT_SECURE_NO_WARNINGS
           /* Skip warnings for sprintf(..) test messages etc */
           #define _CRT_SECURE_NO_WARNINGS
         #endif
      #endif

      #ifdef GWARNING
         #ifdef SGWINMSG
         #define G_WARNING( str ) sgprintf(0,"WARNING " str " in " __FILE__ )
         #else
            #ifdef _CONSOLE
               #ifdef __cplusplus
               }
               #endif
               #include <stdio.h>
               #ifdef __cplusplus
               extern "C" {
               #endif
               #define G_WARNING( str ) printf("\nWARNING " str " in " __FILE__ )
            #else
               #define G_WARNING( str ) /* nothing (defined by user) */
            #endif
         #endif
      #else
         #define G_WARNING( str ) /* nothing (defined by user)  */
      #endif

      #ifdef GERROR
         #ifdef SGWINMSG
         #define G_ERROR( str ) sgprintf(-1,"ERROR " str " in " __FILE__ )
         #else
            #ifdef _CONSOLE
               #ifdef __cplusplus
               }
               #endif
               #include <stdio.h>
               #ifdef __cplusplus
               extern "C" {
               #endif
               #define G_ERROR( str ) printf("\nERROR " str " in " __FILE__ )
            #else
               #define G_ERROR( str ) /* nothing  (defined by user) */
            #endif
         #endif
      #else
         #define G_ERROR( str ) /* nothing  (defined by user) */
   #endif
#endif

/*********************************************************************
  Font definitions
*********************************************************************/

/* B&W symbol */
typedef struct _GSYMHEAD /* Symbol header */
   {
   GXT cxpix;       /* Symbol size in num X pixels */
   GYT cypix;       /* Symbol size in num Y pixels */
   } GSYMHEAD;

typedef GCODE GSYMHEAD PFCODE *PGSYMHEAD;

typedef struct _GBWSYMBOL /* One table entry */
   {
   GSYMHEAD sh;     /* Symbol header */
   SGUCHAR  b[3];   /* Symbol data, variable length = (cxpix/8+1)*cypix */
   } GBWSYMBOL;

typedef GCODE GBWSYMBOL PFCODE * PGBWSYMBOL;

/* Color symbol */
typedef struct _GCSYMHEAD /* Symbol header */
   {
   GXT colorid;     /* Color header id (= B&W_x = 0) */
   GYT pbits;       /* Number of bits pr pixel (= color mode) */
   GXT cxpix;       /* Symbol size in num X pixels */
   GYT cypix;       /* Symbol size in num Y pixels */
   } GCSYMHEAD;

typedef GCODE GCSYMHEAD PFCODE * PGCSYMHEAD;

typedef struct _GCSYMBOL /* One table entry */
   {
   GCSYMHEAD sh;    /* Symbol header */
   SGUCHAR  b[3];   /* Symbol data, variable length */
   } GCSYMBOL;

typedef GCODE GCSYMBOL PFCODE * PGCSYMBOL;

#if defined( GVIRTUAL_FONTS ) || defined( GVIRTUAL_FILES ) || defined( GVIRTUAL_FILES_STATIC )
/* Include definition for the GFONTBUFIDX type */
#ifdef __cplusplus
}
#endif
#include <getvmem.h>      /* GFONTBUFIDX type, GFONTDEVICE type */
#ifdef __cplusplus
extern "C" {
#endif
#endif /* GVIRTUAL_FONTS || GVIRTUAL_FILES */

#ifdef GVIRTUAL_FONTS

typedef GCODE void PFCODE * PGVOIDC;

typedef struct _GSYMHEADV_V /* Virtual symbol header (variable part) */
   {
   GYT numbits;            /* Number of bits pr pixel (0=b&w mode, >=1 -> color mode) */
   GXT cxpix;              /* Symbol size in num X pixels */
   GYT cypix;              /* Symbol size in num Y pixels */
   GFONTBUFIDX bidx;       /* Symbol data index in virtual storage */
   GBUFINT symsize;        /* Virtual storage size for font symbol */
   GFONTDEVICE device_id;  /* Virtual font device identifer */
   PGVOIDC symbol_font;    /* Storage for PGFONT pointer for font assosiated with virtual font symbol */
   } GSYMHEADV_V;

typedef GSYMHEADV_V * PGSYMHEADV_V;

typedef struct _GSYMHEADV  /* Virtual symbol header (constant part) */
   {
   GXT id0;                /* virtual symbol identifier = 0 */
   GYT id1;                /* virtual symbol identifier = 0 */
   GXT type_id;            /* extended symbol type id (1 = virtual font symbol) */
   PGSYMHEADV_V psymh_v;   /* pointer to variable part of vf symbol header */
   } GSYMHEADV;

typedef GCODE GSYMHEADV PFCODE * PGSYMHEADV;

#endif

/* Generic symbol header */
typedef union _GSYMBOL /* One table entry */
   {
   GSYMHEAD sh;     /* B&W symbol header */
   GCSYMHEAD csh;   /* Color symbol header */
   #ifdef GVIRTUAL_FONTS
   GSYMHEADV vsh;   /* Virtual symbol header */
   #endif
   } GSYMBOL;

typedef GCODE GSYMBOL PFCODE * PGSYMBOL;
typedef GCODE SGUCHAR PFCODE * PGSYMBYTE; /* Pointer to graphic symbol data */

/*********************************************************************
   Segment: Basic initialization and error handling
   Level: HWDriver
   Codepage, defines a codepage.
*/
typedef struct _GCP_RANGE
   {
   GWCHAR min;  /* Minimum value included in range */
   GWCHAR max;  /* Maximum value included in range */
   GWCHAR idx;  /* Index in symbol table for the first value */
   } GCP_RANGE;

typedef GCODE GCP_RANGE PFCODE * PGCP_RANGE;

typedef struct _GCPHEAD /* codepage header */
   {
   GWCHAR cprnum;    /* Number of GCP_RANGE elements ( >=1) */
   GWCHAR def_wch;   /* Default character used when not found in codepage */
   } GCPHEAD;

typedef GCODE GCPHEAD PFCODE * PGCPHEAD;

typedef struct _GCODEPAGE
   {
   GCPHEAD cph;
   GCP_RANGE cpr[1]; /* Dynamic length. Must contain cprnum elements. Minimum is 1 element */
   } GCODEPAGE;

typedef GCODE GCODEPAGE PFCODE * PGCODEPAGE;

/*********************************************************************
   Segment: Basic initialization and error handling
   Level: HWDriver
   Font struct, defines a font with an array of symbols
*/
typedef struct _GFONT
   {
   GXT        symwidth;   /* default width  (= 0 signals extended font structure) */
   GYT        symheight;  /* default height */
   GBUFINT    symsize;    /* number of bytes in a symbol */
   PGSYMBOL   psymbols;   /* pointer to array of GSYMBOL's (may be NULL) */
   SGUINT     numsym;     /* number of symbols in psymbols (if pcodepage == NULL) */
   PGCODEPAGE pcodepage;  /* pointer to default codepage for font (may be NULL) */
   } GFONT;

typedef GCODE GFONT PFCODE * PGFONT;

#ifdef GVIRTUAL_FONTS
/*********************** Virtual font extentions *****************/

typedef GCP_RANGE  * PGCP_RANGE_V; /* Pointer to codepage range element (variable) */

typedef struct _GCPHEADV    /* codepage header */
   {
   GWCHAR gcpv_id;          /* = 0 = identifier for dynamic codepage structure header */
   GWCHAR type_id;          /* Extended code page structure type (id = 1 for virtual fonts) */
   GWCHAR cprnum;           /* Number of GCP_RANGE elements ( >=1) */
   GWCHAR def_wch;          /* Default character used when not found in codepage */
   GFONTBUFIDX si_codepage; /* Storage index to base of array of GCP_RANGE elements */
   } GCODEPAGEV;

typedef GCODE GCODEPAGEV PFCODE * PGCODEPAGEV;

typedef struct _GFONTV
   {
   GXT         gfontv_id;  /* GFONT default width = 0 -> id for extended font structure */
   GYT         type_id;    /* Extended font structure type (id = 1 for virtual fonts) */
   SGUCHAR     numbits;    /* Symbol number of bits pr pixel (0=b&w mode, >=1 -> color mode) */
   SGUCHAR     reserved;   /* Reserved fill parameter. Type assure structure word alignment) */
   GXT         symwidth;   /* Symbol default width */
   GYT         symheight;  /* Symbol default height */
   SGUCHAR     chsp;       /* Text character pixel spacing (0=normal) */
   SGUCHAR     lnsp;       /* Text line pixel spacing (0=normal) */
   SGUINT      numsym;     /* number of symbols in psymbols (if pcodepage == NULL) */
   GBUFINT     symsize;    /* number of bytes in a symbol */
   PGCODEPAGEV pcodepage;  /* pointer to default codepage for font (may be 0 if no codepage is used) */
   void *      extention;  /* Font parameter extention (reserved for compatibility) (=NULL) */
   GFONTBUFIDX si_symbols; /* storage index in virtual memory to base of array of GSYMBOL's */
   GFONTDEVICE device_id;  /* Virtual font device identifer
                             (application driver specific type defined in getvmem.h) */
   } GFONTV;

typedef GCODE GFONTV PFCODE * PGFONTV;

#ifdef GVIRTUAL_FONTS_DYN

typedef struct _GFONTV_DYN  /* Dynamic (flexible) font lookup descriptor (ROM part) */
   {
   GXT         gfontv_id;  /* GFONT default width = 0 -> id for extended font structure */
   GYT         type_id;    /* Extended font structure type (id = 3 for virtual named fonts (using dynamic font info load to dedicated buffer) */
   GFONTDEVICE device_id;  /* Virtual font device identifer (passed to device loader) */
   PGCSTR      fontname;   /* Pointer to font name string in ROM (used for font location search in virtual image) */
   } GFONTV_DYN;

typedef GCODE GFONTV_DYN PFCODE * PGFONTV_DYN;

typedef struct _GFONTVDYN   /* Dynamic virtual font (RAM part) */
   {
   GXT         gfontv_id;   /* GFONT default width = 0 -> id for extended font structure */
   GYT         type_id;     /* Extended font structure type (id = 2,3 for dynamic virtual fonts) */
   SGUCHAR     numbits;     /* Symbol number of bits pr pixel (0=b&w mode, >=1 -> color mode) */
   SGUCHAR     reserved;    /* Load management flags (0 for ROM storage) */
   GXT         symwidth;    /* Symbol default width */
   GYT         symheight;   /* Symbol default height */
   SGUCHAR     chsp;        /* Text character pixel spacing (0=normal) */
   SGUCHAR     lnsp;        /* Text line pixel spacing (0=normal) */
   SGUINT      numsym;      /* number of symbols in psymbols (if pcodepage == NULL) */
   GBUFINT     symsize;     /* number of bytes in a symbol */
   void *      extention;   /* Font parameter extention (reserved for compatibility) (=NULL) */
   GFONTBUFIDX si_symbols;  /* storage index in virtual memory to base of array of GSYMBOL's */
   GFONTDEVICE device_id;   /* Virtual font device identifer */
   /* Dyn font codepage info (cprnum = 0 -> no codepage) */
   GWCHAR      cprnum;      /* Number of GCP_RANGE elements ( >=1 if codepages are used) */
   GWCHAR      def_wch;     /* Default character used when not found in codepage */
   GFONTBUFIDX si_codepage; /* Storage index to base of array of GCP_RANGE elements */
   } GFONTVDYN;

typedef struct _GFONTV_DYN_BUF  /* Dynamic (flexible) lookup font descriptor (ROM part) with dedicated font buffer (RAM part) */
   {
   GXT         gfontv_id;   /* GFONT default width = 0 -> id for extended font structure */
   GYT         type_id;     /* Extended font structure type (id = 2 for virtual named fonts (using dynamic font info load in shared cashe) */
   GFONTDEVICE device_id;   /* Virtual font device identifer (passed to device loader) */
   PGCSTR      fontname;    /* Pointer to font name string in ROM (used for font location search in virtual image) */
   GFONTVDYN   *fontbuf;    /* Ponter to dynamic loadable font part */
   } GFONTV_DYN_BUF;

typedef GCODE GFONTV_DYN_BUF PFCODE * PGFONTV_DYN_BUF;

#endif /* GVIRTUAL_FONTS_DYN */
#endif /* GVIRTUAL_FONTS */

#ifdef GVIRTUAL_FONTS_DYN
void gfontv_clr( PGFONT *pfont);  /* gfontv.c */
#else
#define gfontv_clr( pfont)        /* nothing */
#endif

/*** Font type classification and font element access macros ***/
#ifdef GVIRTUAL_FONTS
  #ifdef GVIRTUAL_FONTS_DYN
    #define gisfontv_named(pfont)   (((((PGFONTV)(pfont))->type_id   & 0x2)!=0) ? 1 : 0)
    #define gissymbolv_named(psym)  (((((PGSYMHEADV)(psym))->type_id & 0x2)!=0) ? 1 : 0)
    #define giscpv_named(pcp)       (((((PGCODEPAGEV)(pcp))->type_id & 0x2)!=0) ? 1 : 0)
    PGCODEPAGE gi_fpcodepage( PGFONT pfont);  /* gfsel.c */
  #endif  /* GVIRTUAL_FONTS_DYN */

  #define gisfontv(pfont)      (((PGFONT)(pfont)==NULL) ? 0 : (((PGFONT)(pfont))->symwidth == 0))
  #define gisfontcpv(pcp)      ((((PGCODEPAGE)(pcp))->cph.cprnum == 0) ? 1 : 0)
  #define gissymbolv(psym)     ((((PGSYMBOL)(psym))->sh.cypix == 0) ? 1 : 0)

  GXT gsymw(PGSYMBOL psymbol);
  GYT gsymh(PGSYMBOL psymbol);
  SGBOOL giscolor(PGSYMBOL psymbol);
  SGUCHAR gcolorbits(PGSYMBOL psymbol);

  /* (Internal) access macros for font structure elements */
  #ifdef GVIRTUAL_FONTS_DYN
    extern GFONTVDYN *gvf_curfont; /* Pointer to RAM image part for current font */
    #define gi_fsymh(pfont)      ((gisfontv(pfont)==0) ? (((PGFONT) (pfont))->symheight) : ((gisfontv_named(pfont)==0) ? (((PGFONTV)(pfont))->symheight) : (gvf_curfont->symheight)))
    #define gi_fsymw(pfont)      ((gisfontv(pfont)==0) ? (((PGFONT) (pfont))->symwidth)  : ((gisfontv_named(pfont)==0) ? (((PGFONTV)(pfont))->symwidth)  : (gvf_curfont->symwidth)))
    #define gi_fsymsize(pfont)   ((gisfontv(pfont)==0) ? (((PGFONT) (pfont))->symsize)   : ((gisfontv_named(pfont)==0) ? (((PGFONTV)(pfont))->symsize)   : (gvf_curfont->symsize)))
    #define gi_fnumsym(pfont)    ((gisfontv(pfont)==0) ? (((PGFONT) (pfont))->numsym)    : ((gisfontv_named(pfont)==0) ? (((PGFONTV)(pfont))->numsym)    : (gvf_curfont->numsym)))
    #define gvfdevice(pfont)     ((gisfontv(pfont)==0) ? ((GFONTDEVICE)0)                : ((gisfontv_named(pfont)==0) ? (((PGFONTV)(pfont))->device_id) : (gvf_curfont->device_id)))
    #define gi_chsp(pfont)       ((gisfontv_named(pfont)==0) ? (((PGFONTV)(pfont))->chsp) : (gvf_curfont->chsp))
    #define gi_lnsp(pfont)       ((gisfontv_named(pfont)==0) ? (((PGFONTV)(pfont))->lnsp) : (gvf_curfont->lnsp))
  #else /* GVIRTUAL_FONTS_DYN */
    #define gi_fsymh(pfont)      (gisfontv(pfont) ? (((PGFONTV)(pfont))->symheight) : (((PGFONT)(pfont))->symheight))
    #define gi_fsymw(pfont)      (gisfontv(pfont) ? (((PGFONTV)(pfont))->symwidth)  : (((PGFONT)(pfont))->symwidth))
    #define gi_fsymsize(pfont)   (gisfontv(pfont) ? (((PGFONTV)(pfont))->symsize)   : (((PGFONT)(pfont))->symsize))
    #define gi_fnumsym(pfont)    (gisfontv(pfont) ? (((PGFONTV)(pfont))->numsym)    : (((PGFONT)(pfont))->numsym))
    #define gi_fpcodepage(pfont) (gisfontv(pfont) ? (PGCODEPAGE)(((PGFONTV)(pfont))->pcodepage) : (PGCODEPAGE)(((PGFONT)(pfont))->pcodepage))
    #define gvfdevice(pfont)     (gisfontv(pfont) ? (((PGFONTV)(pfont))->device_id) : ((GFONTDEVICE)0))
    #define gi_chsp(pfont)       (((PGFONTV)pfont)->chsp)
    #define gi_lnsp(pfont)       (((PGFONTV)pfont)->lnsp)
  #endif  /* GVIRTUAL_FONTS_DYN */

  #define GVTYPEMASK 0x1F
#else
  /* Only default GFONT structures used */
  #define gsymw(psymbol) (giscolor((psymbol)) ? ((PGCSYMBOL) psymbol)->sh.cxpix : ((PGBWSYMBOL)psymbol)->sh.cxpix)
  #define gsymh(psymbol) (giscolor((psymbol)) ? ((PGCSYMBOL) psymbol)->sh.cypix : ((PGBWSYMBOL)psymbol)->sh.cypix)
  #define giscolor(psymbol)   ((psymbol)->csh.colorid == 0)
  #define gcolorbits(psymbol) ((psymbol)->csh.pbits)

  /* (Internal) access macros for font structure elements */
  #define gi_fsymh(pfont)      ((pfont)->symheight)
  #define gi_fsymw(pfont)      ((pfont)->symwidth)
  #define gi_fpcodepage(pfont) ((pfont)->pcodepage)
  #define gi_fsymsize(pfont)   ((pfont)->symsize)
  #define gi_fnumsym(pfont)    ((pfont)->numsym)
  #define gvfdevice(pfont)     ((GFONTDEVICE)0)
#endif

/* Public access macros for font size info */
#define gfontsize(pfont) (((void*)(pfont)== NULL) ? 0 : gi_fnumsym(pfont))   /* Number of symbols in font */
#define gfsymsize(pfont) (((void*)(pfont)== NULL) ? 0 : gi_fsymsize(pfont))  /* Datasize in number of bytes of each symbol in font */


/*********************************************************************
   Types of cursors
*/
typedef enum _GCURSOR
   {
   GCURSIZE1 = 0x00,
   GCURSIZE2,
   GCURSIZE3,
   GCURSIZE4,
   GCURSIZE5,
   GCURSIZE6,
   GCURSIZE7,
   GCURSIZE8,
   /* and one of these */
   GCURON = 0x10,
   GCURVERTICAL = 0x20,
   GCURBLINK = 0x40,
   GCURBLOCK = 0x80
   } GCURSOR;

/* Palette types */
typedef struct
   {
   SGUCHAR r;
   SGUCHAR g;
   SGUCHAR b;
   } GPALETTE_RGB;

#if (GDISPPIXW <= 8)
 typedef struct
    {
    SGUCHAR gr;
    } GPALETTE_GREY;
#else
 typedef struct
    {
    SGUINT gr;
    } GPALETTE_GREY;
#endif

#define GHW_INVERSE      0x80
#define GHW_GREYMODE     0x40
#define GHW_TRANSPERANT  0x20
#define GHW_PALETTEMASK  (GHW_TRANSPERANT-1)

/* Calculate storage size needed for holding a symbol, ex when fetched with ggetsym(..). (Can be used in array[] declarations) */
#if   (GDISPPIXW == 1)
 #define GSYM_SIZE(w,h) ((GBUFINT)(((((GBUFINT)(w))+7)/8)*((GBUFINT)(h))+sizeof(GSYMHEAD)))
#elif (GDISPPIXW == 2)
 #define GSYM_SIZE(w,h) ((GBUFINT)(((((GBUFINT)(w))+3)/4)*((GBUFINT)(h))+sizeof(GCSYMHEAD)))
#elif (GDISPPIXW <= 4)
 #define GSYM_SIZE(w,h) ((GBUFINT)(((((GBUFINT)(w))+1)/2)*((GBUFINT)(h))+sizeof(GCSYMHEAD)))
#elif (GDISPPIXW <= 8)
 #define GSYM_SIZE(w,h) ((GBUFINT)(((GBUFINT)(w))*((GBUFINT)(h))+sizeof(GCSYMHEAD)))
#elif (GDISPPIXW <= 16)
 #define GSYM_SIZE(w,h) ((GBUFINT)(((GBUFINT)(w))*((GBUFINT)(h))*2+sizeof(GCSYMHEAD)))
#elif (GDISPPIXW <= 24)
 #define GSYM_SIZE(w,h) ((GBUFINT)(((GBUFINT)(w))*((GBUFINT)(h))*3+sizeof(GCSYMHEAD)))
#else
 #define GSYM_SIZE(w,h) ((GBUFINT)(((GBUFINT)(w))*((GBUFINT)(h))*4+sizeof(GCSYMHEAD)))
#endif

/* Old function converted to a macro to be generic */
#define gsymsize(xs,ys,xe,ye) GSYM_SIZE(((xe)-(xs))+1,((ye)-(ys))+1)
/* Old named viewport function converted to macro. Just for backward compatibility, deprecated for new designs */
#define gsymsize_vp(vp,xs,ys,xe,ye) gsymsize(xs,ys,xe,ye)

/*********************************************************************
Font definitions end
*/

#ifdef GBASIC_INIT_ERR

extern GCODE GFONT FCODE SYSFONT; /* instantiated in ghwinit.c */
/* Var to hold error information in LCD driver.
   0 means no error.*/
extern SGBOOL glcd_err;           /* instantiated in ghwinit.c */
/* Vars for text foreground and background colors */
extern GCOLOR ghw_def_foreground; /* instantiated in ghwinit.c */
extern GCOLOR ghw_def_background; /* instantiated in ghwinit.c */

SGBOOL ghw_init(void);
void ghw_puterr( PGCSTR str );
SGUCHAR ghw_err(void);
void ghw_exit(void);
void ghw_dispoff(void);
void ghw_dispon(void);
void ghw_setcolor(GCOLOR fore, GCOLOR back);

void ghw_io_init(void);  /* Located in ghwioini.c */
void ghw_io_exit(void);

#else

#define ghw_init() 1
#define ghw_puterr( str ) { /* Nothing */ }
#define ghw_err() 1
#define ghw_dispoff() { /* Nothing */ }
#define ghw_dispon()  { /* Nothing */ }
#define ghw_setcolor(fore, back) { /* Nothing */ }

#define ghw_io_init();  { /* Nothing */ }  /* Located in ghwioinit.c */
#define ghw_io_exit();  { /* Nothing */ }

#endif /* GBASIC_INIT_ERR */


#ifndef GHW_NO_HDW_FONT
SGBOOL ghw_loadsym( PGSYMBOL psymtab, SGUCHAR nosym, SGUCHAR offset);
#else
#define ghw_loadsym( psymtab, nosym, offset)  1 /* Return error flag */
#endif

#ifdef GHW_INTERNAL_CONTRAST
/* The controller uses contrast regulation on chip */
SGUCHAR ghw_cont_set(SGUCHAR contrast);
SGUCHAR ghw_cont_change(SGCHAR contrast_diff);
#else
#define ghw_cont_set(a)    { /* Nothing */ }
#define ghw_cont_change(a) { /* Nothing */ }
#endif

#ifdef GHW_INTERNAL_BACKLIGHT
void ghw_set_backlight(SGUCHAR level);
#else
#define ghw_set_backlight(a) { /* Nothing */ }
#endif

#if (defined( GBASIC_INIT_ERR ) && (defined(GBUFFER) || !defined( GHW_NO_LCD_READ_SUPPORT )))
void ghw_rdblk( GXT ltx, GYT lty, GXT rbx, GYT rby, SGUCHAR *dest, GBUFINT bufsize );
void ghw_wrblk( GXT ltx, GYT lty, GXT rbx, GYT rby, SGUCHAR *src );
void ghw_restoreblk(SGUCHAR *src);
GBUFINT ghw_blksize( GXT ltx, GYT lty, GXT rbx, GYT rby );
#else
#define ghw_rdblk( ltx, lty, rbx, rby, dest, size ) { /* Nothing */}
#define ghw_wrblk( ltx, lty, rbx, rby, src  ) { /* Nothing */}
#define ghw_restoreblk( src ) { /* Nothing */}
#define ghw_blksize( ltx, lty, rbx, rby ) 0
#endif

#if (defined( GBASIC_INIT_ERR ) && defined( GHW_LARGE_VIDEOBUF ))
/* The controller enables use of a video buffer larger than the screen */
void ghw_set_screenpos(GXT x, GYT y);      /* ghwbuf.c */
void ghw_get_screenpos(GXT *xp, GYT *yp);  /* ghwbuf.c */
#else
#define ghw_set_screenpos(x,y)   { /*Nothing*/ }
#define ghw_get_screenpos(xp,yp) { /*Nothing*/ }
#endif

/*** Functions to be accessed only via highlevel functions *****/


/* gfcursor, gfsel, gvpinit */
#ifndef GNOCURSOR
extern GCURSOR ghw_cursor;        /* instantiated in ghwinit.c */
void ghw_setcursor( GCURSOR type );
#endif

/* gvpinv.c gfcursor.c */
#if (!defined( GNOCURSOR ) && defined (GSOFT_FONTS )) || defined (GGRAPHICS)
#ifndef GHW_NO_LCD_READ_SUPPORT
void ghw_invert(GXT ltx, GYT lty, GXT rbx, GYT rby);
#endif
#endif

/* gfputch.c gvpscroll.c */
#if defined( GBASIC_TEXT ) || defined(GSOFT_FONTS) || defined(GGRAPHIC)
#if (defined(GBUFFER) || !defined( GHW_NO_LCD_READ_SUPPORT ))
void ghw_gscroll( GXT ltx, GYT lty, GXT rbx, GYT rby, GYT lines, SGUINT pattern );
#endif
#endif

#ifdef GVIEWPORT
typedef struct _GPOS
   {
   GXT x;
   GYT y;
   } GPOS;
typedef GPOS PGENERIC * PGPOS;

void ghw_fill( GXT ltx, GYT lty, GXT rbx, GYT rby, SGUINT pattern );
#endif

/* gfputch.c gfsetp.c */
#if !defined( GHW_NO_HDW_FONT ) && defined(GBASIC_TEXT)
void ghw_putch( SGUCHAR idx );
GXT ghw_getcursorxpos(void);
GYT ghw_getcursorypos(void);
void ghw_setcabspos( GXT x, GYT y );
#else
#define ghw_putch( idx )          { /* Nothing */ }
#define ghw_getcursorxpos() 0
#define ghw_getcursorypos() 0
#define ghw_setcabspos( x, y )    { /* Nothing */ }
#endif

#if (defined(GBUFFER) || !defined( GHW_NO_LCD_READ_SUPPORT )) && !defined( GHW_NO_HDW_FONT ) && (defined( GBASIC_TEXT ) || defined(GSOFT_FONTS) || defined(GGRAPHIC))
void ghw_tscroll( GXT ltx, GYT lty, GXT rbx, GYT rby );
#else
#define ghw_tscroll(ltx,lty,rbx,rby ) { /* Nothing */ }
#endif

#ifdef GSOFT_SYMBOLS
#if (defined(GBUFFER) || !defined( GHW_NO_LCD_READ_SUPPORT ))
/* gsymget.c */
SGUCHAR ghw_rdsym(GXT ltx, GYT lty, GXT rbx, GYT rby, PGUCHAR dest);
#endif
/* giputsym.c */
void ghw_wrsym(GXT ltx, GYT lty, GXT rbx, GYT rby, PGSYMBYTE src, SGUINT bw, SGUCHAR mode);
#endif

#ifdef GGRAPHICS
/* ggline.c ggpixel.c gcircle.c */
void ghw_rectangle( GXT ltx, GYT lty, GXT rbx, GYT rby, GCOLOR color );
void ghw_setpixel( GXT x, GYT y, GCOLOR color );
#if (defined(GBUFFER) || !defined( GHW_NO_LCD_READ_SUPPORT ))
/* ggpixel.c */
GCOLOR ghw_getpixel( GXT x, GYT y );
#endif
#endif /* GGRAPHICS */


#if defined( GBASIC_INIT_ERR ) && (GHW_PALETTE_SIZE > 0)
SGBOOL ghw_palette_wr(SGUINT start_index, SGUINT num_elements, GCONSTP GPALETTE_RGB PFCODE *palette);
SGBOOL ghw_palette_rd(SGUINT start_index, SGUINT num_elements, GPALETTE_RGB *palette);
SGBOOL ghw_palette_grey_wr(SGUINT start_index, SGUINT num_elements, GCONSTP GPALETTE_GREY  PFCODE *palette);
SGBOOL ghw_palette_grey_rd(SGUINT start_index, SGUINT num_elements, GPALETTE_GREY *palette);
#else
#define ghw_palette_wr(s, n, p) { /* Nothing */ }
#define ghw_palette_rd(s, n, p) { /* Nothing */ }
#define ghw_palette_grey_wr(s, n, p) { /* Nothing */ }
#define ghw_palette_grey_rd(s, n, p) { /* Nothing */ }
#endif

#ifdef GBASIC_INIT_ERR
GCOLOR ghw_getgray(SGUCHAR graylevel, SGUCHAR greymode);
#if defined( GHW_USING_RGB ) || defined( GHW_USING_GREY_LEVEL )
 #ifdef GHW_USING_GREY_LEVEL
  GCOLOR ghw_color_conv(GCOLOR dat, SGUCHAR dat_bit_pr_pixel);
 #else
  GCOLOR ghw_color_conv(SGULONG dat, SGUCHAR dat_bit_pr_pixel);
 #endif
 GCOLOR ghw_color_blend(GCOLOR fore, GCOLOR back, SGUCHAR alpha);
#else
 #define ghw_color_conv(dat, dat_bit_pr_pixel) 0
 #define ghw_color_blend(fore, back, alpha) 0
#endif

 #ifdef GHW_USING_RGB
   GCOLOR ghw_rgb_to_color( GCONSTP GPALETTE_RGB *palette );
   void ghw_color_to_rgb( SGULONG color, GPALETTE_RGB *palette, SGUCHAR mode );
 #else
   #define ghw_rgb_to_color( palette )  0
   #define ghw_color_to_rgb( color, palette, mode )  { /* Nothing */ }
 #endif

#else  /* GBASIC_INIT_ERR */

 #define ghw_getgray( graylevel, greymode) 0
 #define ghw_rgb_to_color( palette )  0

#endif /* GBASIC_INIT_ERR */

/* RGB color conversion macros */

#ifdef GHW_USING_RGB
 /* Macros for R,G,B color lane conversion to-from individual normalized lane colors */
 #define G_COLOR24_TO_RED_LANE(c)     ((SGUCHAR)((c)>>16)&0xff)   /* To normalized lane color */
 #define G_COLOR24_TO_GREEN_LANE(c)   ((SGUCHAR)((c)>>8)&0xff)
 #define G_COLOR24_TO_BLUE_LANE(c)    ((SGUCHAR)(c)&0xff)
 #define G_COLOR24_FROM_RED_LANE(r)   (((GCOLOR)((r)&0xff))<<16)  /* From normalized lane color */
 #define G_COLOR24_FROM_GREEN_LANE(g) (((GCOLOR)((g)&0xff))<<8)
 #define G_COLOR24_FROM_BLUE_LANE(b)  ((GCOLOR)((b)&0xff))

 #define G_COLOR18_TO_RED_LANE(c)     (((SGUCHAR)((c)>>10)&0xfc) | ((SGUCHAR)((c)>>16)&0x3)) /* Assure full saturation */
 #define G_COLOR18_TO_GREEN_LANE(c)   (((SGUCHAR)((c)>>4)&0xfc)  | ((SGUCHAR)((c)>>10)&0x3))
 #define G_COLOR18_TO_BLUE_LANE(c)    (((SGUCHAR)((c)<<2)&0xfc)  | ((SGUCHAR)((c)>>4) &0x3))
 #define G_COLOR18_FROM_RED_LANE(r)   (((GCOLOR)((r)&0xfc))<<16) /* To color lane position and resolution */
 #define G_COLOR18_FROM_GREEN_LANE(g) (((GCOLOR)((g)&0xfc))<<8)
 #define G_COLOR18_FROM_BLUE_LANE(b)  (((GCOLOR)((b)&0xfc))>>2)

 #define G_COLOR16_TO_RED_LANE(c)     (((SGUCHAR)((c)>>8)&0xf8) | ((SGUCHAR)((c)>>13)&0x7)) /* Assure full saturation */
 #define G_COLOR16_TO_GREEN_LANE(c)   (((SGUCHAR)((c)>>3)&0xfc) | ((SGUCHAR)((c)>>8) &0x3))
 #define G_COLOR16_TO_BLUE_LANE(c)    (((SGUCHAR)((c)<<3)&0xf8) | ((SGUCHAR)((c)>>2) &0x7))
 #define G_COLOR16_FROM_RED_LANE(r)   (((GCOLOR)((r)&0xf8))<<8)  /* To color lane position and resolution */
 #define G_COLOR16_FROM_GREEN_LANE(g) (((GCOLOR)((g)&0xfc))<<3)
 #define G_COLOR16_FROM_BLUE_LANE(b)  (((GCOLOR)((b)&0xf8))>>3)

 #define G_COLOR8_TO_RED_LANE(c)      (((SGUCHAR)((c)&0xe0))   |((SGUCHAR)((c)&0xe0)>>3)|((SGUCHAR)((c)&0xe0)>>6)) /* Assure full saturation */
 #define G_COLOR8_TO_GREEN_LANE(c)    (((SGUCHAR)((c)&0x1c)<<3)|((SGUCHAR)((c)&0x1c))   |((SGUCHAR)((c)&0x1c)>>3))
 #define G_COLOR8_TO_BLUE_LANE(c)     (((SGUCHAR)((c)&0x03)<<6)|((SGUCHAR)((c)&0x03)<<4)|((SGUCHAR)((c)&0x03)<<2)|((SGUCHAR)(c)&0x03))
 #define G_COLOR8_FROM_RED_LANE(r)    (((GCOLOR)((r)&0xe0)))       /* To color lane position and resolution */
 #define G_COLOR8_FROM_GREEN_LANE(g)  (((GCOLOR)((g)&0xe0))>>3)
 #define G_COLOR8_FROM_BLUE_LANE(b)   (((GCOLOR)((b)&0xc0))>>6)

 #if   (GDISPPIXW >= 24)
   #define G_RED_MSK   0xff0000
   #define G_GREEN_MSK 0x00ff00
   #define G_BLUE_MSK  0x0000ff
   #if (GDISPPIXW == 24)
      #define G_RGB_TO_COLOR(r,g,b) ((((GCOLOR)(r)&0xff)<<16)|(((GCOLOR)(g)&0xff)<<8)|((GCOLOR)(b)&0xff))
   #else
      #define G_RGB_TO_COLOR(r,g,b) ((((GCOLOR)(r)&0xff)<<16)|(((GCOLOR)(g)&0xff)<<8)|((GCOLOR)(b)&0xff)|((GCOLOR)0xff000000))
   #endif
   #define G_COLOR_TO_INTENSITY(c) (((((SGUINT)(((c)>>16)&0xff))*76) + (((SGUINT)(((c)>>8)&0xff))*150)+((SGUINT)((c)&0xff))*30)/256)
   /* Macros for R,G,B color conversion to-from individual normalized lane colors */
   #define G_COLOR_TO_RED_LANE(c)     G_COLOR24_TO_RED_LANE(c)
   #define G_COLOR_TO_GREEN_LANE(c)   G_COLOR24_TO_GREEN_LANE(c)
   #define G_COLOR_TO_BLUE_LANE(c)    G_COLOR24_TO_BLUE_LANE(c)
   #define G_COLOR_FROM_RED_LANE(r)   G_COLOR24_FROM_RED_LANE(r)
   #define G_COLOR_FROM_GREEN_LANE(g) G_COLOR24_FROM_GREEN_LANE(g)
   #define G_COLOR_FROM_BLUE_LANE(b)  G_COLOR24_FROM_BLUE_LANE(b)
 #elif (GDISPPIXW == 18)
   #define G_RED_MSK   0x03f000
   #define G_GREEN_MSK 0x000fc0
   #define G_BLUE_MSK  0x00003f
   #define G_RGB_TO_COLOR(r,g,b) ((((GCOLOR)(r)&0xfc)<<10)|(((GCOLOR)(g)&0xfc)<<4)|(((GCOLOR)(b)&0xff)>>2))
   #define G_COLOR_TO_INTENSITY(c) (SGUCHAR)((((SGULONG)((((SGUINT)(((c)>>12)&0x3f))*76) + (((SGUINT)(((c)>>6)&0x3f))*150)+(((SGUINT)((c)&0x3f))*30)))*255)/16128)
   /* Macros for R,G,B color conversion to-from individual normalized lane colors */
   #define G_COLOR_TO_RED_LANE(c)     G_COLOR18_TO_RED_LANE(c)
   #define G_COLOR_TO_GREEN_LANE(c)   G_COLOR18_TO_GREEN_LANE(c)
   #define G_COLOR_TO_BLUE_LANE(c)    G_COLOR18_TO_BLUE_LANE(c)
   #define G_COLOR_FROM_RED_LANE(r)   G_COLOR18_FROM_RED_LANE(r)
   #define G_COLOR_FROM_GREEN_LANE(g) G_COLOR18_FROM_GREEN_LANE(g)
   #define G_COLOR_FROM_BLUE_LANE(b)  G_COLOR18_FROM_BLUE_LANE(b)
 #elif (GDISPPIXW == 16)
   #define G_RED_MSK   0xf800
   #define G_GREEN_MSK 0x07e0
   #define G_BLUE_MSK  0x001f
   #define G_RGB_TO_COLOR(r,g,b) ((((GCOLOR)(r)&0xf8)<<8)|(((GCOLOR)(g)&0xfc)<<3)|(((GCOLOR)(b)&0xff)>>3))
   #define G_COLOR_TO_INTENSITY(c) (SGUCHAR)((((SGULONG)((((SGUINT)(((c)>>11)&0x1f))*76) + (((SGUINT)(((c)>>5)&0x3f))*(150/2))+(((SGUINT)((c)&0x1f))*30)))*255)/8011)
   /* Macros for R,G,B color conversion to-from individual normalized lane colors */
   #define G_COLOR_TO_RED_LANE(c)     G_COLOR16_TO_RED_LANE(c)
   #define G_COLOR_TO_GREEN_LANE(c)   G_COLOR16_TO_GREEN_LANE(c)
   #define G_COLOR_TO_BLUE_LANE(c)    G_COLOR16_TO_BLUE_LANE(c)
   #define G_COLOR_FROM_RED_LANE(r)   G_COLOR16_FROM_RED_LANE(r)
   #define G_COLOR_FROM_GREEN_LANE(g) G_COLOR16_FROM_GREEN_LANE(g)
   #define G_COLOR_FROM_BLUE_LANE(b)  G_COLOR16_FROM_BLUE_LANE(b)
 #elif (GDISPPIXW == 8)
   #define G_RED_MSK    0xe0
   #define G_GREEN_MSK  0x1c
   #define G_BLUE_MSK   0x03
   #define G_RGB_TO_COLOR(r,g,b) (((GCOLOR)(r)&0xe0)|(((GCOLOR)(g)>>3)&0x1c)|(((GCOLOR)(b)&0xff)>>6))
   #define G_COLOR_TO_INTENSITY(c) (SGUCHAR)((((SGULONG)((((SGUINT)(((c)>>5)&0x7))*(76/2)) + (((SGUINT)(((c)>>2)&0x7))*(150/2))+(((SGUINT)((c)&0x3))*30)))*255)/881)
   /* Macros for R,G,B color conversion to-from individual normalized lane colors */
   #define G_COLOR_TO_RED_LANE(c)     G_COLOR8_TO_RED_LANE(c)
   #define G_COLOR_TO_GREEN_LANE(c)   G_COLOR8_TO_GREEN_LANE(c)
   #define G_COLOR_TO_BLUE_LANE(c)    G_COLOR8_TO_BLUE_LANE(c)
   #define G_COLOR_FROM_RED_LANE(r)   G_COLOR8_FROM_RED_LANE(r)
   #define G_COLOR_FROM_GREEN_LANE(g) G_COLOR8_FROM_GREEN_LANE(g)
   #define G_COLOR_FROM_BLUE_LANE(b)  G_COLOR8_FROM_BLUE_LANE(b)
   #endif
#else
 /* RGB values to grey level lumisence */
 #define G_RGB_TO_COLOR(r,g,b) (((((SGUINT)(r))*76) + (((SGUINT)(g))*150)+((SGUINT)(b))*30)>>(16-GDISPPIXW))
 /* Grey intensity = normalized grey value */
 #define G_COLOR_TO_INTENSITY(c) (SGUCHAR)((((SGUINT)(c))*255)/((1<<GDISPPIXW)-1))
#endif


/* Stop C declarations */
#ifdef __cplusplus
   }
#endif
/* Add simulator definitions (or map PC mode simulator debug functions to "nothing") */
#include <gsimintf.h>
/* Restart C declarations */
#ifdef __cplusplus
   extern "C" {
#endif

#if (defined(_WIN32) && defined(IOTESTER_USB))
   /* Stop C declarations */
   #ifdef __cplusplus
     }
   #endif
   #include <iotester.h>  /* iot_sync() prototype */
   /* Restart C declarations */
   #ifdef __cplusplus
     extern "C" {
   #endif
#endif

typedef enum _GUPDATE
   {
   GUPDATE_OFF,  /* Activate delayed update */
   GUPDATE_ON    /* De-activate delayed update, draw pending content */
   } GUPDATE;

#ifdef GBUFFER
  /* Buffered mode */
  void ghw_updatehw(void);
  GUPDATE ghw_setupdate( GUPDATE update );
#else
  #if (defined(_WIN32) && defined(GHW_PCSIM))
     /* Map LCD updates to Simulator flush only */
     #define ghw_updatehw()   GSimFlush()
     #define ghw_setupdate(u) GSimFlush()
  #elif (defined(_WIN32) && defined(IOTESTER_USB))
     #define ghw_updatehw()   iot_sync(IOT_SYNC);
     #define ghw_setupdate(a) { /* Nothing */ }
  #else
     /* Neither buffer, nor simulator */
     #define ghw_updatehw()   { /* Nothing */ }
     #define ghw_setupdate(a) { /* Nothing */ }
  #endif
#endif

#ifdef GHW_ACCELERATOR

void ghw_acc_waitdone(void);
void ghw_acc_rectangle(GXT ltx, GYT lty, GXT rbx, GYT rby, GCOLOR color, SGUCHAR fill);
void ghw_acc_copy(GXT ltx, GYT lty, GXT rbx, GYT rby, GXT new_ltx, GYT new_lty);
#ifdef GHW_ACC_SUPPORT_CIRCLE
void ghw_acc_circle(GXT cenx, GYT ceny, SGUINT rad, SGUCHAR fill, GCOLOR color);
#endif
#ifdef GHW_ACC_SUPPORT_LINE
void ghw_acc_line(GXT ltx, GYT lty, GXT rbx, GYT rby, GCOLOR color);
#endif

#endif /* GHW_ACCELERATOR */

/* Compile time calculation of ghw_rdblk(..) buffer size
   Note: The optimum buffer size depends on both the size of video storage units,
   and, if GDISPPIXW < 8, the orientation of video storage units and the how well
   corner cordinates are aligned with pixel storage units.
*/
#if defined( GHW_USING_VBYTE_HORPIX )
   /* Vertical buffer storage unit (byte) containing multiple pixels, horizontal pixel data over multiple bytes */
   #define GHW_BLK_SIZE(ltx,lty,rbx,rby) \
   ((((GBUFINT)(rby)/GDISPCW) - ((GBUFINT)(lty)/GDISPCW) + 1) * \
    ((GBUFINT)((rbx) - (ltx)) + 1) * ((GBUFINT)GDISPPIXW) + \
    ((GBUFINT)GDISPPIXW) + \
    ((GBUFINT)2*sizeof(GXT)) + ((GBUFINT)2*sizeof(GYT)))
#elif defined( GHW_USING_HBYTE_VERPIX )
   /* Horizontal buffer storage unit (byte) containing multiple pixels, vertical pixel data over multiple bytes */
   #define GHW_BLK_SIZE(ltx,lty,rbx,rby) \
   ((((GBUFINT)(rbx)/GDISPCW) - ((GBUFINT)(ltx)/GDISPCW) + 1) * \
   ((GBUFINT)((rby) - (lty)) + 1) * ((GBUFINT)GDISPPIXW) + \
   ((GBUFINT)GDISPPIXW) + \
   ((GBUFINT)2*sizeof(GXT)) + ((GBUFINT)2*sizeof(GYT)))
#elif defined( GHW_USING_VBYTE )
   /* Vertical buffer storage unit (byte) containing multiple pixels, vertical pixel data within buffer storage unit */
   #define GHW_BLK_SIZE(ltx,lty,rbx,rby) \
      (((GBUFINT)((rby)/(GDISPHCW/GDISPPIXW)) - ((GBUFINT)(lty)/(GDISPHCW/GDISPPIXW)) + 1) * \
      ((GBUFINT)(GDISPHCW/8)) * ((GBUFINT)((rbx) - (ltx)) + 1) + \
      (GDISPHCW/8)*2 + ((GBUFINT)2*sizeof(GXT)) + ((GBUFINT)2*sizeof(GYT)))
#elif (GDISPPIXW < 8)
   /* Horizontal buffer storage unit (byte) containing multiple pixels, horizontal pixel data within buffer storage unit */
   #define GHW_BLK_SIZE(ltx,lty,rbx,rby) \
     (((GBUFINT)((rbx)/(GDISPHCW/GDISPPIXW)) - ((GBUFINT)(ltx)/(GDISPHCW/GDISPPIXW)) + 1) * \
     ((GBUFINT)(GDISPHCW/8)) * ((GBUFINT)((rby) - (lty)) + 1) + \
      (GDISPHCW/8)*2 + ((GBUFINT)2*sizeof(GXT)) + ((GBUFINT)2*sizeof(GYT)))
#else
    /* One or more horizontal bytes. Pixel data over (multiple succeding) byte(s) */
    /* i.e. 8,16,24,32 bit pixel data */
    #define GHW_BLK_SIZE(ltx,lty,rbx,rby) \
      ((((GBUFINT)((rbx) - (ltx))+1)*sizeof(GCOLOR)) * ((GBUFINT)((rby) - (lty)) + 1) + \
       (GDISPHCW/8) + \
       ((GBUFINT)2*sizeof(GXT)) + ((GBUFINT) 2*sizeof(GYT)))
#endif

#if (defined( GHW_ALLOCATE_BUF ) && defined( GBUFFER ))
  GBUFINT ghw_gbufsize( void );

  #ifdef GSCREENS
    SGUCHAR ghw_is_owner( SGUCHAR *buf );
    SGUCHAR *ghw_save_state( SGUCHAR *buf );
    void ghw_set_state(SGUCHAR *buf, SGUCHAR doinit);
  #else
    #define ghw_is_owner( a ) 0
    #define ghw_save_state(a) NULL
    #define ghw_set_state(a,b) { /* Nothing */ }
  #endif /* GSCREENS */

#else
  #define ghw_gbufsize() 0
  #define ghw_is_owner( a ) 0
  #define ghw_save_state(a) NULL
  #define ghw_set_state(a,b)   { /* Nothing */ }
#endif

/* Internal range check / correction macros */
#define GLIMITU(x,u) \
   { \
   if ((x) > (u)) (x)=(u);\
   }

#define GLIMITD(x,d) \
   { \
   if ((x) < (d)) (x)=(d);\
   }

/* Fast / low overhead clear of RAM based object type */
#define G_OBJ_CLR( obj ) \
   {  \
   register unsigned char *p = (unsigned char*)((void *)(&(obj)));  \
   register unsigned int count = sizeof(obj);  \
   while (count-- > 0) *p++ = 0; \
   }

#define G_POBJ_CLR( pobj, objtype ) \
   {  \
   register unsigned char *p = (unsigned char*)((void *)(pobj));  \
   register unsigned int count = sizeof(objtype);  \
   while (count-- > 0) *p++ = 0; \
   }

/* Fast in-line copy between two blocks with size = objsize */
#define G_POBJ_CPY( ptoobj, pfromobj, objsize ) \
   {  \
   register unsigned int _objsize = (unsigned int)(objsize);\
   if (_objsize-- > 0) \
      { \
      register char *_ptoobj   = (char*)((void *)(ptoobj));     \
      register char *_pfromobj = (char*)((void *)(pfromobj));   \
      do \
         { \
         _ptoobj[_objsize] = _pfromobj[_objsize]; \
         } \
      while( _objsize-- != 0); \
      } \
   }


/* Stop C declarations */
#ifdef __cplusplus
   }
#endif

/* Include prototypes for standard fonts */
#include <gi_fonts.h>

#endif /* GDISPHW_H */



