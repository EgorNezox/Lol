/************************* gsymfill.c ******************************

   Creation date: 980223

   Revision date:     02-01-23
   Revision Purpose:  symsize parameter added to gi_putsymbol(..)

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Version number: 2.2
   Copyright (c) RAMTEX Engineering Aps 1998-2004

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GSOFT_SYMBOLS

/********************************************************************
   Segment: Software symbols
   Level: Graphics
   Fill box area with symbol.
   Symbol is repeated as may times as possible
   starting from the upper left corner.
   If required only a part of the symbol is used.
*/
void gfillsym( GXT xs, GYT ys, GXT xe, GYT ye, PGSYMBOL psym )
   {
   SGUINT x; /* must be integer! */
   SGUINT y;

   gi_datacheck(); /* check internal data for errors */
   /* normalize to view-port */
   xs += gcurvp->lt.x;
   ys += gcurvp->lt.y;
   xe += gcurvp->lt.x;
   ye += gcurvp->lt.y;

   /* limit values to view-port */
   LIMITTOVP( "gfillsym",xs,ys,xe,ye );
   if( psym == NULL )
      {
      G_WARNING( "gfillsym: parameter, No symbol" );
      return;
      }

   glcd_err = 0; /* Reset HW error flag */
   for( y = ys; y<=ye; y += gsymh(psym) )
      for( x = xs; x<=xe; x += gsymw(psym))
        gi_putsymbol( (GXT)x,(GYT)y,xe,ye, psym,0,0 );

   ghw_updatehw();
   }

#ifdef GFUNC_VP

void gfillsym_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye, PGSYMBOL psym )
   {
   GSETFUNCVP(vp, gfillsym(xs,ys,xe,ye,psym) );
   }

#endif /* GFUNC_VP */
#endif /* GSOFT_SYMBOLS */

