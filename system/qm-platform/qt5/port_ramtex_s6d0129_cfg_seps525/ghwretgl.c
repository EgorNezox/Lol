/************************** ghwretgl.c *****************************

   Low-level function for drawing rectangles or straight vertical or
   horizontal lines.

   Absolute coordinates are used.

   The S6D0129 controller is assumed to be used with a LCD module.
   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is copied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Revision date:
   Revision Purpose:

   Version number: 1.0
   Copyright (c) RAMTEX Engineering Aps 2006

*********************************************************************/

#include <s6d0129.h>   /* s6d0129 controller specific definements */

#ifdef GGRAPHICS

extern SGBOOL ghw_upddelay;

#if (!defined( GHW_ACCELERATOR ) || defined( GBUFFER ))
/*
   Draw horisontal line
*/
static void ghw_lineh(GXT xb, GYT yb, GXT xe, GCOLOR color)
   {
   #ifdef GBUFFER
   GBUFINT gbufidx;
   invalx( xb );
   invalx( xe );
   invaly( yb );
   gbufidx = GINDEX(xb,yb);
   for (; xb <= xe; xb++ )
      {
      /* Write destination */
      gbuf[gbufidx++] = color;
      }
   if (ghw_upddelay == 0)
      ghw_updatehw();
   #else

   ghw_set_xyrange(xb,yb,xe,yb);
   for (; xb <= xe; xb++ )
      ghw_auto_wr(color);  /* Write destination */

   #endif
   }

/*
   Draw vertical line
*/
static void ghw_linev(GXT xb, GYT yb, GYT ye, GCOLOR color)
   {
   #ifdef GBUFFER
   GBUFINT gbufidx;
   invalx( xb );
   invaly( yb );
   invaly( ye );
   gbufidx = GINDEX(xb,yb);
   for (; yb <= ye; yb++)
      {
      gbuf[gbufidx] = color;
      gbufidx += GDISPW;
      }

   if (ghw_upddelay == 0)
      ghw_updatehw();

   #else

   ghw_set_xyrange(xb,yb,xb,ye);
   while (yb <= ye )
      {
      ghw_auto_wr(color);
      yb++;
      }

   #endif
   }
#endif

/*
   Provides accelerated line drawing for horizontal/vertical lines.

   If left-top and right-bottom is on a single vertical or horizontal
   line a single line is drawn.

   All coordinates are absolute coordinates.
*/
void ghw_rectangle(GXT ltx, GYT lty, GXT rbx, GYT rby, GCOLOR color)
   {
   glcd_err = 0;

   /* Force resonable values */
   GLIMITU(ltx,GDISPW-1);
   GLIMITU(lty,GDISPH-1);
   GLIMITD(rby,lty);
   GLIMITU(rby,GDISPH-1);
   GLIMITD(rbx,ltx);
   GLIMITU(rbx,GDISPW-1);
   #ifdef GBUFFER
   GBUF_CHECK();
   #endif
   if (ltx != rbx)
      ghw_lineh(ltx, lty, rbx, color);      /* Draw horisontal line */

   if (lty != rby)
      {
      ghw_linev(ltx, lty, rby, color);      /* Draw vertical line */
      if (ltx != rbx)
         {                                  /* It is box coordinates */
         ghw_lineh(ltx, rby, rbx, color);   /* Draw bottom horizontal line */
         ghw_linev(rbx, lty, rby, color);   /* Draw right vertical line */
         }
      }
   else
      if (ltx == rbx)
         ghw_lineh(ltx, rby, rbx, color);   /* Draw dot */
   }
#endif /* GGRAPHICS */



