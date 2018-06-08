/************************** ghwretgl.c *****************************

   Low-level function for drawing rectangles or straight vertical or
   horizontal lines.

   Absolute coordinates are used.

   The display controller is assumed to be used with a LCD module.
   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is copied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Creation date:
   Revision date:    4-1-05
   Revision Purpose: Non-buffered mode speed optimized by use of
                     ghw_tmpbuf for read-modify-write operations.
   Revision date:    061005
   Revision Purpose: GBUFFER mode speed optimized
   Revision date:    11-11-2011
   Revision Purpose: Customized for use with horizontal storage units.
   Revision date:    16-03-2012
   Revision Purpose: Support for use of vertical storage units
                     and 90-(270) degree rotation added
                     Speed improvement by new position handling
   Revision date:
   Revision Purpose:

   Version number: 1.4
   Copyright (c) RAMTEX Engineering Aps 2004-2015

*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

#ifdef GGRAPHICS

#ifdef GBUFFER
extern SGBOOL ghw_upddelay;
#endif

#ifdef GHW_USING_VBYTE

/*
   Draw vertical line
*/
static void ghw_linev(GXT xb, GYT yb, GYT ye, GHWCOLOR color)
   {
   GFAST GHWCOLOR msk;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   #endif
   if ((xb >= GDISPW) || (yb >= GDISPH)) return;
   if (ye >= GDISPH) ye = GDISPH-1;

   #ifdef GBUFFER
   invalx( xb );
   invaly( yb );
   invaly( ye );
   #endif

   msk = startmask[GPIXEL(yb)];
   /* Access whole vertical bytes at a time */
   yb = GPIXMSK(yb);

   for (; yb <= ye; yb += GHWPIXPSU)
      {
      if (yb == GPIXMSK(ye))
         msk &= stopmask[GPIXEL(ye)];

      /* Write destination */
      #ifdef GBUFFER
      gbufidx = GINDEX(xb,yb);
      if (msk == GHWCOLOR_NO_MSK)
         gbuf[gbufidx] = color;
      else
         {
         gbuf[gbufidx] = (color & msk) | (gbuf[gbufidx] & ~msk);
         msk = GHWCOLOR_NO_MSK;
         }
      #else
      ghw_set_yrange(yb,yb); /* prevent autowrap */
      ghw_set_xrange(xb,xb);
      if (msk == GHWCOLOR_NO_MSK)
         {
         ghw_auto_wr(color);
         }
      else
         {
         ghw_auto_wr((color & msk) | (ghw_rd_x(xb) & ~msk));
         msk = GHWCOLOR_NO_MSK;
         }
      #endif
      }
   #ifdef GBUFFER
   if (ghw_upddelay == 0)
   #endif
      ghw_updatehw();
   }

/*
   Draw horizontal line
*/
static void ghw_lineh(GXT xb, GYT yb, GXT xe, GHWCOLOR color)
   {
   GFAST GHWCOLOR msk;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   #else
   GFAST GXT x,xr;
   #endif
   if ((xb >= GDISPW) || (yb >= GDISPH)) return;
   if  (xe >= GDISPW) xe = GDISPW-1;

   #ifdef GBUFFER
   invalx( xb );
   invaly( yb );
   invalx( xe );
   gbufidx = GINDEX(xb,yb);
   #endif
   msk = pixmsk[GPIXEL(yb)];
   color &= msk;

   #ifdef GBUFFER
   for (; xb <= xe; xb++)
      {
      gbuf[gbufidx] = color | (gbuf[gbufidx] & ~msk);
      gbufidx++;
      }
   #else
   /* Do read modify-write, via temp buffer */
   ghw_set_yrange(yb,yb);
   while (xb <= xe )
      {
      ghw_set_xpos(xb);
      x = xe - xb; /* (remaining) line length */
      if (x >= sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0]))
         x = sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0])-1;

      /* Set one byte wide range, and read to tmp buffer  */
      ghw_auto_rd_start();
      for ( xr = 0; xr <= x; xr++)
         ghw_tmpbuf[xr] = ghw_auto_rd();
      ghw_set_xpos(xb);

      /* Do modify-write via tmp buffer */
      for ( xr = 0; xr <= x; xr++)
         ghw_auto_wr( color | (ghw_tmpbuf[xr] & ~msk));
      xb += x+1;
      }
   #endif
   #ifdef GBUFFER
   if (ghw_upddelay == 0)
   #endif
      ghw_updatehw();
   }

#else  /* GHW_USING_VBYTE */
/*
   Draw horisontal line
*/
static void ghw_lineh(GXT xb, GYT yb, GXT xe, GHWCOLOR color)
   {
   GFAST GYT x;
   GFAST GHWCOLOR msk,mske;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   #endif
   if ((xb >= GDISPW) || (yb >= GDISPH)) return;
   if  (xe >= GDISPW) xe = GDISPW-1;

   #ifdef GBUFFER
   invalx( xb );
   invalx( xe );
   invaly( yb );
   gbufidx = GINDEX(xb,yb);
   #else
   ghw_set_ypos(yb);
   ghw_set_xpos(xb);
   #endif

   msk = startmask[GPIXEL(xb)];
   mske = stopmask[GPIXEL(xe)];
   xe = GXBYTE(xe);

   for (x = GXBYTE(xb); x <= xe; x++ )
      {
      if (x == xe)
         msk &= mske;

      /* Write destination */
      #ifdef GBUFFER
      if (msk == GHWCOLOR_NO_MSK)
         gbuf[gbufidx] = color;
      else
         {
         gbuf[gbufidx] = (color & msk) | (gbuf[gbufidx] & ~msk);
         msk = GHWCOLOR_NO_MSK;
         }
      gbufidx++;
      #else
      if (msk == GHWCOLOR_NO_MSK)
         ghw_auto_wr(color);
      else
         {
         ghw_auto_wr((color & msk) | (ghw_rd_x(x*(GDISPHCW/GDISPPIXW)) & ~msk));
         msk = GHWCOLOR_NO_MSK;
         }
      #endif
      }
   #ifdef GBUFFER
   if (ghw_upddelay == 0)
   #endif
      ghw_updatehw();
   }

/*
   Draw vertical line
*/
static void ghw_linev(GXT xb, GYT yb, GYT ye, GHWCOLOR color)
   {
   GFAST GHWCOLOR msk;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   #else
   GFAST GYT y,yr;
   #endif

   if ((xb >= GDISPW) || (yb >= GDISPH)) return;
   if (ye >= GDISPH) ye = GDISPH-1;

   #ifdef GBUFFER
   invalx( xb );
   invaly( yb );
   invaly( ye );
   gbufidx = GINDEX(xb,yb);
   #endif

   msk = pixmsk[GPIXEL(xb)];
   color &= msk;

   #ifdef GBUFFER
   for (; yb <= ye; yb++)
      {
      gbuf[gbufidx] = color | (gbuf[gbufidx] & ~msk);
      gbufidx += GDISPSW;
      }
   #else
   while (yb <= ye )
      {
      y = ye - yb;
      if (y >= sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0]))
         y = sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0])-1;

      /* Set one byte wide range, and read to tmp buffer  */
      ghw_set_yrange(yb,yb+y);
      ghw_set_xrange(xb,xb);
      ghw_auto_rd_start();
      for ( yr = 0; yr <= y; yr++)
          {
          ghw_tmpbuf[yr] = ghw_auto_rd();
          }

      /* Do modify-write via tmp buffer */
      ghw_set_yrange(yb,yb+y);
      ghw_set_xrange(xb,xb);
      for ( yr = 0; yr <= y; yr++)
         ghw_auto_wr( color | (ghw_tmpbuf[yr] & ~msk));
      yb += y+1;
      }
   #endif
   #ifdef GBUFFER
   if (ghw_upddelay == 0)
   #endif
      ghw_updatehw();
   }

#endif /* GHW_USING_VBYTE */

/*
   Provides accelerated line drawing for horizontal/vertical lines.

   If left-top and right-bottom is on a single vertical or horizontal
   line a single line is drawn.

   All coordinates are absolute coordinates.
*/
void ghw_rectangle(GXT ltx, GYT lty, GXT rbx, GYT rby, GCOLOR color)
   {
   GHWCOLOR col;
   glcd_err = 0;

   GBUF_CHECK();

   /* Speed color handling by expanding color to all pixels in storage unit */
   col = ghw_prepare_color(color);

   if (ltx != rbx)
      ghw_lineh(ltx, lty, rbx, col);      /* Draw horisontal line */

   if (lty != rby)
      {
      ghw_linev(ltx, lty, rby, col);      /* Draw vertical line */
      if (ltx != rbx)
         {                                  /* It is box coordinates */
         ghw_lineh(ltx, rby, rbx, col);   /* Draw bottom horizontal line */
         ghw_linev(rbx, lty, rby, col);   /* Draw right vertical line */
         }
      }
   }
#endif /* GGRAPHICS */

