/************************** ghwinv.c *****************************

   Invert box area i.e. swap between two colors

   The box area may have any pixel boundary

   ---------

   The SSD0323 controller is assumed to be used with a LCD module.

   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is compied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)


   Creation date:
   Revision date:    4-1-05
   Revision Purpose: Speed optimization of read-modify-write operations
                     in non-buffered mode
   Revision date:    27-02-2015
   Revision Purpose: Support for use of vertical storage units
                     and 90-(270) degree rotation added
                     Skip drawing if completely outside screen area, else limit to within.
   Revision date:
   Revision Purpose:

   Version number: 1.10
   Copyright (c) RAMTEX Engineering Aps 2004-2015

*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

#if (!defined( GNOCURSOR ) && defined (GSOFT_FONTS )) || defined (GGRAPHICS)

void ghw_invert(GXT ltx, GYT lty, GXT rbx, GYT rby)
   {
   #ifdef GHW_USING_VBYTE
   GHWCOLOR *cp;
   GYT y;
   #endif
   GXT x;
   GFAST GHWCOLOR val;
   #ifdef GBUFFER
   #ifndef GHW_USING_VBYTE
   GBUFINT gbufidx;
   #endif
   GBUF_CHECK();
   #endif

   glcd_err = 0;

   /* Skip if illegal or completely outside screen */
   if ((ltx >= GDISPW) || (lty >= GDISPH) || (rby < lty) || (rbx < ltx)) return;
   /* Force reasonable values */
   GLIMITU(rby,GDISPH-1);
   GLIMITU(rbx,GDISPW-1);

   #ifdef GBUFFER
   invalrect( ltx, lty );
   invalrect( rbx, rby );
   #endif

   #ifdef GHW_USING_VBYTE

   for (;lty <= rby; lty=GPIXMSK(lty)+GHWPIXPSU)
      {
      GXT xb,xr;
      #ifdef GBUFFER
      cp = &gbuf[GINDEX(ltx,lty)];
      #else
      cp = &ghw_tmpbuf[0];
      ghw_set_yrange(lty,lty);
      #endif

      for (xb = ltx; xb <= rbx;)
         {
         x = rbx - xb; /* (remaining) line length */
         #ifndef GBUFFER
         if (x >= sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0]))
            x =  sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0])-1;

         ghw_set_xpos(xb);
         ghw_auto_rd_start();
         for ( xr = 0; xr <= x; xr++)  /* Read to buffer */
            cp[xr] = ghw_auto_rd();
         #endif

         /* Process buffer */
         for ( xr = 0; xr <= x; xr++)
            {
            y = lty;
            val = cp[xr];
            /* Loop pixels in video byte and swap foreground and background color */
            for(;;)
               {
               GFAST GHWCOLOR pmsk;
               pmsk = pixmsk[GPIXEL(y)];
               if ((val & pmsk) == (ghw_foreground & pmsk))
                  val = (val & ~pmsk) | (ghw_background & pmsk);
               else
               if ((val & pmsk) == (ghw_background & pmsk))
                  val = (val & ~pmsk) | (ghw_foreground & pmsk);

               if ((GPIXEL(y) == GPIXEL(GHWCOLOR_NO_MSK)) || (y == rby)) /* = max pixel value or = end of area */
                  break;  /* Video byte is processed */
               y++;
               }
            cp[xr] = val;
            }

         #ifndef GBUFFER
         ghw_set_xpos(xb);
         for ( xr = 0; xr <= x; xr++)  /* Process buffer */
            ghw_auto_wr( cp[xr] );
         #endif

         xb+=x+1;
         }
      }
   #else  /* GHW_USING_VBYTE */

   for (;lty <= rby; lty++)
      {
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty);
      #else
      ghw_set_yrange(lty,lty);
      ghw_set_xpos(ltx);
      #endif

      /* loop invert of whole bytes */
      for (x = ltx; x <= rbx; x++)
         {
         #ifdef GBUFFER
         val = gbuf[gbufidx];
         #else
         val = ghw_rd_x(x);  /* Read data, keep position unmodified */
         #endif

         /* Loop pixels in video byte and swap foreground and background color */
         for(;;)
            {
            GFAST GHWCOLOR pmsk;
            pmsk = pixmsk[GPIXEL(x)];

            if ((val & pmsk) == (ghw_foreground & pmsk))
               val = (val & ~pmsk) | (ghw_background & pmsk);
            else
            if ((val & pmsk) == (ghw_background & pmsk))
               val = (val & ~pmsk) | (ghw_foreground & pmsk);

            if ((GPIXEL(x) == GPIXEL(GHWCOLOR_NO_MSK)) || (x == rbx)) /* = max pixel value or = end of area */
               break;  /* Video byte is processed */
            x++;
            }

         #ifdef GBUFFER
         gbuf[gbufidx] = val;
         gbufidx++;
         #else
         ghw_auto_wr(val);
         #endif
         }
      }
   #endif /* GHW_USING_VBYTE */
   }

#endif /* GBASIC_TEXT */

