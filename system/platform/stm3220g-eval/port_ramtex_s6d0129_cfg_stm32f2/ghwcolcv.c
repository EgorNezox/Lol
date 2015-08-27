/************************* ghwcolcv.c ****************************

   Generic RGB palette / GCOLOR / symbol color conversion functions.

   Creation date:    10-02-2009
   Revision date:    04-17-2011
   Revision Purpose: Correction to ghw_color_blend(..) for 32 bit.
                     ghw_color_to_rgba(..) added

   Version number: 1.1
   Copyright (c) RAMTEX Engineering Aps 2009-2011

*********************************************************************/

#include <gdisphw.h>
#ifdef GHW_USING_RGB

/*
   Convert RGB symbol color value to generic RGB palette color
*/
void ghw_color_to_rgb( SGULONG color, GPALETTE_RGB *palette, SGUCHAR mode )
   {
   if (palette == NULL)
      return;

   if (mode == 8)
      {
      palette->r = (SGUCHAR) (color & 0xe0);
      palette->g = (SGUCHAR) ((color << 3) & 0xe0);
      palette->b = (SGUCHAR) ((color << 6) & 0xc0);
      /* Let MSBit on each color ribble down to get full saturation */
      palette->r |= (palette->r >> 3) | (palette->r >> 5);
      palette->g |= (palette->g >> 3) | (palette->g >> 5);
      palette->b |= (palette->b >> 2) | (palette->b >> 4) | (palette->b >> 6);
      }
   else
   if (mode == 16)
      {
      palette->r = (SGUCHAR)((color >> 8) & 0xf8);
      palette->g = (SGUCHAR)((color >> 3) & 0xfc);
      palette->b = (SGUCHAR)((color << 3) & 0xf8);
      /* Let MSBit on each color ribble down to get full saturation */
      if (palette->r == palette->b)
         {
         if (palette->r == (palette->g & 0xf8))
            palette->g = palette->r;  /* Promote grey level */
         }
      else
         palette->g |= (palette->g >> 6);
      palette->r |= (palette->r >> 5);
      palette->b |= (palette->b >> 5);
      }
   else
   if (mode == 18)
      {
      palette->r = (SGUCHAR)((color >> 10) & 0xfc);
      palette->g = (SGUCHAR)((color >> 4) & 0xfc);
      palette->b = (SGUCHAR)((color << 2) & 0xfc);
      /* Let MSBit on each color ribble down to get full saturation */
      palette->r |= (palette->r >> 6);
      palette->g |= (palette->g >> 6);
      palette->b |= (palette->b >> 6);
      }
   else
   if (mode >= 24)
      {
      palette->r = (SGUCHAR)(color >> 16);
      palette->g = (SGUCHAR)(color >> 8);
      palette->b = (SGUCHAR) color;
      }
   else
      {
      palette->r = 0;
      palette->g = 0;
      palette->b = 0;
      }
   }

/*
   Convert a RGB symbol color mode to hardware specific color mode
   This function is called both by ghwsymwr and common functions
*/
GCOLOR ghw_color_conv(SGULONG dat, SGUCHAR dat_bit_pr_pixel)
   {
   GPALETTE_RGB palette;
   ghw_color_to_rgb(dat, &palette, dat_bit_pr_pixel);
   return ghw_rgb_to_color( &palette );
   }

/*
   Blend two RGB colors using the hardware specific color mode
   This function is called both by ghwsymwr and common functions
*/
GCOLOR ghw_color_blend(GCOLOR fore, GCOLOR back, SGUCHAR alpha)
   {
   SGUCHAR ialpha;
   if (alpha == 0)
      return back;
   if (alpha == 0xff)
      return fore;
   ialpha = 0xff-alpha;
   #if (GDISPPIXW <= 24)
   return (GCOLOR)
        ((((((SGULONG)(fore & G_RED_MSK  ))*alpha + ((SGULONG)(back & G_RED_MSK  ))*ialpha)/0xff)&G_RED_MSK  ) |
         (((((SGULONG)(fore & G_GREEN_MSK))*alpha + ((SGULONG)(back & G_GREEN_MSK))*ialpha)/0xff)&G_GREEN_MSK) |
         (((((SGULONG)(fore & G_BLUE_MSK ))*alpha + ((SGULONG)(back & G_BLUE_MSK ))*ialpha)/0xff)&G_BLUE_MSK ));
   #else
   return (GCOLOR)
        ((((((SGULONG)(fore & G_RED_MSK  ))*alpha + ((SGULONG)(back & G_RED_MSK  ))*ialpha)/0xff)&G_RED_MSK  ) |
         (((((SGULONG)(fore & G_GREEN_MSK))*alpha + ((SGULONG)(back & G_GREEN_MSK))*ialpha)/0xff)&G_GREEN_MSK) |
         (((((SGULONG)(fore & G_BLUE_MSK ))*alpha + ((SGULONG)(back & G_BLUE_MSK ))*ialpha)/0xff)&G_BLUE_MSK ) | 0xff000000);
   #endif
   }

#endif

