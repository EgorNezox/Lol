/************************** ghwpixel.c *****************************

   Low-level functions for graphic pixel set and clear
   Absolute coordinates are used.

   The UC161x controller is assumed to be used with a LCD module.
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
   Revision date:     4-1-05
   Revision Purpose:  Speed optimization of read-modify-write operations
                      in non-buffered mode
   Revision date:    11-11-2011
   Revision Purpose: Customized for use with horizontal storage units.
   Revision date:    16-03-2012
   Revision Purpose: Speed improvement by new position handling
   Revision date:    04-03-2015
   Revision Purpose: Support for use of vertical storage units
                     and 90-(270) degree rotation added
   Revision Purpose: Throwing away pixels outside screen area instead of limiting
   Revision date:
   Revision Purpose:

   Version number: 1.5
   Copyright (c) RAMTEX Engineering Aps 2004-2015

*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

#ifdef GGRAPHICS

#ifdef GHW_USING_VBYTE
  #define  GPIXELOFF(x,y) GPIXEL(y)
#else
  #define  GPIXELOFF(x,y) GPIXEL(x)
#endif

/*
   Set pixel color
*/
void ghw_setpixel( GXT x, GYT y, GCOLOR color )
   {
   GHWCOLOR col;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   GBUF_CHECK();
   #endif

   glcd_err = 0;

   /* Skip if completely outside */
   if ((y >= GDISPH) || (x >= GDISPW)) return;

   col = (GHWCOLOR)((color & GPIXMAX) << shiftmsk[GPIXELOFF(x,y)]);

   #ifdef GBUFFER
   gbufidx = GINDEX(x,y);
   gbuf[gbufidx] = ((gbuf[gbufidx] & ~pixmsk[GPIXELOFF(x,y)]) | col);
   invalrect( x, y );
   #else
   ghw_set_yrange(y,y);
   ghw_set_xrange(x,x); /* Init position for read */
   ghw_auto_rd_start();
   col = (ghw_auto_rd() & ~pixmsk[GPIXELOFF(x,y)]) | col;
   ghw_set_xrange(x,x); /* Restore position for write */
   ghw_auto_wr(col);
   #endif
   }

/*
   Get pixel color
*/
GCOLOR ghw_getpixel(GXT x, GYT y)
   {
   glcd_err = 0;

   /* Skip if completely outside, return dummy */
   if ((y >= GDISPH) || (x >= GDISPW)) return 0;

   #ifdef GBUFFER
   #ifdef GHW_ALLOCATE_BUF
   if (gbuf == NULL)
      {
      glcd_err = 1;
      return 0;
      }
   #endif
   /* Calculate byte index */
   return (GCOLOR) ((gbuf[GINDEX(x,y)] >> shiftmsk[GPIXELOFF(x,y)]) & GPIXMAX);
   #else
   ghw_set_yrange(y,y);
   ghw_set_xrange(x,x);
   ghw_auto_rd_start();
   return (GCOLOR) ((ghw_auto_rd() >> shiftmsk[GPIXELOFF(x,y)]) & GPIXMAX);
   #endif
   }
#endif
