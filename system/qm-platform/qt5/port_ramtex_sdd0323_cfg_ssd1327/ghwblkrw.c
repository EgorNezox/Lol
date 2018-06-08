/************************** ghwblkrw.c *****************************

   Graphic block copy functions for OLED display

   Read graphic area from the display to a GLCD buffer.
   Write graphic buffer to OLED display.

   Information about the size of the graphic area is stored in the buffer.
   The buffer can be written back to the display with ghw_wrblk(),
   optionally with another start origin.

   All coordinates are absolute pixel coordinate.

   ---------

   The SSD0323 controller is assumed to be used with a OLED module.

   The following OLED module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the OLED hardware.
              The memory buffer content is complied to the OLED
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

#ifdef GBASIC_INIT_ERR
#if (defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT))

typedef struct
   {
   GXT lx;
   GYT ly;
   GXT rx;
   GYT ry;
   GHWCOLOR dat[1];
   } GHW_BLK_HEADER, *PGHW_BLK_HEADER;

/****************************************************************
 ** block functions
****************************************************************/

/*
   Calculate the needed size for the buffer used by ghw_rdblk()
   Return value can be used as parameter for buffer allocation with
   malloc.
   The coordinates to this function may be absolute or view-port relative
*/
GBUFINT ghw_blksize(GXT ltx, GYT lty, GXT rbx, GYT rby)
   {
   /* Force resonable values (assure that unsigned is poitive) */
   GLIMITD(rby,lty);
   GLIMITD(rbx,ltx);
   return GHW_BLK_SIZE(ltx,lty,rbx,rby);
   }

/*
   Copy a graphic area from the display to a GLCD buffer
   Information about the size of the graphic area is saved in the buffer.
   The buffer can be written back to the display with ghw_wrblk(),
   optionally with another start origin.

   All coordinates are absolute pixel coordinate.

   The first part of the buffer will be a dynamic header defining
   the block rectangle:
      GXT left_top_x,
      GYT left_top_y,
      GXT right_bottom_x,
      GYT right_bottom_y,
     followed by th block data
*/
void ghw_rdblk(GXT ltx, GYT lty, GXT rbx, GYT rby, SGUCHAR *dest, GBUFINT bufsize )
   {
   GXT x;
   PGHW_BLK_HEADER desthdr;
   GHWCOLOR *cp;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   GBUF_CHECK();
   #endif

   glcd_err = 0;
   if (dest == NULL)
      return;

   /* Force reasonable values */
   GLIMITU(ltx,GDISPW-1);
   GLIMITU(lty,GDISPH-1);
   GLIMITD(rby,lty);
   GLIMITU(rby,GDISPH-1);
   GLIMITD(rbx,ltx);
   GLIMITU(rbx,GDISPW-1);

   if (ghw_blksize(ltx,lty,rbx,rby) > bufsize)
      {
      G_ERROR( "ghw_rdblk: dest buffer too small" );
      return;
      }

   /* Save header info */
   desthdr = (PGHW_BLK_HEADER) dest;
   cp = &(desthdr->dat[0]);
   desthdr->lx = ltx;
   desthdr->ly = lty;
   desthdr->rx = rbx;
   desthdr->ry = rby;

   #ifndef GBUFFER
   /* Set both x,y ranges in advance and take advantage of
      the controllers auto wrap features */
   ghw_set_yrange(lty,rby);
   ghw_set_xrange(ltx,rbx);
   ghw_auto_rd_start();
   #endif
   /* Convert to whole byte coordinates */
   #ifdef GHW_USING_VBYTE
   /* Vertical storage units */
   lty = GYBYTE(lty);
   rby = GYBYTE(rby);
   #else
   /* Horizontal storage units */
   ltx = GXBYTE(ltx);
   rbx = GXBYTE(rbx);
   #endif

   for (; lty <= rby; lty++)
      {
      #ifdef GBUFFER
      gbufidx = ((GBUFINT)ltx) + ((GBUFINT)lty) * GDISPSW;
      for (x = ltx; x <= rbx; x++, gbufidx++)
         *cp++ = gbuf[gbufidx];
      #else
      for (x = ltx; x <= rbx; x++)
         *cp++ = ghw_auto_rd();
      #endif
      }
   *cp = 0;   /* tail = 0 (needed for simple wrblk shift) */
   }

/*
   Copy a graphic area from a GLCD buffer to the display
   The GLCD buffer must have been read with ghw_rdblk

   If the destination range is larger than the buffered range
   then the destination range is limited to fit the size of
   the buffered range.

   If the destination range is smaller than the buffered range
   then only the upper-left part of the buffer is written to
   the display.
*/
void ghw_wrblk(GXT ltx, GYT lty, GXT rbx, GYT rby, SGUCHAR *src )
   {
   GXT w,we,x,xe;
   GYT h,he;
   #ifdef GHW_USING_VBYTE
   GYT y;
   #endif
   GHWCOLOR *cp;
   GHWCOLOR dat,msk;
   PGHW_BLK_HEADER srchdr;
   SGINT shift;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   GBUF_CHECK();
   #else
   #ifdef GHW_USING_VBYTE
   GXT xb,xbe;
   #endif
   #endif

   glcd_err = 0;
   if (src == NULL)
      return;

   /* Force reasonable values */
   GLIMITU(ltx,GDISPW-1);
   GLIMITU(lty,GDISPH-1);
   GLIMITD(rby,lty);
   GLIMITD(rbx,ltx);
   GLIMITU(rby,GDISPH-1);
   GLIMITU(rbx,GDISPW-1);

   #ifdef GBUFFER
   invalrect( ltx, lty );
   invalrect( rbx, rby );
   #endif

   /* Get header info about stored buffer */
   srchdr = (PGHW_BLK_HEADER) src;
   cp = (GHWCOLOR *) &(srchdr->dat[0]);
   w  = srchdr->lx;
   h  = srchdr->ly;
   we = srchdr->rx;
   he = srchdr->ry;

   /* Limit destination range against source window size in buffer */
   if (rbx-ltx > (we-w))
      rbx = ltx + (we-w);
   if (rby-lty > (he-h))
      rby = lty + (he-h);

   #ifdef GHW_USING_VBYTE
   /* Vertical storage units */
   shift = (SGINT) shiftmsk[GPIXEL(h)] - (SGINT) shiftmsk[GPIXEL(lty)];
   msk  = startmask[GPIXEL(lty)];

   lty = GPIXMSK(lty); /* Access whole vertical bytes at a time */

   xe = rbx - ltx;    /* = num horizontal bytes */
   w  = we - w + 1;   /* Stored line width in bytes */
   #ifndef GBUFFER
   xbe = 0;           /* Silence illegal warning */
   #endif

   for (y=lty; y <= rby; y+=GHWPIXPSU)
      {
      /* Calculate buffer start / stop indexes for line */
      if (y == GPIXMSK(rby))
         msk &= stopmask[GPIXEL(rby)];

      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,y);
      #else
      ghw_set_ypos( y );
      ghw_set_xpos( ltx );
      #endif
      /* Select patterns and start mask */
      /* Mask bits outside boundaries */
      #ifndef GBUFFER
      for (x = 0, xb = 0; x <= xe; x++ )
      #else
      for (x = 0; x <= xe; x++ )
      #endif
         {
         if (shift != 0)
            {
            if (shift < 0)
             /* Use normal video byte ( 0->3 ) */
               {
               dat = (y<lty) ? 0 : cp[x+w];
               dat = (GHWCOLOR)(((((GHWCOLORD)cp[x]) << GDISPHCW) + ((GHWCOLORD) dat)) >> (GDISPHCW+shift));
               }
            else
               {
               dat = (GHWCOLOR)((((GHWCOLORD) cp[x])+ (((GHWCOLORD) cp[(GBUFINT)x-w]) << GDISPHCW)) >> shift);
               }
            }
         else
            dat = (GHWCOLOR) cp[x];

         #ifdef GBUFFER
         if (msk != GHWCOLOR_NO_MSK)
            dat = (gbuf[gbufidx] & ~msk) | (dat & msk);
         gbuf[gbufidx++] = dat;
         #else
         if (msk != GHWCOLOR_NO_MSK)
            {
            /* Read-modify-write operations needed */
            if (xb == 0)
               {  /* prefetch read data to temp buffer */
               xbe = xe-x; /* (remaining) line length */
               if (xbe >= (sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0])))
                  xbe = ((sizeof(ghw_tmpbuf)/sizeof(ghw_tmpbuf[0]))-1);
               ghw_auto_rd_start();
               for ( ; xb <= xbe; xb++)
                  ghw_tmpbuf[xb] = ghw_auto_rd();
               ghw_set_xpos( ltx+x );
               xb = 0;
               }
            dat = (ghw_tmpbuf[xb] & ~msk) | (dat & msk);
            if (xb++ == xbe)
               xb = 0;
            }
         ghw_auto_wr( dat );
         #endif
         }
      cp = &cp[w]; /* Set to next buffer line */
      msk = GHWCOLOR_NO_MSK;
      }
   #else /* GHW_USING_VBYTE */
   /* Horizontal storage units */

   /* Set shift value (negative = shift left) */
   shift = (SGINT) shiftmsk[GPIXEL(w)] - (SGINT) shiftmsk[GPIXEL(ltx)];
   xe = GXBYTE(rbx) - GXBYTE(ltx);    /* = num horizontal bytes */
   w  = GXBYTE(we)  - GXBYTE(w) + 1;  /* Stored line width in bytes */

   for (; lty <= rby; lty++)
      {
      /* Calculate buffer start / stop indexes for line */
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty);
      #else
      ghw_set_yrange(lty,lty);
      ghw_set_xpos(ltx);
      #endif
      /* Select patterns and start mask */
      msk = startmask[GPIXEL(ltx)];

      for (x = 0; x <= xe; x++ )
         {
         if (shift != 0)
            {
            if (shift < 0)
               {
               dat = (GHWCOLOR)((((GHWCOLORD) cp[x] << GDISPHCW) + ((GHWCOLORD) cp[x+1])) >> (GDISPHCW+shift));
               }
            else
               {
               dat = (x == 0) ? 0 : cp[x-1];
               dat = (GHWCOLOR)(((((GHWCOLORD) dat) << GDISPHCW) + (GHWCOLORD) cp[x]) >> abs(shift));
               }
            }
         else
            dat = (GHWCOLOR) cp[x];

         if (x == xe)
            msk &= stopmask[GPIXEL(rbx)];

         #ifdef GBUFFER
         if (msk != GHWCOLOR_NO_MSK)
            {
            dat = (gbuf[gbufidx] & ~msk) | (dat & msk);
            msk = GHWCOLOR_NO_MSK;
            }
         gbuf[gbufidx++] = dat;
         #else
         if (msk != GHWCOLOR_NO_MSK)
            {
            dat = (ghw_rd_x((x==0) ? ltx : rbx) & ~msk) | (dat & msk);
            msk = GHWCOLOR_NO_MSK;
            }
         ghw_auto_wr( dat );
         #endif
         }
      cp = &cp[w]; /* Set to next buffer line */
      }

   #endif /* GHW_USING_VBYTE */

   ghw_updatehw();   /* This function may be called directly from the user level so update is needed */
   #ifdef GHW_FAST_SIM_UPDATE
   GSimFlush();
   #endif
   }

/*
   Retore a block buffer in the same position as it was read
   The position information is taken from the header
*/
void ghw_restoreblk(SGUCHAR *src)
   {
   PGHW_BLK_HEADER srchdr;
   if ((srchdr = (PGHW_BLK_HEADER) src) != NULL)
      ghw_wrblk(srchdr->lx,srchdr->ly, srchdr->rx, srchdr->ry, src );
   }

#endif /* defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT) */
#endif /* GGRAPHICS */


