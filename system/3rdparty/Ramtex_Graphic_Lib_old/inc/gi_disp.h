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

   Version number: 3.12
   Copyright (c) RAMTEX Engineering Aps 1998-2010

*********************************************************************/

#include <gdisp.h> /* GLCD protoypes */

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef GVIEWPORT
/*********************************************************************
   Segment: Viewport
   Level: Viewport
   viewport struct
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
   #ifdef GSOFT_FONTS
   SGUCHAR cursor_on;
   #endif
   #ifndef GCONSTTAB
   GXT tabs[GMAXTABS];   /* Variable tabulator table */
   #endif
   #ifdef GWIDECHAR
   SGUCHAR strtype;      /* String type flag, enables common char / widechar processing */
   #endif
   } GCOMMON_DATA, *PGCOMMON_DATA;

extern GCOMMON_DATA gdata;

/* Pointer to current viewport */
extern GVP GFAST *gcurvp; /* pointer to current viewport struct, instantiated in gvpinit.c */
extern SGUCHAR gcurvpnum; /* Current viewport number */

typedef union
   {
   PGCSTR s;
   #ifdef GWIDECHAR
   PGCWSTR ws;
   #endif
   } GSTRINGPTR;

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
  #define GETCHAR(cp) ((gdata.strtype!=0) ? *(cp.ws) : (GWCHAR)(*(cp.s)))
 #endif
  /* increment either type char pointer or type wide char pointer */
 #define GINCPTR(cp) {if (gdata.strtype!=0) cp.ws++; else cp.s++;}
 #define GETCTRLCHAR(cp) ((gdata.strtype!=0) ? *cp.ws : (GWCHAR)(* (cp.s)))

#else
 /* Fetch logical character from either char string or multibyte string */
 #ifdef GMULTIBYTE
  #define GETCHAR(cp) (gi_mbtowc((PGCSTR *) (&cp.s),(SGUCHAR) gi_usingcodepage()))
 #else
  #define GETCHAR(cp) ((GWCHAR) (* (cp.s)))
 #endif
  /* increment type char pointer */
  #define GINCPTR(cp) cp.s++
  #define GETCTRLCHAR(cp) ((GWCHAR) (*(cp.s)))
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

#else  /* GVIEWPORT */

/* These macros are also used by non viewport features */
#define GINCPTR(cp) cp.s++
#define GETCTRLCHAR(cp) ((GWCHAR) (*(cp.s)))

#endif /* GVIEWPORT */


#ifdef GSOFT_SYMBOLS
/* functions in gsymput.c */
void gi_putsymbol(GXT xs,GYT ys,GXT xemax,GYT yemax, PGSYMBOL psymbol,GYT yoffset, GBUFINT symsize);

/* functions in gsymrotate.c */
extern SGLONG gi_fp_sin;
extern SGLONG gi_fp_cos;
void gi_setfpsincos( float angle );
void gi_symrotate( SGINT x, SGINT y, PGSYMBOL psymbol, SGINT xanchor, SGINT yanchor, GMODE symflag, SGUINT symbytewidth);
SGINT gi_putchw_rotate( GWCHAR ch, SGINT x, SGINT y );
GCOLOR gi_getsym_pixel( SGUINT x, SGUINT y);
SGUCHAR gi_getsym_open( PGSYMBOL psymbol, SGUINT symbytewidth);
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
  extern SGBOOL gi_cursor_on;
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


#ifdef GHW_USING_COLOR
  #define GI_COLOR_CV(col) (col)
#else
  #define GI_COLOR_CV(col) ((col))
#endif

#ifdef __cplusplus
}
#endif

#endif
