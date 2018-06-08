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

   The SSD0323 controller is assumed to be used with a OLED module.

   The following OLED module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the OLED hardware.
              The memory buffer content is compied to the OLED
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Revision date:    27-02-2015
   Revision Purpose: Support for use of vertical storage units
                     and 90-(270) degree rotation added
                     Skip drawing if completely outside screen area, else limit to within.
   Revision date:    19-07-2017
   Revision Purpose: Pattern calculation corrected. Can now correctly handle 
                     different storage unit sizes used by the different display 
                     controller variants.

   Version number: 1.11
   Copyright (c) RAMTEX Engineering Aps 2004-2017

*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

#ifdef GVIEWPORT


static void ghw_setfill_pattern( SGUINT pattern )
   {
   GFAST GHWCOLOR val;
   #ifdef GHW_USING_VBYTE
   /* Pattern is distributed over 8 vertical bytes aligned to absolute positions */
   GXT x;
   /* Convert fill pattern to color settings */
   if ((pattern == 0x0000) || (pattern == 0xffff))
      {
      /* Uniform color, used fast setting */
      val = (pattern) ? ghw_foreground : ghw_background;
      for (x=0; x < 8; x++)
        ghw_tmpbuf[x] = val;
      }
   else
      {
      /* Mixed color pattern */
      SGUINT mskp;
      GHWCOLOR pmsk;
      GYT ytmp;

      for (mskp = 0x80, x=0; mskp != 0; mskp >>= 1,x++)
         {
         /* default is background color */
         ghw_tmpbuf[x] = ghw_background;
         /* Add foreground color to even pixels if bit is 1 */
         if ((pattern & (mskp<<8)) != 0)
            {
            for (ytmp = 0; ytmp < GHWPIXPSU; ytmp+=2)
               {
               pmsk = pixmsk[ytmp];
               ghw_tmpbuf[x] = (ghw_tmpbuf[x] & ~pmsk) | (ghw_foreground & pmsk);
               }
            }
         /* Add foreground color to odd pixels if bit is 1 */
         if ((pattern & mskp) != 0)
            {
            for (ytmp = 1; ytmp < GHWPIXPSU; ytmp+=2)
               {
               pmsk = pixmsk[ytmp];
               ghw_tmpbuf[x] = (ghw_tmpbuf[x] & ~pmsk) | (ghw_foreground & pmsk);
               }
            }
         }
      }

   #else
   /* Pattern is distributed over horizontal bytes aligned to absolute positions */
   GXT x;
   /* Convert fill pattern to color settings */
   if ((pattern == 0x0000) || (pattern == 0xffff))
      {
      /* Uniform color, used fast setting */
      val = (pattern) ? ghw_foreground : ghw_background;
      for (x=0; x < (2*GDISPPIXW)/sizeof(GHWCOLOR); x++)
        ghw_tmpbuf[x] = val;
      }
   else
      {
      /* Mixed color pattern */
      SGUINT mskp;
      GHWCOLOR pmsk;
      mskp = 0x8000;
      x=0;
      do
         {
         /* default is background color */
         val = ghw_background;
         for(;;)
            {
            if ((pattern & mskp) != 0)
               {
               /* Add foreground color if bit is 1 */
               pmsk = pixmsk[GPIXEL(x)];
               val = (val & ~pmsk) | (ghw_foreground & pmsk);
               }
            mskp >>= 1;
            if (GPIXEL(x+1) == 0)
               break;  /* Color byte is assembled */
            x++;
            }
         ghw_tmpbuf[GXBYTE(x)] = val;
         x++;
         }
      while(mskp != 0);
      }
   #endif
   }

void ghw_fill(GXT ltx, GYT lty, GXT rbx, GYT rby, SGUINT pattern)
   {
   #ifndef GHW_USING_VBYTE
   GHWCOLOR mskb;
   GHWCOLOR mske;
   #endif
   GXT x;
   GFAST GHWCOLOR val,msk;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   GBUF_CHECK();
   #else
   #ifdef GHW_USING_VBYTE
   GXT xb;
   GXT xbe = 0; /* Silence warning */
   #endif
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

   /* Form pattern for fill */
   ghw_setfill_pattern(pattern);

   #ifdef GHW_USING_VBYTE
   msk = startmask[GPIXEL(lty)];
   for (lty = GPIXMSK(lty);;)
      {
      /* Mask bits outside boundaries */
      if (lty == GPIXMSK(rby))
         msk &= stopmask[GPIXEL(rby)];

      /* loop fill of whole bytes */
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty);
      for (x = ltx; x <= rbx; x++ )
         {
         val = ghw_tmpbuf[x & (8-1)];
         if (msk != GHWCOLOR_NO_MSK)
            val = (gbuf[gbufidx] & ~msk) | (val & msk);
         gbuf[gbufidx++] = val;
         }
      #else
      ghw_set_yrange(lty,lty); /* Limit range to line, prevent auto wrap at x edges */
      ghw_set_xpos(ltx);
      for (x = ltx, xb = 0; x <= rbx; x++ )
         {
         /* Get row pattern for column (aligned to screen coordinates) */
         val = ghw_tmpbuf[ x & (8-1) ];
         if (msk != GHWCOLOR_NO_MSK)
            {
            /* Read-modify-write operations needed */
            if (xb == 0)
               {  /* prefetch read data to temp buffer */
               xbe = rbx-x; /* (remaining) line length */
               if (xbe >= (sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0]))-8)
                  xbe = (sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0]))-9;
               ghw_auto_rd_start();
               for ( ; xb <= xbe; xb++)
                  ghw_tmpbuf[xb+8] = ghw_auto_rd();
               ghw_set_xpos(x);
               xb = 0;
               }
            val = (ghw_tmpbuf[xb+8] & ~msk) | (val & msk);
            if (xb++ == xbe)
               xb = 0;
            }
         ghw_auto_wr(val);
         }
      #endif
      if (lty == GPIXMSK(rby))
          break; /* Normal exit */
      lty += GHWPIXPSU;
      msk = GHWCOLOR_NO_MSK;
      }

   #else  /* GHW_USING_VBYTE */

   mske = stopmask[GPIXEL(rbx)];
   mskb = startmask[GPIXEL(ltx)];
   rbx = GXBYTE(rbx);

   for (; lty <= rby; lty++)
      {
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty);
      #else
      ghw_set_yrange(lty,lty); /* Limit range to line, prevent auto wrap at x edges */
      ghw_set_xpos(ltx);
      #endif
      msk = mskb;

      /* loop fill of whole bytes */
      for (x = GXBYTE(ltx); x <= rbx; x++ )
         {
         /* Mask bits outside boundaries */
         SGUCHAR i;
         if (x == rbx)
            msk &= mske;
//         i = (x%(8/GDISPPIXW)) + (GBUFINT) (8/GDISPPIXW)*(lty & 0x1);
         i = (x%(GDISPPIXW/sizeof(GHWCOLOR))) + (GDISPPIXW/sizeof(GHWCOLOR))*(lty & 0x1);
         val = ghw_tmpbuf[i];
//         val = ghw_tmpbuf[i(x%(8/GDISPPIXW)) + (GBUFINT) (8/GDISPPIXW)*(lty & 0x1)];

         #ifdef GBUFFER
         if (msk != GHWCOLOR_NO_MSK)
            {
            val = (gbuf[gbufidx] & ~msk) | (val & msk);
            msk = GHWCOLOR_NO_MSK;
            }
         gbuf[gbufidx++] = val;
         #else
         if (msk != GHWCOLOR_NO_MSK)
            {
            val = (ghw_rd_x(x*GHWPIXPSU) & ~msk) | (val & msk);
            msk = GHWCOLOR_NO_MSK;
            }
         ghw_auto_wr(val);
         #endif
         }
      }
   #endif /* GHW_USING_VBYTE */
   }

#endif /* GBASIC_TEXT */

