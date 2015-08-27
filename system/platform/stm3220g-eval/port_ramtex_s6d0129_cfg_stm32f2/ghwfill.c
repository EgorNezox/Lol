/************************** ghwfill.c *****************************

   Fill box area with a pattern.

   The box area may have any pixel boundary, however the pattern is
   always aligned to the physical background, which makes patching
   of the background easier with when using multiple partial fills.

   The pattern word is used as a 2 character pattern.
   The LSB byte of pattern are used on even pixel lines and the MSB byte
   are used on odd pixel lines making it easy to make a homogene two color
   bit raster (for instance when pat = 0x55aa or = 0xaa55)

   ---------

   The s6d0129 controller is assumed to be used with a LCD module.

   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is compied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Revision date:     11-11-10
   Revision Purpose:  Corrected bug so patterns are uniform across the screen.
                      Accelerated fill with pattern = 'all-foreground' or 'all-background'.

   Version number: 1.01
   Copyright (c) RAMTEX Engineering Aps 2007-2010

*********************************************************************/
#include <s6d0129.h>   /* s6d0129 controller specific definements */

#ifdef GVIEWPORT

#if ( defined( GHW_ACCELERATOR ) && defined (GBUFFER) )
extern SGBOOL ghw_upddelay;
#endif

void ghw_fill(GXT ltx, GYT lty, GXT rbx, GYT rby, SGUINT pattern)
   {
   GYT y;
   GXT x;
   #ifdef GBUFFER
   GCOLOR *cp;
   GBUF_CHECK();
   #endif

   glcd_err = 0;

   /* Force reasonable values */
   GLIMITU(ltx,GDISPW-1);
   GLIMITU(lty,GDISPH-1);
   GLIMITD(rby,lty);
   GLIMITU(rby,GDISPH-1);
   GLIMITD(rbx,ltx);
   GLIMITU(rbx,GDISPW-1);

   #ifdef GBUFFER
   invalrect( ltx, lty );
   invalrect( rbx, rby );
   #else
   ghw_set_xyrange(ltx,lty,rbx,rby);
   #endif

   if ((pattern == 0) || (pattern == 0xffff))
      {
      /* Accelerated loop fill for uniform color */
      GCOLOR c = (pattern != 0) ? ghw_def_foreground : ghw_def_background;
      for (y = lty; y <= rby; y++)
         {
         #ifdef GBUFFER
         cp = &gbuf[GINDEX(ltx,y)];
         #endif
         x = rbx-ltx;
         do
            {
            #ifdef GBUFFER
            *cp++ = c;
            #else
            ghw_auto_wr(c);
            #endif
            }
         while (x-- != 0);
         }
      }
   else
      {
      /* loop fill for pattern background (mixed foreground / background) */
      for (y = lty; y <= rby; y++)
         {
         SGUCHAR msk,pat;
         pat = ((y & 1) != 0) ? (SGUCHAR)(pattern / 256) : (SGUCHAR)(pattern & 0xff);
         msk = sympixmsk[GPIXEL(ltx)];
         #ifdef GBUFFER
         cp = &gbuf[GINDEX(ltx,y)];
         #endif
         for (x = ltx; x <= rbx; x++ )
            {
            #ifdef GBUFFER
            *cp++ = (pat & msk) ? ghw_def_foreground : ghw_def_background;
            #else
            ghw_auto_wr(((pat & msk) ? ghw_def_foreground : ghw_def_background));
            #endif
            if ((msk >>= 1) == 0)
               msk = sympixmsk[GPIXEL(x+1)];
            }
         }
      }
   }

#endif /* GBASIC_TEXT */

