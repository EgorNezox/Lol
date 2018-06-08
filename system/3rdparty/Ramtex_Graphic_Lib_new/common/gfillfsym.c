/************************* gfillfsym.c ****************************

   Creation date: 20-06-2012

   Fill box area with a symbol from a font. The symbol is repeated as may
   times as possible starting from the upper left corner.
   Any symbol cropping is done at right / lower edges.

   Note: if the font contains a code page then the symbol index is treated
   like a (wide) character and converted in accordance with the codepage settings.

   Revision date:
   Revision Purpose:

   Version number: 1.0
   Copyright (c) RAMTEX International Aps 2012
   Web site, support and upgrade: www.ramtex.dk

********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GSOFT_FONTS

void gfillfsym( GXT xs, GYT ys, GXT xe, GYT ye, SGUINT index, PGFONT pfont )
   {
   PGSYMBOL psym;
   SGUINT x; /* must be integer! */
   SGUINT y;
   gi_datacheck(); /* check internal data for errors */
   /* Get pointer to symbol, use code page lookup if the font contains a codepage */
   if ((psym = gi_getsymbol((GWCHAR)index,pfont,gi_fpcodepage(pfont))) == NULL)
      {
      /* Warning already issued in gi_getsymbol(..) */
      return;
      }
   /* normalize to view-port */
   xs += gcurvp->lt.x;
   ys += gcurvp->lt.y;
   xe += gcurvp->lt.x;
   ye += gcurvp->lt.y;

   /* limit values to view-port */
   LIMITTOVP( "gfillfsym",xs,ys,xe,ye );

   glcd_err = 0; /* Reset HW error flag */
   for( y = ys; y<=ye; y += gsymh(psym) )
      {
      for( x = xs; x<=xe; x += gsymw(psym))
         {
         gi_putsymbol( (GXT)x,(GYT)y,xe,ye, psym, 0, gi_fsymsize(pfont) );
         }
      }

   ghw_updatehw();
   }

#ifdef GFUNC_VP

void gfillfsym_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye, SGUINT index, PGFONT pfont )
   {
   GSETFUNCVP(vp, gfillfsym( xs, ys, xe, ye, index, pfont ));
   }

#endif /* GFUNC_VP */
#endif /* GSOFT_SYMBOLS */


