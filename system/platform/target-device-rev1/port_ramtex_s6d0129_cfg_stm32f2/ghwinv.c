/************************** ghwinv.c *****************************

   Invert box area i.e. swap between two colors

   The box area may have any pixel boundary

   ---------

   The S6D0129 controller is assumed to be used with a LCD module.

   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is compied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Revision date:
   Revision Purpose:

   Version number: 1.00
   Copyright (c) RAMTEX Engineering Aps 2007

*********************************************************************/
#include <s6d0129.h>   /* s6d0129 controller specific definements */

#if (!defined( GNOCURSOR ) && defined (GSOFT_FONTS )) || defined (GGRAPHICS)
#if (defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT))

void ghw_invert(GXT ltx, GYT lty, GXT rbx, GYT rby)
   {
   GXT x;
   register GCOLOR color, fore, back;
   #ifdef GBUFFER
   GBUFINT gbufidx;
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
   #endif
   fore = ghw_def_foreground & GHW_COLOR_CMP_MSK;
   back = ghw_def_background & GHW_COLOR_CMP_MSK;

   for (; lty <= rby; lty++)
      {
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty);
      /* loop invert of colors */
      for (x = ltx; x <= rbx; x++)
         {
         color = gbuf[gbufidx];
         /* Swap foreground and background colors */
         if ((color & GHW_COLOR_CMP_MSK) == fore )
            gbuf[gbufidx] = ghw_def_background;
         else
         if ((color & GHW_COLOR_CMP_MSK) == back )
            gbuf[gbufidx] = ghw_def_foreground;
         gbufidx++;
         }
      #else

      #ifdef GHW_NO_RDINC
      ghw_set_xyrange(ltx,lty,rbx,lty);
      for (x = ltx; x <= rbx; x++)
         {
         color = ghw_rd(x,lty);

      #else /* GHW_NO_RDINC */
      GCOLOR *cp;
      /* Loop pixel row read */
      ghw_setxypos(ltx,lty);
      ghw_auto_rd_start();
      /* Loop pixel row read */
      for (x = ltx, cp = &ghw_tmpbuf[0]; x <= rbx; x++, cp++)
         *cp = ghw_auto_rd();

      ghw_setxypos(ltx,lty);
      for (x = ltx, cp = &ghw_tmpbuf[0]; x <= rbx; x++,cp++)
         {
         color = *cp;
         #endif /* GHW_NO_RDINC */

         /* Swap foreground and background colors */
         if ((color & GHW_COLOR_CMP_MSK) == fore )
            color = ghw_def_background;
         else
         if ((color & GHW_COLOR_CMP_MSK) == back )
            color = ghw_def_foreground;
         ghw_auto_wr(color);
         }
      #endif
      }
   }

#endif /* defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT) */
#endif /* GBASIC_TEXT */

