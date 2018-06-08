#ifndef GI_DISP_H
#define GI_DISP_H
/************************* gi_disp.h *******************************

   Declarations of internal structures and functions

   Creation date: 980223

   Revision date:
   Revision Purpose:  gi_noupdate inserted in GVP

   Revision date:     020703
   Revision Purpose:  ## removed in GWARNING

   Revision date:     020729
   Revision Purpose:  Library data collected in GCOMMON_DATA structure

   Revision date:     030208
   Revision Purpose:  Defines for extended alginment added
                      Conditional compilation with GCURSOR_SUPPORT

   Revision date:     03-06-06
   Revision Purpose:  Color support added

   Revision date:     040718
   Revision Purpose:  GEXTMODE switch and GVPAPP structure added.


   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp support added
                      gdisp.h include moved outside __cplusplus to get
                      compatability with EC++ template compiler versions

   Revision date:     24-04-07
   Revision Purpose:  gi_sizeofmb(..) gi_mbtowc(..) added

   Revision date:     25-01-08
   Revision Purpose:  Major redesign of gputs gfputch. char, multibyte-char,
                      and wide-char now uses a common processing body. Less ROM size.
                      More viewport format features added.
                      GWORDWRAP compilation switch is replace by mode setting

   Revision date:     18-02-09
                      Functions for symbol rotation and symbol pixel data fetch added.

   Revision date:     31-03-09
   Revision Purpose:  gi_resetvp() added

   Revision date:     06-07-09
   Revision Purpose:  gfputch.c internal function prototypes moved to here

   Revision date:     01-07-10
   Revision Purpose:  gcurvpnum introduced to optimize viewport handling speed.

   Revision date:     11-11-10
   Revision Purpose:  GPOS definition moved to gdisphw.h and made "public"

   Revision date:     14-11-12
   Revision Purpose:  Declaration of internal data structures and Virtual Font
                      definitions moved to gi_disphw.h

   Revision date:     01-12-15
   Revision Purpose:  Declaration of internal GACOLOR data parameter type
                      macros, and values for alpha+color processing and
                      parsing.

   Revision date:    28-12-2015
   Revision Purpose: SGFIXP changed from a typedef to a #define to enable
                     #ifdef .. tests and to match a similar definitions in gfixedpt.h

   Revision date:     14-01-16
   Revision Purpose:  GETCHAR macro for non GMULTIBYTE mode updated with a cast to SGUCHAR
                      Prevent char sign promotion when GWIDECHAR is also defined.

   Revision date:     10-03-16
   Revision Purpose:  Symbolic names for GALPHA opaqua, transperant and mask values added
                      to increase portability.
                      gi_symrotate(..) prototyped moved here to increase resuability.

   Revision date:     13-10-16
   Revision Purpose:  gi_sizestrseg(..) funtion and GSTRMARKS structure prototype
                      made public for potential reuse.
                      (Version Test with GI_DISP_H_VER 319

   Revision date:     05-11-2016
   Revision Purpose:  _GTOI(..) _GTOX(..) _GTOY(..) moved to gdisphw.h to be made public.

   Version number: 3.21
   Copyright (c) RAMTEX International Aps 1998-2016
   Web site, support and upgrade: www.ramtex.dk


*********************************************************************/

#include <gdisp.h>     /* Public GLCD protoypes */
#ifdef __cplusplus
extern "C" {
#endif

/* This internal version */
#define GI_DISP_H_VER 321

/* Define internal parameter type for holding and processing alpha + color info */
#if defined( GHW_USING_COLOR) && (GDISPPIXW >= 16)
   #define GALPHA_OPAQUE    0xff
   #define GALPHA_FULLTRANS 0x00
   #define GALPHAMSK        0xff
   #define GACOLOR      SGULONG
   #define G_ICOLOR_ALPHA(argb) ((SGUCHAR)((argb)>>24)&0xff)
   #define GI_ALPHA_TO_ACOLOR(a)  (((GACOLOR)(a))<<24)
#else
   #define GALPHA_OPAQUE    0xff
   #define GALPHA_FULLTRANS 0x00
   #define GALPHAMSK        0xff
   #define GACOLOR      SGUINT
   #define G_ICOLOR_ALPHA(argb) ((SGUCHAR)((argb)>>8)&GALPHAMSK)
   #define GI_ALPHA_TO_ACOLOR(a)  (((GACOLOR)(a))<<8)
#endif

#define GI_OPAQUE     GI_ALPHA_TO_ACOLOR(GALPHA_OPAQUE)
#define GI_FULLTRANS  GI_ALPHA_TO_ACOLOR(GALPHA_FULLTRANS)

#define GI_IS_FULLTRANS(ic) (((ic) & GI_OPAQUE)==GI_FULLTRANS)  /* is ARGB full transperant */
#define GI_IS_OPAQUE(ic)    (((ic) & GI_OPAQUE)==GI_OPAQUE)     /* is ARGB full opaque */

#ifdef GHW_USING_RGB
  #define GI_CMSK(ic)       ((ic) & (G_RED_MSK|G_GREEN_MSK|G_BLUE_MSK))  /* Reduce ARGB to RGB (before color compare) */
  #define GACOLOR_BLEND( rgbf, af, rgbb, ab) /* Blend rgb parts according to two alpha values */ \
    ((((((GACOLOR)(rgbf) & G_RED_MSK  )*(af) + ((GACOLOR)(rgbb) & G_RED_MSK  )*(ab)) / GALPHAMSK) & G_RED_MSK   )+ \
     (((((GACOLOR)(rgbf) & G_GREEN_MSK)*(af) + ((GACOLOR)(rgbb) & G_GREEN_MSK)*(ab)) / GALPHAMSK) & G_GREEN_MSK )+ \
     (((((GACOLOR)(rgbf) & G_BLUE_MSK )*(af) + ((GACOLOR)(rgbb) & G_BLUE_MSK )*(ab)) / GALPHAMSK) & G_BLUE_MSK  ))
#else
  #define GI_CMSK(ic)       ((ic) & GALPHAMSK)  /* Reduce AGREY to GREY (before color compare) */
  #define GACOLOR_BLEND( gf, af, gb, ab)  /* Blend grey color parts according to two alpha values */ \
     ((((GACOLOR)(gf)&GALPHAMSK)*(af) + ((GACOLOR)(gb)&GALPHAMSK)*(ab)) / GALPHAMSK)
#endif

/* This structure is also used by non-viewport features */
typedef union
   {
   PGCSTR s;          /* Pointer to 1 byte object type */
   #ifdef GWIDECHAR
   PGCWSTR ws;        /* Pointer to compiler specific type (object size >= 2 bytes) */
   #endif
   } GSTRINGPTR;

#ifdef GVIEWPORT

/*********************************************************************

   Storage structures.
*/
typedef struct _GVP
   {
   GPOS lt;             /* viewport in absolute x,y */
   GPOS rb;
   GPOS cpos;           /* Current char pos. in viewport, in pixels, absolute coordinate */
   GPOS ppos;           /* Current pixel pos. in viewport, absolute coordinate */
   PGCODEPAGE codepagep;/* pointer to current codepage */
   PGFONT pfont;        /* pointer to current font */
   GMODE mode;          /* Current graphics mode */
   GPOS  fsize;         /* Current font symbol default size */
   #ifndef GNOTXTSPACE
   GPOS chln;           /* Additional character spacing, line spacing */
   #endif
   #ifdef GHW_USING_COLOR
   GCOLOR foreground;
   GCOLOR background;
   #endif
   #ifdef GDATACHECK
   SGUCHAR gi_check;    /* internal var, for GVP struct check */
   #endif
   #ifdef GEXTMODE
   GVPAPP vpapp;        /* Application defined data assosiated with each viewport */
   #endif
   } GVP, *PGVP;

typedef struct _GCOMMON_DATA
   {
   GVP viewports[GNUMVP]; /* View-port struct that holds state info */
   SGUCHAR curvpnum;      /* Current viewport number (internal parameter for faster ggetvpnum() handling) */
   #ifdef GSOFT_FONTS
   SGUCHAR cursor_on;
   GCURSOR cursor_type;   /* Hardware cursor type */
   #endif
   #ifndef GCONSTTAB
   GXT tabs[GMAXTABS];    /* Variable tabulator table */
   #endif
   #ifdef GWIDECHAR
   SGUCHAR strtype;       /* String type flag, enables common char / widechar processing */
   #endif
   } GCOMMON_DATA, *PGCOMMON_DATA;

extern GCOMMON_DATA gdata;

/* Pointer to current viewport */
extern GVP GFAST *gcurvp; /* pointer to current viewport struct, instantiated in gvpinit.c */

/* Macros for GCOMMON_DATA access */
//#if defined( GSCREENS ) || defined( GHW_ALLOCATE_BUF )
//#else
#ifndef GNOCURSOR
  #define GDATA_CURSORON (gdata.cursor_on)
  #define GDATA_CURSOR   (gdata.cursor_type)
#endif
#define GDATA_VIEWPORTS gdata.viewports
#define GDATA_TABS      gdata.tabs
#define GDATA_STRTYPE   gdata.strtype
#define GDATA_CURVPNUM  gdata.curvpnum
#define GDATA_WIDTH     gdata.width
#define GDATA_HEIGHT    gdata.height
#define GDATA_UPDDELAY  gdata.upddelay
//#endif


#ifdef GVIRTUAL_FONTS
 /* Check for hardware font (Virtual fonts can not be hardware fonts) */
 #define gishwfont() (gisfontv(gcurvp->pfont) ? 0 : ((gcurvp->pfont->psymbols == NULL) ? 1:0))
#else
 #define gishwfont() (gcurvp->pfont->psymbols == NULL)
#endif
#define gi_usingcodepage() ((((PGCODEPAGE)gi_fpcodepage(gcurvp->pfont)) != ((PGCODEPAGE)NULL)) ? 1:0)

#ifdef GMULTIBYTE
/* functions in ggetmbc.c */
SGUCHAR gi_sizeofmb(PGCSTR str);
GWCHAR gi_mbtowc(PGCSTR *strp, SGUCHAR using_codepage);
#endif

#define G_IS_INVERSE()        (((SGUCHAR) gcurvp->mode & (SGUCHAR)GINVERSE) != 0)
#define G_IS_NOSCROLL()       (((SGUCHAR) gcurvp->mode & (SGUCHAR)GNOSCROLL) != 0)
#define G_IS_ALIGN_TOP()      (((SGUCHAR) gcurvp->mode & (SGUCHAR)GALIGN_VCENTER) == (SGUCHAR)GALIGN_TOP)
#define G_IS_ALIGN_BOTTOM()   (((SGUCHAR) gcurvp->mode & (SGUCHAR)GALIGN_VCENTER) == (SGUCHAR)GALIGN_BOTTOM)
#define G_IS_ALIGN_V_CENTER() (((SGUCHAR) gcurvp->mode & (SGUCHAR)GALIGN_VCENTER) == (SGUCHAR)GALIGN_VCENTER)
#define G_IS_ALIGN_LEFT()     (((SGUCHAR) gcurvp->mode & (SGUCHAR)GALIGN_HCENTER) == (SGUCHAR)GALIGN_LEFT)
#define G_IS_ALIGN_RIGHT()    (((SGUCHAR) gcurvp->mode & (SGUCHAR)GALIGN_HCENTER) == (SGUCHAR)GALIGN_RIGHT)
#define G_IS_ALIGN_H_CENTER() (((SGUCHAR) gcurvp->mode & (SGUCHAR)GALIGN_HCENTER) == (SGUCHAR)GALIGN_HCENTER)
#define G_IS_ALIGN_H()        (((SGUCHAR) gcurvp->mode & (SGUCHAR)GALIGN_HCENTER) != 0)
#define G_IS_ALIGN_HR()       (((SGUCHAR) gcurvp->mode & (SGUCHAR)GALIGN_RIGHT) != 0)
#define G_IS_WORDWRAP()       (((SGUCHAR) gcurvp->mode & (SGUCHAR)(GWORD_WRAP|GNO_WRAP)) == (SGUCHAR) GWORD_WRAP)
#define G_IS_NOWRAP()         (((SGUCHAR) gcurvp->mode & (SGUCHAR) GNO_WRAP) != 0)

#define G_IS_VPCLR_UP()       ((gcurvp->mode & GVPCLR_UP)    != 0)
#define G_IS_VPCLR_DOWN()     ((gcurvp->mode & GVPCLR_DOWN)  != 0)
#define G_IS_VPCLR_LEFT()     ((gcurvp->mode & GVPCLR_LEFT)  != 0)
#define G_IS_VPCLR_RIGHT()    ((gcurvp->mode & GVPCLR_RIGHT) != 0)
#define G_IS_TRANSPERANT()    ((gcurvp->mode & GTRANSPERANT) != 0)

#if (!defined( GSOFT_FONTS ))
#define  G_IS_PARTIAL_LINE()    0
#define  G_IS_PARTIAL_CHAR()    0
#define  G_IS_LINECUT()         ((gcurvp->mode & GNO_WRAP) != 0)
#else
#define  G_IS_PARTIAL_LINE()    ((gcurvp->mode & GPARTIAL_LINE) != 0)
#define  G_IS_PARTIAL_CHAR()    ((gcurvp->mode & GPARTIAL_CHAR) != 0)
#define  G_IS_LINECUT()         ((gcurvp->mode & (GNO_WRAP|GPARTIAL_CHAR)) == (GNO_WRAP|GPARTIAL_CHAR))
#endif

#ifdef GWIDECHAR

  /* Fetch logical character from either widechar string, char string or multibyte string*/
 #ifdef GMULTIBYTE
  #define GETCHAR(cp) ((gdata.strtype!=0) ? *(cp.ws) : \
            (gi_mbtowc((PGCSTR *) (&(cp.s)),(SGUCHAR) gi_usingcodepage())))
 #else
  #define GETCHAR(cp) ((gdata.strtype!=0) ? *(cp.ws) : (GWCHAR)((SGUCHAR) *(cp.s)))
 #endif
  /* increment either type char pointer or type wide char pointer */
 #define GINCPTR(cp) {if (gdata.strtype!=0) cp.ws++; else cp.s++;}
 #define GETCTRLCHAR(cp) ((gdata.strtype!=0) ? *cp.ws : (GWCHAR)((SGUCHAR) *(cp.s)))

#else
 /* Fetch logical character from either char string or multibyte string */
 #ifdef GMULTIBYTE
  #define GETCHAR(cp) (gi_mbtowc((PGCSTR *) (&cp.s),(SGUCHAR) gi_usingcodepage()))
 #else
  #define GETCHAR(cp) ((GWCHAR) ((SGUCHAR) *(cp.s)))
 #endif
  /* increment type char pointer */
  #define GINCPTR(cp) cp.s++
  #define GETCTRLCHAR(cp) ((GWCHAR) ((SGUCHAR) *(cp.s)))
#endif

/* functions in gvpinit.c */
#ifdef GDATACHECK
  void gi_datacheck(void);
  void gi_calcdatacheck(void);
#else
  #define gi_datacheck() { /* nothing */ }
  #define gi_calcdatacheck() { /* nothing */ }
#endif

#if (defined( GDATACHECK ) && !defined(GNOTXTSPACE))
  void gi_limit_check(void);
#else
  #define gi_limit_check() { /* nothing */ }
#endif

/* in gvpsel.c */
void gi_selvp( SGUCHAR vp );
/* in gvpinit.c */
void gi_resetvp(SGUCHAR resettype);

/* in gvpset.c */
void gi_setvp_cur_complete(void);

#else  /* GVIEWPORT */

/* These macros are also used by non viewport features */
#define GINCPTR(cp) cp.s++
#define GETCTRLCHAR(cp) ((GWCHAR) (*(cp.s)))

#endif /* GVIEWPORT */

#ifdef GSOFT_SYMBOLS
/* functions in gsymput.c */
void gi_putsymbol(GXT xs,GYT ys,GXT xemax,GYT yemax, PGSYMBOL psymbol,GYT yoffset, GBUFINT symsize);

#ifndef SGFIXP
  /* Type to hold fixed point coordinate values (also prototyped in gfixedpt.h) */
  #define SGFIXP SGLONG
#endif
/* functions in gsymrotate.c */
extern SGFIXP gi_fp_sin;
extern SGFIXP gi_fp_cos;
void gi_setfpsincos( float angle );
void gi_symrotate( SGFIXP x, SGFIXP y,  SGFIXP axo, SGFIXP ayo, GMODE  symflag,  SGUINT symw, SGUINT symh);
SGFIXP gi_putchw_rotate( GWCHAR ch, SGFIXP x, SGFIXP y );


/* functions in gsympixrd.c */
GACOLOR gi_getsym_pixel(SGUINT x, SGUINT y);
SGUCHAR gi_getsym_open( PGSYMBOL psymbol, SGUINT symbytewidth, GMODE symmode);
#endif /* GSOFT_SYMBOLS */

#ifdef GSOFT_FONTS
/* functions in gfsymw.c */
PGSYMBOL gi_getsymbol( GWCHAR c, PGFONT pfont, PGCODEPAGE codepagep );
#endif /* GSOFT_FONTS */

#if defined( GBASIC_TEXT ) || defined( GSOFT_FONTS )
/* Functions in gslen.c */
SGUINT gi_strlen( PGCSTR sp, SGUCHAR mode );
#endif
SGUCHAR gi_strlines( PGCSTR sp ); /* gstrln.c */


#ifndef GNOCURSOR
  /* function in gfcursor.c */
  void gi_cursor( SGBOOL on );
#else
  #define  gi_cursor( on )  { /* Nothing */ }
#endif

/* Functions in gfputch.c (used by gsputs.c) */
SGUCHAR gi_process_newline(GYT lnsp);
void gi_put_prepare(void);
void gi_put_complete(void);
GXT gi_tabstep(GXT x);
#ifndef GHW_NO_HDW_FONT
void gi_xyposalign(void);
#endif
char gi_putch( GWCHAR val );
extern SGBOOL gi_fullheightline;

/* Local struction and functions used in gsputs.c only
  (made public here for potential library extentions) */
typedef struct
   {
   GSTRINGPTR cpbeg;
   GSTRINGPTR cpend;
   GSTRINGPTR cpnext;
   SGUINT slen;
   } GSTRMARKS, *PGSTRMARKS;

SGUCHAR gi_sizestrseg( PGSTRMARKS sm );

#ifdef GGRAPHICS
/* function in ggline.c */
void gi_line( GXT x, GYT y, GXT x2, GYT y2 );
/* function in gcarc.c */
void gi_carc(GXT xc, GYT yc, GXYT r, SGUCHAR arctype);
#endif /* GGRAPHICS */

/************************************************************************
   prints a warning if pos is outside VP and
   moves pos to legal values
*/
#define LIMITTOVP( f, mxs,mys,mxe,mye ) \
   { \
   if( mxs < gcurvp->lt.x ) \
      { \
      G_WARNING( f ": parameter xs<vp.left" ); \
      mxs = gcurvp->lt.x; \
      } \
   if( mxs > gcurvp->rb.x ) \
      { \
      G_WARNING( f ": parameter, xs>vp.right" ); \
      mxs = gcurvp->rb.x; \
      } \
   if( mxe > gcurvp->rb.x ) \
      { \
      G_WARNING( f ": parameter, xe>vp.right" ); \
      mxe = gcurvp->rb.x; \
      } \
   if( mxe < mxs ) \
      { \
      G_WARNING( f ": parameter, xe<xs" ); \
      mxe = mxs; \
      } \
   if( mys < gcurvp->lt.y ) \
      { \
      G_WARNING( f ": parameter, ys<vp.top" ); \
      mys = gcurvp->lt.y; \
      } \
   if( mys > gcurvp->rb.y ) \
      { \
      G_WARNING( f ": parameter, ys>vp.bottom" ); \
      mys = gcurvp->rb.y; \
      } \
   if( mye > gcurvp->rb.y ) \
      { \
      G_WARNING( f ": parameter, ye>vp.bottom" ); \
      mye = gcurvp->rb.y; \
      } \
   if( mye < mys ) \
      { \
      G_WARNING( f ": parameter, ye<ys" ); \
      mye = mys; \
      } \
   }

/* Check and correct vp overflow */
#define GCHECKVP( vp ) \
   { \
   if( (vp) >= GNUMVP ) \
      { \
      G_WARNING( "Viewport number overflow" ); \
      (vp) = GNUMVP-1; \
      } \
   }

/* Wrapper macros for implementing named viewport _vp functions */
#define GSETFUNCVP( VP, FUNC) \
   { \
   SGUCHAR oldvpnum = ggetvpnum(); \
   gi_selvp(( VP )); \
   ( FUNC ); \
   gi_selvp(oldvpnum); \
   }

#define GGETFUNCVP( VP, FUNC) \
   { \
   SGUCHAR oldvpnum = ggetvpnum(); \
   gi_selvp(( VP )); \
   retp = ( FUNC ); \
   gi_selvp(oldvpnum); \
   }

#ifdef __cplusplus
}
#endif

#endif /* GI_DISP_H */
