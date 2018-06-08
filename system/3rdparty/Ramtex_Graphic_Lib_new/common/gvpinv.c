/************************* gvpinv.c *******************************

   Creation date:    19-03-02

   Revision date:    26-07-02
   Revision Purpose: Cursor is killed before invert

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Version number: 1.2
   Copyright (c) RAMTEX International Aps 2002-2004
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD protoypes */

#ifdef GGRAPHICS
#ifndef GHW_NO_LCD_READ_SUPPORT
/********************************************************************
   Segment: View-port
   Level: View-port
   Invert area in view-port.
*/
void ginvertvp( GXT xs, GYT ys, GXT xe, GYT ye)
   {
   gi_datacheck();      /* check internal data for errors */
   gi_cursor( 0 );      /* kill cursor */

   /* normalize to view-port */
   xs += gcurvp->lt.x;
   xe += gcurvp->lt.x;
   ys += gcurvp->lt.y;
   ye += gcurvp->lt.y;

   /* limit values to view-port */
   LIMITTOVP( "ginvertvp", xs,ys,xe,ye );

   glcd_err = 0; /* Reset HW error flag */
   ghw_invert(xs,ys,xe,ye);

   ghw_updatehw();
   gi_cursor( 1 );      /* set cursor */

   gi_calcdatacheck();  /* correct VP to new settings */
   }


#ifdef GFUNC_VP
void ginvertvp_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye)
   {
   GSETFUNCVP(vp, ginvertvp(xs,ys,xe,ye) );
   }

#endif /* GFUNC_VP */
#endif /* GHW_NO_LCD_READ_SUPPORT */
#endif

