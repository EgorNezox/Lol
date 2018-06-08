/************************** ghwsymrd.c *****************************

   Graphic symbol read functions for LCD display

   Read graphic area from the display to a GLCD buffer using the general
   symbol format.

   The byte ordering for a symbol is horizontal byte(s) containing the
   first pixel row at the lowest address followed by the byte(s) in
   the pixel row below etc. The symbol is left aligned in the byte buffer.

   All coordinates are absolute pixel coordinate.

   ---------

   The SSD0323 controller is assumed to be used with a LCD module.

   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is complied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)


   Creation date:
   Revision date:    270405
   Revision Purpose: The bw parameter to ghw_rdsym(..) changed to SGUINT
                     to accomondate all combinations of display size and
                     pixel resolutions.
   Revision date:    11-07-13
   Revision Purpose: General optimization of ghw_rdsym interface.
   Revision date:    27-02-2015
   Revision Purpose: Support for use of vertical storage units
                     and 90-(270) degree rotation added
   Revision date:
   Revision Purpose:

   Version number: 1.3
   Copyright (c) RAMTEX Engineering Aps 2004-2015

*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */
#ifdef GSOFT_SYMBOLS

/* High-level symbol formats */
#if ((GDISPPIXW != 4) && (GDISPPIXW != 2) && (GDISPPIXW != 1))
   #error GDISPPIXW bit-pr-pixel mode not supported (illegal gdispcfg.h)
#endif

/*
   Copy a graphic area from the display to a buffer organized with the
   common symbol and font format. The default pixel resolution is used.

   Returns bit-pr-pixel used in saved symbol storage units.
*/
SGUCHAR ghw_rdsym(GXT ltx, GYT lty, GXT rbx, GYT rby, PGUCHAR dest)
   {
   SGUINT bw;
   GHWCOLOR olddat;
   #ifdef GHW_USING_VBYTE
   SGUCHAR msk;
   GXT x,xs;
   GYT y,ys;
   SGUCHAR spshift;
   #ifdef GBUFFER
   GHWCOLOR *p;
   #endif
   #else
   GXT x,sidx;
   SGUCHAR dat,tmp;
   SGUCHAR spshift,pshift;

   #ifdef GBUFFER
   GBUFINT gbufidx;
   #endif
   #endif

   glcd_err = 0;
   /* Skip if illegal or completely outside screen */
   if ((dest == NULL) || (ltx >= GDISPW) || (lty >= GDISPH) || (rby < lty) || (rbx < ltx))
      return 0; /* Illegal color type indicates error */

   /* Force reasonable values */
   GLIMITU(rby,GDISPH-1);
   GLIMITU(rbx,GDISPW-1);

   /* Symbol row byte width (the symbol orientation is always hardware independent) */
   bw = ((SGUINT)(rbx-ltx+1)+((8/GDISPPIXW)-1))/(8/GDISPPIXW);

   #ifdef GBUFFER
   invalrect( ltx, lty );
   invalrect( rbx, rby );
   #else
   /* Set both x,y ranges in advance and take advantage of
      the controllers auto wrap features */
   ghw_set_yrange(lty,rby);
   ghw_set_xrange(ltx,rbx);
   ghw_auto_rd_start();
   #endif

   #ifdef GHW_USING_VBYTE

   for (ys = 0; lty<=rby; lty=GPIXMSK(lty)+GHWPIXPSU)
      {
      /* Vertical screen byte loop */
      #ifdef GBUFFER
      p = &gbuf[GINDEX(ltx, lty)];
      #endif

      for (xs = 0, x=ltx, spshift=0; x <= rbx; x++, xs++)
         {
         /* Horizontal video byte loop */
         SGUCHAR *ps;
         #ifdef GBUFFER
         olddat = *p++;
         #else
         olddat = ghw_auto_rd();
         #endif

         #if !defined( GHW_INVERTGRAPHIC_SYM )
         olddat ^= GHWCOLOR_NO_MSK; /* Color symbols created with 0 as black, invert */
         #endif

         /* Location of symbol byte column */
         ps = &dest[xs/(8/GDISPPIXW)+ys*bw];
         if (spshift == 0)
            spshift = 8-GDISPPIXW;
         else
            spshift -= GDISPPIXW;

         /* Vertical storage unit to symbol pixel processon loop */
         for (y = lty, msk = ~(GPIXMAX << spshift);;ps = &ps[bw])
            {
            register SGUCHAR dat;
            /* normalize data pixel */
            dat = (SGUCHAR)((olddat >> shiftmsk[GPIXEL(y)]) & GPIXMAX);
            /* Position pixel for symbol */
            dat <<= spshift;
            /* Append to symbol data */
            *ps = (*ps & msk) | dat;
            y++;
            if ((GPIXEL(y) == 0) || (y>rby))
               break;
            }
         }
      ys += (y-lty);
      }

   #else  /* GHW_USING_VBYTE */

   for (spshift = (8-GDISPPIXW); lty <= rby; lty++)
      {
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty); /* Calculate buffer index for line start */
      #endif

      /* Loop video bytes */
      for (x=ltx, sidx = 0, pshift = 0, dat = 0, olddat = 0; x <= rbx; x++)
         {
         if (pshift == 0)  /* Last pixel in video byte ? */
            {
            #ifdef GBUFFER
            olddat = gbuf[gbufidx++];
            #else
            olddat = ghw_auto_rd();
            #endif
            pshift = (~x & (GHWPIXPSU-1))*GDISPPIXW;

            #if !defined( GHW_INVERTGRAPHIC_SYM )
            olddat ^= GHWCOLOR_NO_MSK; /* Color symbols created with 0 as black, invert */
            #endif
            }
         else
            pshift -= GDISPPIXW;

         /* Assemble storage byte */
         tmp = (olddat >> shiftmsk[GPIXEL(x)]) & GPIXMAX; /* normalize pixel */

         /* add bit to storage byte */
         dat |= (tmp << spshift);

         if ((spshift == 0) || (x == rbx))
            {
            /* Data is complete */
            dest[sidx++] = dat;
            spshift = (8-GDISPPIXW);
            dat = 0;
            continue;
            }

         /* More video bytes are needed to assemble data byte */
         spshift -= GDISPPIXW;
         }
      dest = &dest[bw]; /* Point to next symbol row */
      }
   #endif  /* GHW_USING_VBYTE */
   return GDISPPIXW;  /* This controller use matching symbol format (1,2,4) */
   }


#endif
