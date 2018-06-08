/************************* ggpixel.c *******************************

   Creation date: 980224

   Revision date:     03-11-03
   Revision Purpose:  Color support added

   Revision date:     12-02-04
   Revision Purpose:  gcsetpixel(..) added

   Revision date:     03-06-05
   Revision Purpose:  GHW_USING_COLOR switch support added

   Revision date:
   Revision Purpose:

   Version number: 2.2
   Copyright (c) RAMTEX Engineering Aps 1998-2005

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GGRAPHICS
/********************************************************************
   Segment: Graphics
   Level: Graphics
   A pos. outside view-port is not drawn
*/

#ifdef GHW_USING_COLOR
void gcsetpixel( GXT xs, GYT ys, GCOLOR pixel )
   {
   gi_datacheck(); /* check internal data for errors */
   /* normalize to view-port */
   xs += gcurvp->lt.x;
   ys += gcurvp->lt.y;

   if(( xs < gcurvp->lt.x ) || ( xs > gcurvp->rb.x ) ||
      ( ys < gcurvp->lt.y ) || ( ys > gcurvp->rb.y ))
      return;

   glcd_err = 0; /* Reset HW error flag */
   ghw_setpixel( xs,ys, pixel);
   ghw_updatehw();
   }
#endif

/* Set pixel  == 0 = white, != 0 = black */
void gsetpixel( GXT xs, GYT ys, SGBOOL pixel )
   {
   gi_datacheck(); /* check internal data for errors */
   pixel = (SGBOOL)(G_IS_INVERSE() ? !pixel : pixel);
   /* normalize to view-port */
   xs += gcurvp->lt.x;
   ys += gcurvp->lt.y;

   if(( xs < gcurvp->lt.x ) || ( xs > gcurvp->rb.x ) ||
      ( ys < gcurvp->lt.y ) || ( ys > gcurvp->rb.y ))
      return;

   glcd_err = 0; /* Reset HW error flag */
   ghw_setpixel( xs,ys, pixel ? ghw_def_foreground : ghw_def_background);
   ghw_updatehw();
   }

/********************************************************************
   Segment: Graphics
   Level: Graphics
   Get pixel == 0 = white, != 0 = black
*/

#ifndef GHW_NO_LCD_READ_SUPPORT

GCOLOR gi_cgetpixel( GXT xs, GYT ys )
   {
   gi_datacheck(); /* check internal data for errors */
   /* normalize to view-port */
   xs += gcurvp->lt.x;
   ys += gcurvp->lt.y;

   if( xs < gcurvp->lt.x )
      {
      G_WARNING( "ggetpixel: parameter, x<vp.left" );
      return 0;
      }
   if( xs > gcurvp->rb.x )
      {
      G_WARNING( "ggetpixel: parameter, x>vp.right" );
      return 0;
      }
   if( ys < gcurvp->lt.y )
      {
      G_WARNING( "ggetpixel: parameter, y<vp.top" );
      return 0;
      }
   if( ys > gcurvp->rb.y )
      {
      G_WARNING( "ggetpixel: parameter, y>vp.bottom" );
      return 0;
      }

   glcd_err = 0; /* Reset HW error flag */
   return ghw_getpixel(xs,ys);
   }

SGBOOL ggetpixel( GXT xs, GYT ys )
   {
   GCOLOR color;
   color = gi_cgetpixel(xs,ys);
   return (G_IS_INVERSE() ? (color == ghw_def_background) : (color == ghw_def_foreground));
   }

#endif

#ifdef GFUNC_VP

void gsetpixel_vp( SGUCHAR vp, GXT xs, GYT ys, SGBOOL pixel )
   {
   GSETFUNCVP(vp, gsetpixel(xs,ys,pixel) );
   }

#ifdef GHW_USING_COLOR
void gcsetpixel_vp( SGUCHAR vp, GXT xs, GYT ys, GCOLOR pixelcolor )
   {
   GSETFUNCVP(vp, gcsetpixel(xs,ys,pixelcolor) );
   }
#endif

#ifndef GHW_NO_LCD_READ_SUPPORT
#ifdef GHW_USING_COLOR

GCOLOR gcgetpixel_vp( SGUCHAR vp, GXT xs, GYT ys )
   {
   GCOLOR retp;
   GGETFUNCVP(vp, gcgetpixel(xs,ys) );
   return retp;
   }
#endif

SGBOOL ggetpixel_vp( SGUCHAR vp, GXT xs, GYT ys )
   {
   SGBOOL retp;
   GGETFUNCVP(vp, ggetpixel(xs,ys) );
   return retp;
   }

#endif

#endif /* GFUNC_VP */

#endif /* GGRAPHICS */

