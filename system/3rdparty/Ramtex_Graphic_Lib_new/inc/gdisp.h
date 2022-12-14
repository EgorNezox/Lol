#ifndef GDISP_H
#define GDISP_H
/************************* GDISP.H *********************************

   Prototype file for RAMTEX graphic LCD display driver.

   The struct & functions in this file makes the
   user interface to the driver.

   To start using the driver call ginit(); that sets up the Display.

   Creation date:  11-11-2004

   Revision date:    130504
   Revision Purpose: gvpxl(..),gvpxr(..),gvpyt(..),gvpyb(..) added.
   Revision date:    050504
   Revision Purpose: gmstrlen(..),gmsstrlen(..),gmstrcpy(..),gmstrcat(..) added
   Revision date:    180704
   Revision Purpose: GEXTMODE switch and GVPAPP structure added.
   Revision date:    13-08-04
   Revision Purpose: Named viewport functions _vp added
   Revision date:    12-12-04
   Revision Purpose: GNOSCROLL, GALIGN_TOP, GALIGN_BOTTOM, GALIGN_LEFT added.
   Revision date:    20-01-05
   Revision Purpose: gsetupcpy added, gsetmode_vp corrected.
   Revision date:    14-04-05
   Revision Purpose: GHW_NO_LCD_READ_SUPPORT optimization switch added
   Revision date:    02-06-05
   Revision Purpose: GHW_USING_COLOR switch added. Facilitate automatic
                     converting of color applications to b&w applciation
                     mode.
   Revision date:    13-06-06
   Revision Purpose: gsetpos_vp parameter types changed to GXT,GYT
   Revision date:    290606
   Revision Purpose: GTRANSPERANT mode support added
   Revision date:    090207
   Revision Purpose: ggetmbc(..),gwctomb(..),gwstrcpymb(..),
                     gmbstrcpyw(..),gwcmbsize(..),gmstrlenw(..) added
   Revision date:    25-01-08
   Revision Purpose: More viewport format features added.
                     GWORDWRAP compilation switch is replace by mode setting
   Revision date:    15-06-08
   Revision Purpose: Rearanged for new conditional compilation feature layout
   Revision date:    15-06-09
   Revision Purpose: gputch_vp() now a macro mapping to gputchw_vp
   Revision date:    18-10-09
   Revision Purpose: GMODE changed from enum type to SGUINT + bit mask
                     definitions. Solve enum limitations with some compilers.
   Revision date:    09-09-10
   Revision Purpose: Corected missing groundrect_vp(..) gcarc_vp() prototype
                     when GHW_USING_COLOR defined.
   Revision date:    14-02-11
   Revision Purpose: gputchrot_vp(..) macro corrected
   Revision date:    12-07-11
   Revision Purpose: gputfsym(..), gputfsym_vp(..) functions added
   Revision date:    15-04-2012
   Revision Purpose: gsymsize(..) gsymsize_vp(..) definitions moved to gdisphw.h
   Revision date:    03-07-2012
   Revision Purpose: gfillfsym(..) gfillfsym_vp(..) added
   Revision date:    10-03-2016
   Revision Purpose: gputfsymrot(..), gputfsymrot_vp(..),  added
   Revision date:    16-03-2016
   Revision Purpose: gresetinitvp(..), gresetinitvp_vp(..),  added
   Revision date:    16-06-2016
   Revision Purpose: gputswrot(..) prototype corrected.


   Version number: 4.9
   Copyright (c) RAMTEX Engineering Aps 1998-2016

*********************************************************************/

#include <gdisphw.h>  /* HW driver prototypes */

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************/

/* Viewport mode type GMODE defined in gdisphw.h holds combination of mode flags below */

/* Mode flags */
/* Version <=3 + 4 types */
#define GNORMAL        0x00 /* Normal mode (not inversed not aligned no partial showing) */
#define GINVERSE       0x01 /* Inverse (typical white on black) */
#define GNOSCROLL      0x02 /* Scroll at viewport end is suppressed */
#define GALIGN_LEFT    0x04 /* String is aligned to the left viewport edge */
#define GALIGN_RIGHT   0x08 /* String is aligned to the right viewport edge */
#define GALIGN_HCENTER 0x0c /* String is centered horizontally */
#define GALIGN_TOP     0x10 /* String is aligned to the top */
#define GALIGN_BOTTOM  0x20 /* String is aligned to the bottom */
#define GALIGN_VCENTER 0x30 /* String is centered vertically */

/* Version >= 4 types */
#define GWORD_WRAP     0x40 /* Use word wrap (default is char wrap) */
#define GNO_WRAP       0x80 /* Use no character or word wrapping (only \n processing) */
#define GPARTIAL_LINE 0x100 /* Show lines where symbol height is partly visible  */
#define GPARTIAL_CHAR 0x200 /* Show characters where symbol width can only be partly visible */
#define GVPCLR_UP    0x0400 /* Clear vp area above text line(s) */
#define GVPCLR_DOWN  0x0800 /* Clear vp area below text line(s) */
#define GVPCLR_LEFT  0x1000 /* Clear vp area to the left of text line(s) */ /* default console mode */
#define GVPCLR_RIGHT 0x2000 /* Clear vp area to the right of text line(s) */
#define GTRANSPERANT 0x8000 /* Symbol background color is transperant color */
#define GMODELAST    0xffff /* Used by internal mode limit checks */

/* Compatibility with old source codes */
#define  GLINECUT ( GNO_WRAP | GPARTIAL_CHAR )
#define  GVPCLR   (GVPCLR_UP | GVPCLR_DOWN | GVPCLR_LEFT | GVPCLR_RIGHT)

/* Mode flags for symbol and text rotation modes (overlaids normal mode flags) */
#define GSYMCLR        GVPCLR_UP
#define GNOCOLORBLEND  GVPCLR_DOWN


#ifdef GBASIC_TEXT
SGUCHAR ggetcxpos(void);        /* gfgetcxp.c */
SGUCHAR ggetcypos(void);        /* gfgetcyp.c */
void gputchw( GWCHAR val );     /* gfputch.c */
#define gputch( val ) gputchw( (GWCHAR) ((SGUCHAR) (val)) )
void gsetcpos( SGUCHAR xpos, SGUCHAR ypos );    /* gfsetcp.c */
void gputs( PGCSTR str );       /* gsputs.c */
#else
#define ggetcxpos() 0
#define ggetcypos() 0
#define gputchw( val ) {/* Nothing */}
#define gsetcpos( xpos, ypos ) {/* Nothing */}
#define gputs( str ) {/* Nothing */}
#define gputch( ch ) {/* Nothing */}
#endif /* GBASIC_TEXT */

#if defined( GBASIC_TEXT ) && defined( GWIDECHAR )
void gputsw( PGCWSTR str );     /* gsputs.c */
#else
#define gputsw( str )  {/* Nothing */}
#endif /* GBASIC_TEXT && GWIDECHAR */

#if defined( GBASIC_TEXT ) && defined( GFUNC_VP )
SGUCHAR ggetcxpos_vp(SGUCHAR vp);          /* gfgetcxp.c */
SGUCHAR ggetcypos_vp(SGUCHAR vp);          /* gfgetcyp.c */
void gputchw_vp( SGUCHAR vp, GWCHAR val ); /* gfputch.c */
#define gputch_vp( vp, val ) gputchw_vp( vp, (GWCHAR) ((SGUCHAR) (val)) )
void gsetcpos_vp( SGUCHAR vp, SGUCHAR xpos, SGUCHAR ypos );     /* gfsetcp.c */
void gputs_vp( SGUCHAR vp, PGCSTR str );   /* gsputs.c */
#else
#define ggetcxpos_vp( vp ) 0
#define ggetcypos_vp( vp ) 0
#define gputchw_vp( vp,  val ) {/* Nothing */}
#define gputch_vp( vp,  val )  {/* Nothing */}
#define gsetcpos_vp( vp,  xpos, ypos ) {/* Nothing */}
#define gputs_vp( vp,  str )  {/* Nothing */}
#endif /* GBASIC_TEXT && GFUNC_VP */

#if defined( GBASIC_TEXT ) && defined( GWIDECHAR ) && defined( GWIDECHAR )
void gputsw_vp( SGUCHAR vp, PGCWSTR str ); /* gsputs.c */
#else
#define gputsw_vp( vp,  str )  {/* Nothing */}
#endif /* GWIDECHAR */

#ifndef GNOCURSOR
GCURSOR gi_setcursor( GCURSOR type );
#define gsetcursor(type) gi_setcursor( (GCURSOR) (type) )
GXT gcursor_width( void );      /* gfcursor.c */
GYT gcursor_height( void );     /* gfcursor.c */
#ifdef GSOFT_FONTS
void gcursorblink(void);        /* gfcursor.c */
#else
#define gcursorblink()    {/* Nothing */}
#endif
#else
#define gsetcursor( type ) {/* Nothing */}
#define gcursor_width()   GDISPCW
#define gcursor_height()  GDISPCH
#define gcursorblink()    {/* Nothing */}
#endif /* !GNOCURSOR */

#ifndef GCONSTTAB
void gclrtabs(void);    /* gftabs.c */
void gsettab( GXT s );  /* gftabs.c */
void gsettabs( GXT s ); /* gftabs.c */
#else
#define gclrtabs()  {/* nothing */}
#define gsettabs(s) {/* nothing */}
#define gsettab(s)  {/* nothing */}
#endif /* !GCONSTTAB */

#ifndef GNOTXTSPACE
GXT ggetspch(void);      /* gchlnsp.c */
GYT ggetspln(void);      /* gchlnsp.c */
void gsetspch(GXT chsp); /* gchlnsp.c */
void gsetspln(GYT lnsp); /* gchlnsp.c */
#else
#define ggetspch() 0
#define ggetspln() 0
#define gsetspch(chsp) {/* nothing */}
#define gsetspln(lnsp) {/* nothing */}
#endif /* !GNOTXTSPACE */

#if !defined( GNOTXTSPACE ) && defined( GFUNC_VP )
GXT ggetspch_vp(SGUCHAR vp);            /* gchlnsp.c */
GYT ggetspln_vp(SGUCHAR vp);            /* gchlnsp.c */
void gsetspch_vp(SGUCHAR vp, GXT chsp); /* gchlnsp.c */
void gsetspln_vp(SGUCHAR vp, GYT lnsp); /* gchlnsp.c */
#else
#define ggetspch_vp(vp) 0
#define ggetspln_vp(vp) 0
#define gsetspch_vp(vp,chsp) {/* nothing */}
#define gsetspln_vp(vp,lnsp) {/* nothing */}
#endif /* !GNOTXTSPACE && GFUNC_VP */

#ifndef PI
  /* Define PI for rotation angle calculations */
  #define  PI ((float)3.1415926535)
#endif
/* Convert degree to radians  (radian = degree*PI/180) */
#define  G_DEGREE_TO_RADIAN(deg) (((float)(deg))*(PI/((float)180)))

#if (defined( GBASIC_TEXT ) || defined( GSOFT_FONTS))
GYT gfgetfh( PGFONT fp );        /* gfgeth.c */
GYT ggetfh(void);                /* gfgeth.c */
GYT ggetfh_vp(SGUCHAR vp);       /* gfgeth.c */
PGFONT ggetfont( void );         /* gfsel.c */
PGFONT gselfont( PGFONT pfont ); /* gfsel.c */
GXT gfgetfw( PGFONT fp );        /* gfgetw.c */
GXT ggetfw(void);                /* gfgetw.c */
GXT ggetfw_vp(SGUCHAR vp);       /* gfgetw.c */
SGUINT gpstrheight( PGCSTR str );/* gpstrh.c */
SGUINT gpstrlen( PGCSTR str );   /* gslen.c */
SGUINT gpstrwidth( PGCSTR str ); /* gslen.c */
SGUINT gpwordlen( PGCSTR str );  /* gslen.c */
PGFONT ggetfont_vp( SGUCHAR vp );/* gfsel.c */
void gputsrot( PGCSTR str, float angle); /* gputsrot.c */
void gputchwrot( GWCHAR ch, float angle); /* gputchrot.c */
#define gputchrot( ch, angle) gputchwrot((GWCHAR)(ch),(angle))
#else
#define gfgetfh( fp ) 8
#define ggetfh() 8
#define ggetfh_vp() 8
#define ggetfont()  NULL
#define gselfont( pfont ) NULL
#define ggetfw() 8
#define gfgetfw( fp ) 8
#define gfgetfw_vp( fp ) 8
#define gpstrheight( str ) 0
#define gpstrlen( str ) 0
#define gpstrwidth( str ) 0
#define gpwordlen( str ) 0
#define ggetfont_vp( ch )  NULL
#define gputsrot( str, angle) {/* Nothing */ }
#define gputchwrot( ch, angle) {/* Nothing */ }
#define gputchrot( ch, angle) {/* Nothing */ }
#endif /* GBASIC_TEXT | GSOFT_FONTS */

#if ((defined( GBASIC_TEXT ) || defined( GSOFT_FONTS)) && defined( GWIDECHAR ))
SGUINT gpstrheightw( PGCWSTR str ); /* gpstrh.c */
SGUINT gpstrlenw( PGCWSTR str );    /* gslen.c */
SGUINT gpstrwidthw( PGCWSTR str );  /* gslen.c */
SGUINT gpwordlenw( PGCWSTR str );   /* gslen.c */
void gputswrot( PGCWSTR str, float angle); /* gputsrot.c */
#else
#define gpstrheightw( str ) 0
#define gpstrlenw( str ) 0
#define gpstrwidthw( str ) 0
#define gpwordlenw( str ) 0
#define gputswrot( str, angle) {/* Nothing */ }
#endif

#if ((defined( GBASIC_TEXT ) || defined( GSOFT_FONTS)) && defined( GFUNC_VP ))
PGFONT gselfont_vp( SGUCHAR vp, PGFONT pfont );  /* gfsel.c */
SGUINT gpstrheight_vp( SGUCHAR vp, PGCSTR str ); /* gpstrh.c */
SGUINT gpstrlen_vp( SGUCHAR vp, PGCSTR str );    /* gslen.c */
SGUINT gpstrwidth_vp( SGUCHAR vp, PGCSTR str );  /* gslen.c */
SGUINT gpwordlen_vp( SGUCHAR vp, PGCSTR str );   /* gslen.c */
void gputsrot_vp( SGUCHAR vp, PGCSTR str, float angle ); /* gputsrot.c */
void gputchwrot_vp( SGUCHAR vp, GWCHAR ch, float angle); /* gputchrot.c */
#define gputchrot_vp( vp, ch, angle) gputchwrot_vp((vp),(GWCHAR)(ch),(angle))
#else
#define gselfont_vp( vp,  pfont ) { /* nothing */ }
#define gpstrheight_vp( vp,  str ) 0
#define gpstrlen_vp( vp,  str ) 0
#define gpstrwidth_vp( vp,  str ) 0
#define gpwordlen_vp( vp,  str ) 0
#define gputsrot_vp( vp, str, angle)  {/* Nothing */ }
#define gputchwrot_vp( vp, ch, angle) {/* Nothing */ }
#define gputchrot_vp( vp, ch, angle)  {/* Nothing */ }
#endif /* (GBASIC_TEXT | GSOFT_FONTS) && GFUNC_VP */

#if ((defined( GBASIC_TEXT ) || defined( GSOFT_FONTS)) && defined( GFUNC_VP ) && defined( GWIDECHAR ))
SGUINT gpstrwidthw_vp( SGUCHAR vp, PGCWSTR str );  /* gslen.c */
SGUINT gpstrlenw_vp( SGUCHAR vp, PGCWSTR str );    /* gslen.c */
SGUINT gpstrheightw_vp( SGUCHAR vp, PGCWSTR str ); /* gpstrh.c */
SGUINT gpwordlenw_vp( SGUCHAR vp, PGCWSTR str );   /* gslen.c */
void gputswrot_vp( SGUCHAR vp, PGCWSTR str, float angle ); /* gputsrot.c */
#else
#define gpstrheightw_vp( ch,  str ) 0
#define gpstrlenw_vp( ch,  str ) 0
#define gpstrwidthw_vp( ch,  str ) 0
#define gpwordlenw_vp( ch,  str ) 0
#define gputswrot_vp( ch, str, angle) {/* Nothing */ }
#endif /* (GBASIC_TEXT | GSOFT_FONTS) && GFUNC_VP */

#if (defined( GBASIC_TEXT ) || defined( GGRAPHICS ))
void gsetpos( GXT xpos, GYT ypos );     /* gfsetp.c */
#else
#define gsetpos( xpos, ypos ) {/* Nothing */}
#endif /* GBASIC_TEXT | GGRAPHICS */

#if ((defined( GBASIC_TEXT ) || defined( GGRAPHICS )) && defined( GFUNC_VP ))
void gsetpos_vp( SGUCHAR vp, GXT xpos, GYT ypos ); /* gfsetp.c */
#else
#define gsetpos_vp( ch,  xpos, ypos ) {/* Nothing */}
#endif /* (GBASIC_TEXT | GGRAPHICS) &  GFUNC_VP */

#if ((defined( GBASIC_TEXT ) || defined( GSOFT_FONTS ) || defined( GGRAPHICS )) && \
     (!defined( GHW_NO_LCD_READ_SUPPORT ) || defined(GBUFFER)))
void gscrollcvp(void);  /* gvpscrol.c */
#else
#define gscrollcvp() {/* Nothing */}
#endif /* GBASIC_TEXT | GSOFT_FONTS | GGRAPHICS */
#if ((defined( GBASIC_TEXT ) || defined( GSOFT_FONTS ) || defined( GGRAPHICS )) && defined( GFUNC_VP ))
void gscrollcvp_vp( SGUCHAR vp );       /* gvpscrol.c */
#else
#define gscrollcvp_vp( ch ) {/* Nothing */}
#endif /* GBASIC_TEXT | GSOFT_FONTS | GGRAPHICS */

/* gcarc() and groundedarc() flag parameters */
#define  GLINE     0x00  /* Negated enable, 0 = default = GLINE active */
#define  GFILL     0x03
#define  GFRAME    0x02
#define  GCARC_LT  0x10
#define  GCARC_LB  0x20
#define  GCARC_RT  0x40
#define  GCARC_RB  0x80
#define  GCARC_ALL  (GCARC_LT|GCARC_LB|GCARC_RT|GCARC_RB)

#ifdef GGRAPHICS
void gcircle(SGINT xc, SGINT yc, SGINT r, SGUCHAR fill); /* ggcircle.c */
GXT ggetxpos(void);     /* gggetxyp.c */
GYT ggetypos(void);     /* gggetxyp.c */
void glineto( GXT xe, GYT ye ); /* ggline.c */
void gmoveto( GXT xs, GYT ys ); /* ggline.c */
void gsetpixel( GXT xs, GYT ys, SGBOOL pixel ); /* ggpixel.c */
void grectangle( GXT xs, GYT ys, GXT xe, GYT ye );      /* ggrect.c */
void gfillvp( GXT xs, GYT ys, GXT xe, GYT ye, SGUINT f );       /* gvpfill.c */
#ifdef GHW_USING_COLOR
void gcrectangle( GXT xs, GYT ys, GXT xe, GYT ye, GCOLOR linecolor );
void gcsetpixel( GXT xs, GYT ys, GCOLOR pixel );
#else
#define gcsetpixel( xs, ys, pixelcolor ) gsetpixel((xs),(ys), ((pixelcolor) & GINVERSE) ? 0 : 1)
#define gcrectangle( xs, ys, xe, ye, linecolor )  \
   { \
   GMODE mode = gsetcolorf((linecolor)); \
   grectangle( (xs), (ys), (xe), (ye) ); \
   gsetmode(mode); \
   }
#endif
void groundrect( GXT ltx, GYT lty, GXT rbx, GYT rby, GXYT r, SGUCHAR skipflags);  /* groundrect.c */
void gcarc(GXT xc, GYT yc, GXYT r, SGUCHAR arctype); /* gcarc.c */
#else
#define gcircle(xc, yc, r, fill )  {/* Nothing */}
#define ggetxpos() 0
#define ggetypos() 0
#define glineto( xs, ys ) {/* Nothing */}
#define gmoveto( xe, ye ) {/* Nothing */}
#define gsetpixel( xs, ys, pixel ) {/* Nothing */}
#define grectangle( xs, ys, xe, ye ) {/* Nothing */}
#define gfillvp( xs, ys, xe, ye, f ) {/* Nothing */}
#define gcsetpixel( xs, ys, pixelcolor ) {/* Nothing */}
#define gcrectangle( xs, ys, xe, ye, linecolor ) {/* Nothing */}
#define groundrect( ltx, lty, rbx, rby, r, skipflags) {/* Nothing */}
#define gcarc( xc, yc, r, arctype) {/* Nothing */}

#endif /* GGRAPHICS */

#if defined( GGRAPHICS ) && defined( GFUNC_VP )
void gcircle_vp( SGUCHAR vp, SGINT xc, SGINT yc, SGINT r, SGUCHAR fill );       /* ggcircle.c */
GXT ggetxpos_vp( SGUCHAR vp );  /* gggetxyp.c */
GYT ggetypos_vp( SGUCHAR vp );  /* gggetxyp.c */
void glineto_vp( SGUCHAR vp, GXT xe, GYT ye );  /* ggline.c */
void gmoveto_vp( SGUCHAR vp, GXT xs, GYT ys );  /* ggline.c */
void gsetpixel_vp( SGUCHAR vp, GXT xs, GYT ys, SGBOOL pixel );  /* ggpixel.c */
void grectangle_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye );       /* ggrect.c */
void gfillvp_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye, SGUINT f );        /* gvpfill.c */
#ifdef GHW_USING_COLOR
void gcsetpixel_vp( SGUCHAR vp, GXT xs, GYT ys, GCOLOR pixelcolor );
void gcrectangle_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye, GCOLOR linecolor );
#else
/* No color lib support, convert color to GNORMAL/GINVERT setting */
#define gcsetpixel_vp( vp, xs, ys, pixelcolor ) gsetpixel_vp((vp),(xs),(ys), ((pixelcolor) & GINVERSE) ? 0 : 1)
#define gcrectangle_vp( vp, xs, ys, xe, ye, linecolor )  \
   { \
   GMODE mode = gsetcolorf_vp(vp, (linecolor)); \
   grectangle_vp( (vp), (xs), (ys), (xe), (ye) ); \
   gsetmode_vp(vp, mode); \
   }
#endif /* GHW_USING_COLOR */
void groundrect_vp( SGUCHAR vp, GXT ltx, GYT lty, GXT rbx, GYT rby, GXYT r, SGUCHAR skipflags);  /* groundrect.c */
void gcarc_vp( SGUCHAR vp, GXT xc, GYT yc, GXYT r, SGUCHAR arctype); /* gcarc.c */
#else
#define gcircle_vp( ch, xc, yc, r, fill )  {/* Nothing */}
#define ggetxpos_vp( ch ) 0
#define ggetypos_vp( ch ) 0
#define glineto_vp( ch,  xs, ys ) {/* Nothing */}
#define gmoveto_vp( ch,  xe, ye ) {/* Nothing */}
#define gsetpixel_vp( ch,  xs, ys, pixel ) {/* Nothing */}
#define grectangle_vp( ch,  xs, ys, xe, ye ) {/* Nothing */}
#define gfillvp_vp( ch,  xs, ys, xe, ye, f ) {/* Nothing */}
#define gcsetpixel_vp( ch, xs, ys, pixelcolor ) {/* Nothing */}
#define gcrectangle_vp( ch, xs, ys, xe, ye, linecolor ) {/* Nothing */}
#define groundrect_vp( ch, ltx, lty, rbx, rby, r, skipflags) {/* Nothing */}
#define gcarc_vp( ch, xc, yc, r, arctype) {/* Nothing */}
#endif /* GGRAPHICS && GFUNC_VP */

#if defined( GGRAPHICS ) && !defined( GHW_NO_LCD_READ_SUPPORT )
void ginvertvp( GXT xs, GYT ys, GXT xe, GYT ye); /* gvpinv.c */
SGBOOL ggetpixel( GXT xs, GYT ys );     /* ggpixel.c */
#ifdef GHW_USING_COLOR
 GCOLOR gi_cgetpixel( GXT xs, GYT ys );
 #define gcgetpixel( xs, ys ) gi_cgetpixel( (xs), (ys) )
#else
 #define gcgetpixel( xs, ys ) ggetpixel( (xs), (ys) )
#endif
#else
#define ginvertvp( xs, ys, xe, ye )  {/* Nothing */}
#define ggetpixel( xs, ys ) 0
#define gcgetpixel( xs, ys ) 0
#endif /* GGRAPHICS && ! GHW_NO_LCD_READ_SUPPORT */

#if defined( GGRAPHICS )  && defined( GFUNC_VP ) && !defined( GHW_NO_LCD_READ_SUPPORT )
void ginvertvp_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye); /* gvpinv.c */
SGBOOL ggetpixel_vp( SGUCHAR vp, GXT xs, GYT ys );      /* ggpixel.c */
#ifdef GHW_USING_COLOR
 GCOLOR gcgetpixel_vp( SGUCHAR vp, GXT xs, GYT ys );
#else
 #define gcgetpixel_vp( ch, xs, ys ) ggetpixel( (ch), (xs), (ys) )
#endif
#else
#define ginvertvp_vp( ch,  xs, ys, xe, ye )  {/* Nothing */}
#define ggetpixel_vp( ch,  xs, ys ) 0
#define gcgetpixel_vp( ch,  xs, ys ) 0
#endif /* GGRAPHICS && ! GHW_NO_LCD_READ_SUPPORT */

typedef void * PGSCREENS;
#ifdef GSCREENS
GBUFINT gscsize(void);  /* gscreen.c */
SGUCHAR gscinit(PGSCREENS scp); /* gscreen.c */
SGUCHAR gscisowner( PGSCREENS screen ); /* gscreen.c */
void gscrestore(PGSCREENS screen);      /* gscreen.c */
void gscsave( PGSCREENS screen );       /* gscreen.c */
#else
#define  gscsize( )  0
#define  gscinit( s )     { /* Nothing */ }
#define  gscisowner( s )  0
#define  gscrestore( s )  { /* Nothing */ }
#define  gscsave( s )     { /* Nothing */ }
#endif

#ifdef GSOFT_FONTS
PGCODEPAGE gfgetcp( PGFONT pfont );        /* gcpsel.c */
PGCODEPAGE ggetcp( void );              /* gcpsel.c */
PGCODEPAGE gselcp( PGCODEPAGE pcodepage );    /* gcpsel.c */
GXT gi_getsymw( GWCHAR c );             /* gfsymw.c */
#define ggetsymw( c ) gi_getsymw( (GWCHAR) ( c ) )
PGSYMBOL ggetfsymw(GWCHAR c, PGFONT fp);        /* ggetfsym.c */
#define ggetfsym(c,fp) ggetfsymw((GWCHAR)((SGUCHAR)(c)), (fp))
void gputfsym( GXT x, GYT y, SGUINT index, PGFONT pfont );  /* gputfsym.c */
void gputfsymm( GXT xmarg, GYT ymarg, SGUINT index, PGFONT pfont, GMODE mode ); /* gputfsymm.c */
void gfillfsym( GXT xs, GYT ys, GXT xe, GYT ye, SGUINT index, PGFONT pfont );
#else
#define gfgetcp(pf) NULL
#define ggetcp()    NULL
#define gselcp( pcp )     /* nothing */
#define ggetsymw(c) GDISPCW
#define ggetfsym(c,f) NULL
#define ggetfsymw(c,f) NULL
#define gputfsym(x, y, index, pfont ) /* nothing */
#define gputfsymm(xmarg, ymarg, index, pfont, mode ) /* nothing */
#define gfillfsym( xs, ys, xe, ye, index, pfont ) /* nothing */
#endif /* GSOFT_FONTS */

#if (defined( GSOFT_FONTS ) && defined( GFUNC_VP ))
PGCODEPAGE ggetcp_vp( SGUCHAR vp );     /* gcpsel.c */
PGCODEPAGE gselcp_vp( SGUCHAR vp, PGCODEPAGE pcp );     /* gcpsel.c */
GXT gi_getsymw_vp( SGUCHAR vp, GWCHAR c );      /* gfsymw.c */
#define ggetsymw_vp( vp, c ) gi_getsymw_vp( (vp), (GWCHAR) ( c ) )
void gputfsym_vp( SGUCHAR vp, GXT x, GYT y, SGUINT index, PGFONT pfont ); /* gputfsym.c */
void gputfsymm_vp( SGUCHAR vp, GXT xmarg, GYT ymarg, SGUINT index, PGFONT pfont, GMODE mode ); /* gputfsymm.c */
void gfillfsym_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye, SGUINT index, PGFONT pfont );
#else
#define ggetcp_vp( ch )    NULL
#define gselcp_vp( ch, pcp )     /* nothing */
#define gi_getsymw_vp( ch, c) GDISPCW
#define gputfsym_vp(vp, x, y, index, pfont ) /* nothing */
#define gputfsymm_vp(vp, xmarg, ymarg, index, pfont, mode ) /* nothing */
#define gfillfsym_vp( vp, xs, ys, xe, ye, index, pfont ) /* nothing */
#endif /* GSOFT_FONTS && GFUNC_VP */

#ifdef GSOFT_SYMBOLS
void gfillsym( GXT xs, GYT ys, GXT xe, GYT ye, PGSYMBOL psym ); /* gsymfill.c */
void ggetsym(GXT xs, GYT ys, GXT xe, GYT ye, GSYMBOL * psym, GBUFINT size );    /* gsymget.c */
void gputsym( GXT x, GYT y, PGSYMBOL psym );    /* gsymput.c */
void gputsymrot( SGINT x, SGINT y, float angle, PGSYMBOL psymbol,
                 SGINT xanchor, SGINT yanchor, GMODE symflag);  /* gputsymrot.c */
void gputfsymrot( SGINT x, SGINT y, float angle, SGUINT index, PGFONT pfont,
                  SGINT xanchor, SGINT yanchor, GMODE symflag); /* gputsymrot.c */
#else
#define gfillsym( xs, ys, xe, ye, psym ) {/* Nothing */}
#define ggetsym( xs, ys, xe, ye, psym, size ) {/* Nothing */}
#define gputsym( x, y, psym ) {/* Nothing */}
#define gputsymrot(x,y,angle, psymbol, xanchor,yanchor, symflag) { /* nothing */ }
#define gputfsymrot(x,y,angle, index, pfont, xanchor,yanchor, symflag) { /* nothing */ }
#endif /* SOFT_SYMBOLS */

#if defined( GSOFT_SYMBOLS ) && defined( GFUNC_VP )
void gfillsym_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye, PGSYMBOL psym );  /* gsymfill.c */
void ggetsym_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye, GSYMBOL * psym, GBUFINT size );    /* gsymget.c */
void gputsym_vp( SGUCHAR vp, GXT x, GYT y, PGSYMBOL psym );     /* gsymput.c */
void gputsymrot_vp( SGUCHAR vp, SGINT x, SGINT y, float angle, PGSYMBOL psymbol,
                    SGINT xanchor, SGINT yanchor, GMODE symflag); /* gputsymrot.c */
void gputfsymrot_vp( SGUCHAR vp, SGINT x, SGINT y, float angle, SGUINT index, PGFONT pfont,
                     SGINT xanchor, SGINT yanchor, GMODE symflag); /* gputsymrot.c */
#else
#define gfillsym_vp( ch, xs, ys, xe, ye, psym ) {/* Nothing */}
#define ggetsym_vp( ch, xs, ys, xe, ye, psym, size ) {/* Nothing */}
#define gputsym_vp( ch, x, y, psym ) {/* Nothing */}
#define gputsymrot_vp(ch, x,y,angle, psymbol, xanchor,yanchor, symflag) { /* nothing */ }
#define gputfsymrot_vp(ch, x,y, angle, index, pfont, xanchor,yanchor, symflag) { /* nothing */ }
#endif /* SOFT_SYMBOLS && GFUNC_VP */

#if defined( GSOFT_SYMBOLS ) && defined( GBASIC_TEXT )
void gputcsym( PGSYMBOL psymbol );      /* gsymcput.c */
#else
#define gputcsym( psymbol ) {/* Nothing */}
#endif /* SOFT_SYMBOLS && GBASIC_TEXT */

#if defined( GSOFT_SYMBOLS ) && defined( GBASIC_TEXT ) && defined( GFUNC_VP )
void gputcsym_vp( SGUCHAR vp, PGSYMBOL psymbol );       /* gsymcput.c */
#else
#define gputcsym_vp( ch, psymbol ) {/* Nothing */}
#endif

#ifdef GVIEWPORT
/* Basic highlevel features */
SGUCHAR ginit(void);

GMODE ggetmode( void ); /* ggetmode.c */
void gsetupcpy(SGUCHAR to_vp, SGUCHAR from_vp); /* gsetcpy.c */
void gclrvp(void);      /* gvpclr.c */
void gsetcvp( SGUCHAR cxs, SGUCHAR cys, SGUCHAR cxe, SGUCHAR cye );     /* gvpcset.c */
void ggetvp(GXT *xs, GYT *ys, GXT *xe, GYT *ye );       /* gvpget.c */
SGUCHAR ginit(void);    /* gvpinit.c */
GMODE gi_setmode( GMODE mode ); /* gvpmode.c */
#define gsetmode( mode ) gi_setmode((GMODE)( mode ))
void gresetinitvp(void);    /* gvpreset.c */
void gresetposvp(void); /* gvpreset.c */
void gresetvp(void);    /* gvpreset.c */
SGUCHAR ggetvpnum( void );      /* gvpsel.c */
SGUCHAR gselvp( SGUCHAR vp );   /* gvpsel.c */
void gsetvp(GXT xs, GYT ys, GXT xe, GYT ye );   /* gvpset.c */

void set_max_pal(void);
void set_mid_pal(void);
void set_min_pal(void);

GYT ggetvph_vp(SGUCHAR vp); /* gvpvph.c */
GYT ggetvph(void);          /* gvpvph.c */
GXT ggetvpw(void);          /* gvpvpw.c */
GXT ggetvpw_vp(SGUCHAR vp); /* gvpvpw.c */
GXT gvpxl( SGUCHAR vp );    /* gvpxl.c */
GXT gvpxr( SGUCHAR vp );    /* gvpxr.c */
GYT gvpyb( SGUCHAR vp );    /* gvpyb.c */
GYT gvpyt( SGUCHAR vp );    /* gvpyt.c */
#else
/* No high-level features */
#define ginit() ghw_init()
#define ggetmode() 0
#define gsetupcpy(to_vp, from_vp)      { /* Nothing */ }
#define gclrvp()                       { /* Nothing */ }
#define gsetcvp( cxs, cys, cxe, cye )  { /* Nothing */ }
#define ggetvp(xs, ys, xe,ye )  { /* Nothing */ }
#define gsetmode( mode ) { /* Nothing */ }
#define gresetinitvp() { /* Nothing */ }
#define gresetposvp() { /* Nothing */ }
#define gresetvp()    { /* Nothing */ }
#define ggetvpnum() 0
#define gselvp(vp) 0
#define gsetvp(xs, ys, xe, ye ) { /* Nothing */ }

#define ggetvph_vp(vp) GDISPH
#define ggetvph() GDISPH
#define ggetvpw() GDISPW
#define ggetvpw_vp(vp) GDISPW
#define gvpxl( vp ) 0
#define gvpxr( vp ) (GDISPW-1)
#define gvpyb( vp ) 0
#define gvpyt( vp ) (GDISPH-1)
#endif

#if defined( GVIEWPORT ) && defined( GHW_USING_COLOR )
 GCOLOR ggetcolorb(void);  /* gcgetbak.c */
 GCOLOR ggetcolorf(void);  /* gcgetfor.c */
 GCOLOR gsetcolorb(GCOLOR back); /* gcsetbak.c */
 GCOLOR gsetcolorf(GCOLOR fore); /* gcsetfor.c */
#elif defined( GVIEWPORT )
 /* Assume color application compiled as b&w application (GCOLOR type become a GNORMAL  GINVERSE mode flag) */
 /* Using b&w mode switching to represent foreground background */
 #define ggetcolorf() ((GCOLOR)( ggetmode() & GINVERSE))
 #define ggetcolorb() ((GCOLOR)(~ggetmode() & GINVERSE))
 #define gsetcolorf(fore) ((GCOLOR)gsetmode((ggetmode() & ~GINVERSE) | ( (fore) & GINVERSE)))
 #define gsetcolorb(back) ((GCOLOR)gsetmode((ggetmode() & ~GINVERSE) | (~(back) & GINVERSE)))
#else
 #define ggetcolorf()  1
 #define ggetcolorb()  0
 #define gsetcolorf(fore) 1
 #define gsetcolorb(back) 0
#endif

#if defined( GVIEWPORT ) && defined( GFUNC_VP ) && defined( GHW_USING_COLOR )
 GCOLOR ggetcolorb_vp(SGUCHAR vp); /* gcgetbak.c */
 GCOLOR ggetcolorf_vp(SGUCHAR vp); /* gcgetfor.c */
 GCOLOR gsetcolorb_vp(SGUCHAR vp, GCOLOR back); /* gcsetbak.c */
 GCOLOR gsetcolorf_vp(SGUCHAR vp, GCOLOR fore); /* gcsetfor.c */
#elif (defined( GVIEWPORT ) && defined( GFUNC_VP ))
 /* Assume color application compiled as b&w application */
 /* Using b&w mode switching to represent foreground background */
 #define ggetcolorf_vp( ch ) ((GCOLOR)( ggetmode_vp( (ch) ) & GINVERSE))
 #define ggetcolorb_vp( ch ) ((GCOLOR)(~ggetmode_vp( (ch) ) & GINVERSE))
 #define gsetcolorf_vp( ch, fore) ((GCOLOR)gsetmode_vp( (ch), (ggetmode_vp( (ch) ) & ~GINVERSE) | ( (fore) & GINVERSE)))
 #define gsetcolorb_vp( ch, back) ((GCOLOR)gsetmode_vp( (ch), (ggetmode_vp( (ch) ) & ~GINVERSE) | (~(back) & GINVERSE)))
#else
 #define ggetcolorf_vp()  1
 #define ggetcolorb_vp()  0
 #define gsetcolorf_vp(fore) 1
 #define gsetcolorb_vp(back) 0
#endif

#define gexit() ghw_exit()

/*********************************************************************
   Setup a view-port in one line
   vp = vp to setup
   l = left in abs pixels
   t = top in abs pixels
   r = right in abs pixels
   b = bottom in abs pixels
   f = pointer to font
   cp = pointer to codepage
   m = display mode (normal, inverse)
*/
#define gsetupvp(vp,l,t,r,b,f,cp,m) \
   { \
   gselvp( (vp) ); \
   gresetvp(); \
   gselfont( (f) ); \
   gselcp( (cp) ); \
   gsetvp( (l),(t),(r),(b) ); \
   gsetmode( (m) ); \
   gclrvp(); \
   }

/* Map directly to low_level driver */
#define gsetupdate( on )  ghw_setupdate( (on) )

#if defined( GVIEWPORT ) && defined( GFUNC_VP )
GMODE ggetmode_vp( SGUCHAR vp ); /* ggetmode.c */
void gclrvp_vp( SGUCHAR vp );    /* gvpclr.c */
void gsetcvp_vp( SGUCHAR vp, SGUCHAR cxs, SGUCHAR cys, SGUCHAR cxe, SGUCHAR cye ); /* gvpcset.c */
void ggetvp_vp( SGUCHAR vp, GXT *xs, GYT *ys, GXT *xe, GYT *ye );       /* gvpget.c */
GMODE gi_setmode_vp( SGUCHAR vp, GMODE mode );  /* gvpmode.c */
#define gsetmode_vp( vp, mode ) gi_setmode_vp((vp), ((GMODE)( mode )));
void gresetinitvp_vp( SGUCHAR vp ); /* gvpreset.c */
void gresetposvp_vp( SGUCHAR vp );  /* gvpreset.c */
void gresetvp_vp( SGUCHAR vp ); /* gvpreset.c */
void gsetvp_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye );   /* gvpset.c */
#endif

/* General string functions */

PGSTR gmstrcat( PGSTR dstr, PGCSTR sstr ); /* gmstrcpy.c */
PGSTR gmstrcpy( PGSTR dstr, PGCSTR sstr ); /* gmstrcpy.c */
GWCHAR ggetmbc(PGCSTR *strp);              /* ggetmb.c */
SGUINT gmsstrlen( PGCSTR str );            /* gmslen.c */
SGUINT gmstrlen( PGCSTR str );             /* gmslen.c */
SGUCHAR gstrlines( PGCSTR str );           /* gstrln.c */

PGSTR gwstrcpymb( PGSTR dstr, PGCWSTR sstr );   /* gmbcpyw.c */
SGUCHAR gwctomb( PGSTR str, GWCHAR wc );        /* gmbcpyw.c */
PGWSTR gmbstrcpyw( PGWSTR dstr, PGCSTR sstr );  /* gmbcpyw.c */
#ifdef GWIDECHAR
SGUCHAR gstrlinesw( PGCWSTR str ); /* gstrln.c */
SGUCHAR gwcmbsize( GWCHAR wc );    /* gmslen.c */
SGUINT gmstrlenw( PGCWSTR str );   /* gmslen.c */
#endif

#ifdef GEXTMODE
/* Include GVPAPP structure defined by user (= add viewport extensions) */
#ifdef __cplusplus
}
#endif

#include <gvpapp.h>

#ifdef __cplusplus
extern "C" {
#endif
 typedef GVPAPP PGENERIC * PGVPAPP;
 PGVPAPP ggetapp( void );          /* gvpapp.c */
 PGVPAPP ggetapp_vp( SGUCHAR vp ); /* gvpapp.c */
 SGUCHAR ggetvpapp( PGVPAPP app ); /* gvpapp.c */
#else  /* GEXTMODE */
 #define ggetapp() NULL
 #define ggetapp_vp(vp) NULL
 #define ggetvpapp( app ) 0
#endif /* GEXTMODE */

#ifdef __cplusplus
}
#endif

#endif /* GDISP_H */



