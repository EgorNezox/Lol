/************************* gcarc.c ********************************

   Draw corner arc (a 90 degree arc aligned to x,y axis)
   Drawing possiblities:
     GLINE  Perimeter line with foreground color (default)
     GFILL  Inner area filled with background info
     GFRAME Combination of above
  The arc orinetation is selected with these parameters
     GCARC_LT   left-top corner
     GCARC_LB   left-bottom corner
     GCARC_RT   right top corner
     GCARC_RB   right bottom corner.
  One or more arc orientations is drawn in one call.

   Creation date:     04-12-2008
   Revision date:     06-03-2009
   Revision Purpose:  gi_arc(.) extrated to a seperate module for
                      better code size optimization possibilities.

   Version number: 1.1
   Copyright (c) RAMTEX International Aps 2008
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/
#include <gi_disp.h>

#ifdef GGRAPHICS

/*
*/
void gcarc(GXT xc, GYT yc, GXYT r, SGUCHAR arctype)
   {
   /* Check and limit to prevent negative coordinates */
   if (((arctype & (GCARC_LT|GCARC_LB))!= 0) && ( r > xc))
      {
      r=xc;
      G_WARNING( "gcarc: radius turncated" );
      }
   if (((arctype & (GCARC_LT|GCARC_RT))!= 0) && ( r > yc))
      {
      r=yc;
      G_WARNING( "gcarc: radius turncated" );
      }

   xc += gcurvp->lt.x;
   yc += gcurvp->lt.y;

   /* Limit center to be within viewport */
   LIMITTOVP( "gcarc",xc,yc,xc,yc );

   /* Check and limit radius to prevent viewport overflow */
   if (((arctype & (GCARC_RT|GCARC_RB))!= 0)&&(xc+r > gcurvp->rb.x))
      {
      r = gcurvp->rb.x-xc;
      G_WARNING( "gcarc: radius turncated" );
      }
   if (((arctype & (GCARC_LB|GCARC_RB))!= 0)&& (yc+r > gcurvp->rb.y))
      {
      r = gcurvp->rb.y-yc;
      G_WARNING( "gcarc: radius turncated" );
      }

   if( r == 0)
      return;

   gi_carc(xc, yc, r, arctype);
   ghw_updatehw();
   }

#ifdef GFUNC_VP
void gcarc_vp(SGUCHAR vp, GXT xc, GYT yc, GXYT r, SGUCHAR arctype)
   {
   GSETFUNCVP(vp, gcarc( xc, yc, r, arctype));
   }
#endif /* GFUNC_VP */


#endif /* GGRAPHICS */

