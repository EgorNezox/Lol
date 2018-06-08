/************************* gfsel.c *********************************

   Creation date: 980223

   Revision date:     02-01-23
   Revision Purpose:  Support for soft cursors change.
   Revision date:     14-04-03
   Revision Purpose:  Support for extended codepage.
   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     10-02-05
   Revision Purpose:  cursor y positions adjustment added so a font
                      is visible inside the viewport
   Revision date:     15-10-05
   Revision Purpose:  Warning added if font height exceeds viewport height
   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption
   Revision date:     06-02-08
   Revision Purpose:  Extra spacing support added

   Version number: 4.7
   Copyright (c) RAMTEX Engineering Aps 1998-2008

*********************************************************************/

#include <gi_disp.h> /* glcd prototypes */

#if defined( GSOFT_FONTS ) || defined( GBASIC_TEXT )
/********************************************************************
   Segment: SoftFonts
   Level: Fonts
   Set current font (incl. default character h,w)
   pass a pointer from a <font>.c file.
   Returns previous font
*/
PGFONT gselfont( PGFONT pfont )
   {
   PGFONT pf;
   GYT h;
   #ifndef GNOCURSOR
   GCURSOR c;
   #endif

   #ifndef GNOCURSOR
   glcd_err = 0; /* Reset HW error flag */
   gi_datacheck(); /* check internal data for errors */

   c = ghw_cursor;
   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
       ghw_setcursor( (GCURSOR)(ghw_cursor & ~GCURON) );
   #endif
   #if  !defined( GHW_NO_HDW_FONT ) && defined( GSOFT_FONTS )
   else
   #endif
   #ifdef GSOFT_FONTS
      {
      gi_cursor( 0 ); /* Remove cursor of old font size */
      }
   #endif
   #endif /* GNOCURSOR */

   pf = gcurvp->pfont;
   if( pfont == NULL)
      {
      G_WARNING( "gselfont: Parameter, No font selected" );
      gcurvp->pfont = &SYSFONT;
      gcurvp->codepagep = SYSFONT.pcodepage;
      }
   else
      {
      gcurvp->pfont = pfont;
      gcurvp->codepagep = gi_fpcodepage(pfont);
      }

   /* Move cpos to legal value */
   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
      {
      h = GDISPCH;  /* default height */
      #ifndef GNOTXTSPACE
      /* Hardware fonts does not support extra character or line spacing */
      gcurvp->chln.x = 0;
      gcurvp->chln.y = 0;
      #endif
      }
   #endif
   #if  !defined( GHW_NO_HDW_FONT ) && defined( GSOFT_FONTS )
   else
   #endif
   #if  defined( GSOFT_FONTS )
      {
      #if (defined( GVIRTUAL_FONTS) && !defined( GNOTXTSPACE ))
      /* Check if font spacing is used by font */
      if (gisfontv(pfont))
         {
         /* Use font specific extended spacing settings */
         gcurvp->chln.x = ((PGFONTV)pfont)->chsp;
         gcurvp->chln.y = ((PGFONTV)pfont)->lnsp;
         }
      #endif
      h = gi_fsymh(gcurvp->pfont);
      #ifndef GNOTXTSPACE
      /* Limit line and character spacing to resonable values */
      gi_limit_check();
      #endif
      }
   #endif

   if( gcurvp->cpos.y < gcurvp->lt.y+(h-1))
      {
      gcurvp->cpos.y = gcurvp->lt.y+(h-1); /* Assure font is visible */
      if (gcurvp->cpos.y > gcurvp->rb.y)
         gcurvp->cpos.y = gcurvp->rb.y;    /* Must not exceed buttom */
      }

   #ifdef GGRAPHICS
   /* update graphics pos also */
   gcurvp->ppos.x = (gcurvp->cpos.x > gcurvp->rb.x) ? gcurvp->rb.x : gcurvp->cpos.x;
   gcurvp->ppos.y = gcurvp->cpos.y;
   #endif

   gi_calcdatacheck(); /* correct VP to new settings */

   #ifndef GNOCURSOR
   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
       ghw_setcursor( c );
   #endif
   #if  !defined( GHW_NO_HDW_FONT ) && defined( GSOFT_FONTS )
   else
   #endif
   #ifdef GSOFT_FONTS
      {
      ghw_cursor = c;
      gi_cursor( 1 ); /* Restore cursor with new font size */
      }
   #endif
   #endif /* GNOCURSOR */
   return pf;
   }

PGFONT ggetfont( void )
   {
   gi_datacheck(); /* check internal data for errors */
   return gcurvp->pfont;
   }

#ifdef GFUNC_VP

PGFONT gselfont_vp( SGUCHAR vp, PGFONT pfont )
   {
   PGFONT retp;
   GGETFUNCVP(vp, gselfont(pfont) );
   return retp;
   }

#endif /* GFUNC_VP */

PGFONT ggetfont_vp( SGUCHAR vp )
   {
   if ( vp >= GNUMVP)
      {
      G_WARNING("Parameter error vp >= GNUMVP");
      vp = GNUMVP-1;
      }
   return gdata.viewports[vp].pfont;
   }

#endif /* GSOFT_FONTS || GBASIC_TEXT */


