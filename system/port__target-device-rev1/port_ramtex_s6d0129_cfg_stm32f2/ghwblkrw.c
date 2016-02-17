/************************** ghwblkrw.c *****************************

   Graphic block copy functions for LCD display

   Read graphic area from the display to a GLCD buffer.
   Write graphic buffer to LCD display.

   Information about the size of the graphic area is stored in the buffer.
   The buffer can be written back to the display with ghw_wrblk(),
   optionally with another start origin.

   All coordinates are absolute pixel coordinate.

   ---------

   The s6d0129 controller is assumed to be used with a LCD module.

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
   Revision date:    16-11-2008
   Revision Purpose: Buffer storage size optimized for all pixel widths
                     and all word alignment sizes.

   Revision date:    23-12-2008
   Revision Purpose: Color swap bug in version 1.2 correcterd

   Version number: 1.3
   Copyright (c) RAMTEX Engineering Aps 2007-2008

*********************************************************************/
/* <stdlib.h> is included via gdisphw.h */
#include <s6d0129.h>   /* s6d0129 controller specific definements */

#ifdef GBASIC_INIT_ERR
#if (defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT))

typedef struct
   {
   GXT lx;
   GYT ly;
   GXT rx;
   GYT ry;
   SGUCHAR dat[1];
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
     followed by the block data
*/
void ghw_rdblk(GXT ltx, GYT lty, GXT rbx, GYT rby, SGUCHAR *dest, GBUFINT bufsize )
   {
   GXT x;
   PGHW_BLK_HEADER desthdr;
   GCOLOR col;
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
   dest = &(desthdr->dat[0]);
   desthdr->lx = ltx;
   desthdr->ly = lty;
   desthdr->rx = rbx;
   desthdr->ry = rby;

   /* Convert to byte coordinates */
   #ifndef GBUFFER
   ghw_set_xyrange( ltx, lty, rbx, rby );
   #ifndef GHW_NO_RDINC
   ghw_auto_rd_start();
   #endif
   #endif
   for (; lty <= rby; lty++)
      {
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty);
      #endif
      for (x = ltx; x <= rbx; x++)
         {
         /* Read pixel */
         #ifdef GBUFFER
         col = gbuf[gbufidx++];
         #else
         #ifdef GHW_NO_RDINC
         col = ghw_rd(x,lty);
         #else
         col = ghw_auto_rd();
         #endif
         #endif

         /* Do a generic pixel storage */
         *dest++ = (SGUCHAR) col;
         #if (GDISPPIXW > 8)
         col >>= 8;
         *dest++ = (SGUCHAR) col;
         #if (GDISPPIXW > 16)
         col >>= 8;
         *dest++ = (SGUCHAR) col;
         #if (GDISPPIXW > 24)
         col >>= 8;
         *dest++ = (SGUCHAR) col;
         #endif
         #endif
         #endif
         }
      }
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
   PGHW_BLK_HEADER srchdr;
   GBUFINT bw;
   SGUCHAR *nextline;
   GCOLOR col;
   #ifdef GBUFFER
   GBUFINT gbufidx;
   GBUF_CHECK();
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
   w = srchdr->lx;
   h = srchdr->ly;
   we = srchdr->rx;
   he = srchdr->ry;
   src = &(srchdr->dat[0]);

   /* Limit destination range against source window size in buffer */
   if (rbx-ltx > (we-w))
      rbx = ltx + (we-w);
   if (rby-lty > (he-h))
      rby = lty + (he-h);

   xe = rbx - ltx;         /* = num horizontal pixels */
   w  = (we - w + 1);      /* Stored line width in pixels */
   bw = (GBUFINT) w * ((GDISPPIXW+7)/8); /* Stored line width in bytes */

   for (; lty <= rby; lty++)
      {
      /* Calculate buffer start / stop indexes for line */
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty);
      #else
      ghw_set_xyrange( ltx, lty, rbx, rby );
      #endif
      nextline = &src[bw]; /* Set to next buffer line */

      /* Select patterns and start mask */
      for (x = 0; x <= xe; x++ )
         {
         /* Read pixel data from generic storage */
         col = (GCOLOR) *src;
         src++;
         #if (GDISPPIXW > 8)
         col = (GCOLOR)(col | (((GCOLOR)(*src))<<8));
         src++;
         #if (GDISPPIXW > 16)
         col = (GCOLOR)(col | (((GCOLOR)(*src))<<16));
         src++;
         #if (GDISPPIXW > 24)
         col = (GCOLOR)(col | (((GCOLOR)(*src))<<24));
         src++;
         #endif
         #endif
         #endif

         /* Write pixel */
         #ifdef GBUFFER
         gbuf[gbufidx++] = col;
         #else
         ghw_auto_wr( col );
         #endif
         }
      src = nextline;
      }
   ghw_updatehw();   /* This function may be called directly from the user level so update is needed */
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


