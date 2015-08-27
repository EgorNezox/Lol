/************************** ghwsymrd.c *****************************

   Graphic symbol read functions for LCD display

   Read graphic area from the display to a GLCD buffer using the default
   symbol format.

   The byte ordering for a symbol is horizontal byte(s) containing the
   first pixel row at the lowest address followed by the byte(s) in
   the pixel row below etc. The symbol is left aligned in the byte buffer.

   All coordinates are absolute pixel coordinate.

   ---------

   The S6D0129 controller is assumed to be used with a LCD module.

   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is complied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)


   Revision date:
   Revision Purpose:

   Version number: 1.0
   Copyright (c) RAMTEX Engineering Aps 2006

*********************************************************************/
#include <s6d0129.h>   /* lcd controller specific definements */

#ifdef GSOFT_SYMBOLS
#if (defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT))

/*
   Copy a graphic area from the display to a buffer organized with the
   common symbol and font format. The default pixel resolution is used.
*/
void ghw_rdsym(GXT ltx, GYT lty, GXT rbx, GYT rby, PGUCHAR dest, SGUINT bw, SGCHAR mode)
   {
   GXT x;
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
   GLIMITD(bw,1);  /* just to silence linker, not used with this controller */

   #ifdef GBUFFER
   invalrect( ltx, lty );
   invalrect( rbx, rby );
   #endif
   mode &= GHW_PALETTEMASK;

   if (mode != GDISPPIXW) /* GDISPPIXW == 16,18 or 24 */
      {
      /* The symbol has different color storage format than the controller */
      G_WARNING( "ghwsymr: color depth not supported" );
      return;
      }

   #ifndef GBUFFER
   ghw_set_xyrange(ltx,lty,rbx,rby);
   #ifndef GHW_NO_RDINC
   ghw_auto_rd_start();
   #endif
   #endif

   for (; lty <= rby; lty++)
      {
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty); /* Calculate buffer index for line start */
      #endif

      /* Loop video bytes */
      for (x=ltx; x <= rbx; x++)
         {
         GCOLOR col;
         #ifdef GBUFFER
         col = gbuf[gbufidx++];
         #else

         #ifdef GHW_NO_RDINC
         col = ghw_rd(x,lty);
         #else
         col = ghw_auto_rd();
         #endif

         #endif

         /* Save pixel in library symbol endian order */
         #if (GDISPPIXW > 16)
         *dest++ = (SGUCHAR)((col>>16) & 0xff);
         #endif
         #if (GDISPPIXW > 8)
         *dest++ = (SGUCHAR)((col>>8) & 0xff);
         #endif
         *dest++ = (SGUCHAR)(col & 0xff);
         }
      }
   }


#endif /* defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT) */

#endif
