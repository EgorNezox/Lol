/************************** ghwgscrl.c *****************************

   Scrolls the graphics on LCD x lines up.  The empty area in the
   bottom is cleared with a pattern.

   ---------

   The s6d0129 controller is assumed to be used with a LCD module.

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
   Revision date:
   Revision Purpose:

   Version number: 1.0
   Copyright (c) RAMTEX Engineering Aps 2007

*********************************************************************/
#include <s6d0129.h>    /* s6d0129 controller specific definements */

#if defined( GBASIC_TEXT ) || defined(GSOFT_FONTS) || defined(GGRAPHIC)
#if (defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT))
/*
   Scrolls the graphics on LCD x lines up.
   The empty area in the bottom is cleared

   lines  =  pixel lines to scroll
*/
void ghw_gscroll(GXT ltx, GYT lty, GXT rbx, GYT rby, GYT lines, SGUINT pattern)
   {
   #ifdef GBUFFER
   GYT ylim;
   GXT x;
   GBUFINT gbufidx;
   GBUFINT source;
   GBUF_CHECK();
   #else  /* GBUFFER */
   GYT ys;
   GCOLOR *cp;
   GYT ylim;
   GXT x;
   #endif  /* GBUFFER */

   glcd_err = 0;

   /* Force resoanable values */
   GLIMITU(ltx,GDISPW-1);
   GLIMITU(lty,GDISPH-1);
   GLIMITD(rby,lty);
   GLIMITU(rby,GDISPH-1);
   GLIMITD(rbx,ltx);
   GLIMITU(rbx,GDISPW-1);

   #ifdef GBUFFER
   /* Buffered mode  */
   invalrect( ltx, lty );
   invalrect( rbx, rby );

   if (lines > rby - lty)
      {
      ghw_fill(ltx, lty, rbx, rby, pattern);   /* just clear whole area */
      return;
      }

   ylim = rby - lines;
   for (; lty <= rby; lty++)
      {
      /* Loop rows */
      if (lty >= ylim)
         {
         ghw_fill(ltx, lty, rbx, rby, pattern);   /* clear remaining area */
         return;
         }

      gbufidx = GINDEX(ltx,lty);
      source = gbufidx + (GBUFINT)lines * GDISPW;
      for (x = ltx; x <= rbx; x++)
         {
         /* Loop pixel columns in row */
         gbuf[gbufidx++] = gbuf[source++];
         }
      }

   #else
   /* Non-buffered mode */
   if (lines > rby - lty)
      {
      ghw_fill(ltx, lty, rbx, rby, pattern);   /* just clear whole area */
      return;
      }

   ylim = rby - lines;
   ys = (lty+lines);        /* First source row for scroll */

   for (; lty <= rby; lty++, ys++)
      {
      /* Loop rows */
      if (lty > ylim)
         {
         ghw_fill(ltx, lty, rbx, rby, pattern);   /* clear remaining area */
         return;
         }

      #ifdef GHW_NO_RDINC
      ghw_set_xyrange(ltx,ys,rbx,ys);  /* Initiate LCD controller address pointers*/
      /* Loop pixel row read */
      for (x = ltx, cp = &ghw_tmpbuf[0]; x <= rbx; x++,cp++)
         *cp = ghw_rd(x,ys);
      ghw_set_xyrange(ltx,lty,rbx,lty);  /* Initiate LCD controller address pointers*/
      #else
      ghw_setxypos(ltx,ys);
      ghw_auto_rd_start();
      /* Loop pixel row read */
      for (x = ltx, cp = &ghw_tmpbuf[0]; x <= rbx; x++,cp++)
         *cp = ghw_auto_rd();
      ghw_setxypos(ltx,lty);
      #endif
      /* Loop pixel row write */
      for (x = ltx, cp = &ghw_tmpbuf[0]; x <= rbx; x++,cp++)
         ghw_auto_wr(*cp);
      }
   #endif
   }

#endif /* defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT) */
#endif /* GBASIC_TEXT */

