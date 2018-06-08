/************************* ggrect.c ********************************

   Creation date: 980224

   Revision date:     03-01-26
   Revision Purpose:  Bit mask on GINVERSE mode

   Revision date:     03-11-03
   Revision Purpose:  Color support added

   Revision date:     12-02-04
   Revision Purpose:  gcrectangle(..) added

   Revision date:     05-06-04
   Revision Purpose:  gi_datacheck added in gcrectangle(..)

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Revision date:     03-06-05
   Revision Purpose:  GHW_USING_COLOR switch support added

   Version number: 2.5
   Copyright (c) RAMTEX International Aps 1998-2005
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/
#include <gi_disp.h> /* gLCD prototypes */

#ifdef GGRAPHICS

/********************************************************************
   Segment: Graphics
   Level: Graphics
   Draw a rectangle, not filling inner.
   grectangle(0,0,2,2) is a border with 1 pixel inside.
*/
#ifdef GHW_USING_COLOR
void gcrectangle( GXT xs, GYT ys, GXT xe, GYT ye, GCOLOR linecolor )
   {
   gi_datacheck();

   /* normalize to view-port */
   xs += gcurvp->lt.x;
   ys += gcurvp->lt.y;
   xe += gcurvp->lt.x;
   ye += gcurvp->lt.y;

   /* limit values to view-port */
   LIMITTOVP( "gcrectangle",xs,ys,xe,ye );

   glcd_err = 0; /* Reset HW error flag */
   ghw_rectangle( xs,ys,xe,ye, linecolor );
   ghw_updatehw();
   }
#endif

void grectangle( GXT xs, GYT ys, GXT xe, GYT ye )
   {
   gi_datacheck();

   /* normalize to view-port */
   xs += gcurvp->lt.x;
   ys += gcurvp->lt.y;
   xe += gcurvp->lt.x;
   ye += gcurvp->lt.y;

   /* limit values to view-port */
   LIMITTOVP( "grectangle",xs,ys,xe,ye );

   glcd_err = 0; /* Reset HW error flag */
   ghw_rectangle( xs,ys,xe,ye, G_IS_INVERSE() ? ghw_def_background : ghw_def_foreground );
   ghw_updatehw();
   }

#ifdef GFUNC_VP

#ifdef GHW_USING_COLOR
void gcrectangle_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye, GCOLOR linecolor )
   {
   GSETFUNCVP(vp, gcrectangle(xs,ys,xe,ye, linecolor) );
   }
#endif

void grectangle_vp( SGUCHAR vp, GXT xs, GYT ys, GXT xe, GYT ye )
   {
   GSETFUNCVP(vp, grectangle( xs, ys, xe,  ye ) );
   }

#endif /* GFUNC_VP */


#endif /* GGRAPHICS */

