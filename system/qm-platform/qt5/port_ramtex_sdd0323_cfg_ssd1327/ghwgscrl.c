/************************** ghwgscrl.c *****************************

   Scrolls the graphics on LCD x lines up.  The empty area in the
   bottom is cleared with a pattern.

   ---------

   The SSD0323 controller is assumed to be used with a LCD module.

   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is copied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Revision date:    27-02-2015
   Revision Purpose: Support for use of vertical storage units
                     and 90-(270) degree rotation added
   Revision date:
   Revision Purpose:

   Version number: 1.10
   Copyright (c) RAMTEX Engineering Aps 2004-2015

*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

/* fill pattern buffer located in ghwfill.c */
void ghw_setfill_pattern(SGUINT pattern);

#if defined( GBASIC_TEXT ) || defined(GSOFT_FONTS) || defined(GGRAPHIC)

#define  TMPBUF_HALFSIZE ((sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0]))/2)

/*
   Scrolls the graphics on LCD x lines up.
   The empty area in the bottom is cleared

   lines  =  pixel lines to scroll
*/
void ghw_gscroll(GXT ltx, GYT lty, GXT rbx, GYT rby, GYT lines, SGUINT pattern)
   {
   GHWCOLOR msk;
   GFAST GYT ys,ylim;
   GFAST GXT x,xc,xcb;
   #ifdef GHW_USING_VBYTE
   GHWCOLOR msklim,mske;
   SGUCHAR tidx;
   SGINT shift;
   GHWCOLOR *ghw_tmpbuf2;  /* Used for spliting ghw_tmp buf in two parts */
   GHWCOLOR fill;
   GFAST GYT y;
   #else
   GHWCOLOR mskb;
   GHWCOLOR *tp;
   GXT xe;
   #endif
   #ifdef GBUFFER
   GHWCOLOR *p;
   GBUF_CHECK();
   #else
   #endif

   glcd_err = 0;

   /* Force resoanable values */
   GLIMITU(ltx,GDISPW-1);
   GLIMITU(lty,GDISPH-1);
   GLIMITD(rby,lty);
   GLIMITU(rby,GDISPH-1);
   GLIMITD(rbx,ltx);
   GLIMITU(rbx,GDISPW-1);

   #ifdef GBUFFER
   invalrect( ltx, lty );
   invalrect( rbx, rby );
   #endif

   if (lines > rby - lty)
      {
      ghw_fill(ltx, lty, rbx, rby, pattern);   /* just clear whole area */
      return;
      }

   ylim = rby - lines;


   #ifdef GHW_USING_VBYTE

   /* Only uniform pattern colors are used, so simplify handling here  */
   fill = (pattern != 0) ? ghw_foreground : ghw_background;

   msk = startmask[GPIXEL(lty)];
   mske = stopmask[GPIXEL(rby)];
   msklim = stopmask[GPIXEL(ylim)];

   /* Use ghw_tmpbuf as two halfsize buffers */
   ghw_tmpbuf2 = &ghw_tmpbuf[TMPBUF_HALFSIZE];

   /* Set shift value (negative = shift left) */
   shift = (SGINT) shiftmsk[GPIXEL(lty + lines)] - (SGINT) shiftmsk[GPIXEL(lty)];

   ys = GYBYTE(lty+lines); /* First source row for scroll */
   rby = GYBYTE(rby);
   lty = GYBYTE(lty);
   ylim = GYBYTE(ylim);


   /* Loop byte rows */
   for (y = lty; y <= rby; y++,ys++)
      {
      if (y == rby)
         msk &= mske;  /* Use stop mask */

      for (x = ltx; x <= rbx; )
         {
         /* Loop colums.
           Operations is done a number of bytes at a time via the ghw_tmp
           buffer, in order to take advantage of the LCD
           controllers auto increment functionality.
           This give a factor 5 speed increase compared to
           an implementation which complete one column at a time */
         if ((xcb = rbx-x) > TMPBUF_HALFSIZE-1)
             xcb = TMPBUF_HALFSIZE-1;
         #ifdef GBUFFER
         p = &gbuf[GINDEX(x,ys*GHWPIXPSU)];
         #else
         /* Read data to temp buffers */
         ghw_set_ypos((GYT)(ys*GHWPIXPSU));
         ghw_set_xpos(x);
         ghw_auto_rd_start();
         #endif
         xc = xcb;
         tidx = 0;
         do
            {
            #ifdef GBUFFER
            ghw_tmpbuf[tidx++] = *p++;
            #else
            ghw_tmpbuf[tidx++] = ghw_auto_rd();
            #endif
            }
         while (xc-- != 0);

         if (((shift > 0) && (ys <= rby+((msklim != 0) ? 1 : 0))) ||
             ((shift < 0) && (ys != lty)))
            {
            GYT yt;
            #ifdef GHW_MIRROR_ML
            yt = (shift > 0) ? (ys+1) : (ys-1);
            #else
            yt = (shift > 0) ? (ys-1) : (ys+1);
            #endif

            #ifdef GBUFFER
            p = &gbuf[GINDEX(x,yt*GHWPIXPSU)];
            #else
            ghw_set_ypos((GYT)(yt*GHWPIXPSU));
            ghw_set_xpos(x);
            ghw_auto_rd_start();
            #endif
            xc = xcb;
            tidx = 0;
            do
               {
               #ifdef GBUFFER
               ghw_tmpbuf2[tidx++] = *p++;
               #else
               ghw_tmpbuf2[tidx++] = ghw_auto_rd();
               #endif
               }
            while (xc-- != 0);
            }

         xc = xcb;
         tidx = 0;
         do
            {
            /* Get source data */
            if (y <= ylim)
               {
               register GHWCOLOR val;
               val = ghw_tmpbuf[tidx];
               if (shift != 0)
                  { /* Combine two source storage units to one destination storage unit */
                  if (shift < 0)
                     {
                     val = (GHWCOLOR) (((((GHWCOLORD) val) << GDISPHCW) + ((GHWCOLORD) ghw_tmpbuf2[tidx])) >> (GDISPHCW+shift));
                     }
                  else
                     {
                     val = (GHWCOLOR)((((GHWCOLORD) val) + (((GHWCOLORD) ghw_tmpbuf2[tidx])<<GDISPHCW)) >> shift);
                     }
                  }

               if (y == ylim)
                  ghw_tmpbuf[tidx] = (val & msklim) | (fill & ~msklim);
               else
                  ghw_tmpbuf[tidx] = val;
               }
            else
               ghw_tmpbuf[tidx] = fill; /* use fill color */
            tidx++;
            }
         while(xc-- != 0);

         #ifndef GBUFFER
         ghw_set_ypos((GYT)(y*GHWPIXPSU));
         #endif
         if (msk != GHWCOLOR_NO_MSK)
            {
            /* Fetch byte and mask */
            #ifdef GBUFFER
            p = &gbuf[GINDEX(x,y*GHWPIXPSU)];
            #else
            ghw_set_xpos(x);
            ghw_auto_rd_start();
            #endif
            xc = xcb;
            tidx = 0;
            do
               {
               #ifdef GBUFFER
               ghw_tmpbuf[tidx] = (*p & ~msk) | (ghw_tmpbuf[tidx] & msk);
               p++;
               #else
               ghw_tmpbuf[tidx] = (ghw_auto_rd() & ~msk) | (ghw_tmpbuf[tidx] & msk);
               #endif
               tidx++;
               }
            while(xc-- != 0);
            }

         xc = xcb;
         tidx = 0;
         #ifdef GBUFFER
         p = &gbuf[GINDEX(x,y*GHWPIXPSU)];
         #else
         ghw_set_xpos(x);
         #endif
         do
            {
            #ifdef GBUFFER
            *p++ = ghw_tmpbuf[tidx++];
            #else
            ghw_auto_wr(ghw_tmpbuf[tidx++]);
            #endif
            }
         while(xc-- != 0);
         x += xcb+1;
         }
      msk = GHWCOLOR_NO_MSK;
      }

   #else /* GHW_USING_VBYTE */
   ys = (lty+lines);        /* First source row for scroll */
   mskb = startmask[GPIXEL(ltx)];
   xe = GXBYTE(rbx);

   for (;lty <= ylim; lty++,ys++)
      {
      /* Loop whole row bytes */
      msk = mskb;
      for (x = GXBYTE(ltx); x <= xe; x++)
         {
         #ifdef GBUFFER
         p = &gbuf[GINDEX(x*GHWPIXPSU,ys)];
         #else
         /* Read source data to temp buffer */
         ghw_set_yrange(ys,ys);
         ghw_set_xpos(x*GHWPIXPSU);
         ghw_auto_rd_start();
         #endif
         if ((xcb = xe-x) > sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0])-1)
            xcb = sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0])-1;
         xc = xcb;
         tp = &ghw_tmpbuf[0];
         do
            {
            #ifdef GBUFFER
            *tp++ = *p++;
            #else
            *tp++ = ghw_auto_rd();
            #endif
            }
         while (xc-- != 0);

         /* Write temp buffer to destination data */
         #ifdef GBUFFER
         p = &gbuf[GINDEX(x*GHWPIXPSU,lty)];
         #else
         ghw_set_yrange(lty,lty);
         ghw_set_xpos(x*GHWPIXPSU);
         #endif

         tp = &ghw_tmpbuf[0];
         xc = xcb;
         do
            {
            register GHWCOLOR val;
            if (x == xe)
               msk &= stopmask[GPIXEL(xe)];
            if (msk != GHWCOLOR_NO_MSK)
               {
               #ifdef GBUFFER
               val = (*p & ~mskb) | (*tp++ & mskb);
               #else
               val = (ghw_rd_x(x*GHWPIXPSU) & ~msk) | (*tp++ & msk);
               #endif
               msk = GHWCOLOR_NO_MSK;
               }
            else
               val = *tp++;
            #ifdef GBUFFER
            *p++ = val;
            #else
            ghw_auto_wr(val);
            #endif
            }
         while (xc-- != 0);
         x += xcb;
         }
      }

   /* Fill rest of area */
   ghw_fill(ltx, lty, rbx, rby, pattern);

   #endif /* GHW_USING_VBYTE */
   }
#endif /* GBASIC_TEXT */

