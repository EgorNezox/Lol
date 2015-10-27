/************************** ghwpixel.c *****************************

   Low-level functions for graphic pixel set and clear
   Absolute coordinates are used.

   The S6D0129 controller is assumed to be used with a LCD module.
   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is copied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Revision date:
   Revision Purpose:

   Version number: 1.0
   Copyright (c) RAMTEX Engineering Aps 2007

*********************************************************************/

#include <s6d0129.h>   /* s6d0129 controller specific definements */

#ifdef GGRAPHICS

/*
   Set pixel color
*/
void ghw_setpixel( GXT x, GYT y, GCOLOR color )
   {
   #ifdef GBUFFER
   GBUF_CHECK();
   #endif

   glcd_err = 0;

   /* Force resonable values */
   GLIMITU(y,GDISPH-1);
   GLIMITU(x,GDISPW-1);
   /* Calculate pixel position in byte */

   #ifdef GBUFFER
   gbuf[GINDEX(x,y)] = color;
   invalrect( x, y );
   #else
   ghw_set_xyrange(x,y,x,y);  /* Initiate LCD controller address pointers*/
   ghw_auto_wr(color);
   #endif
   }

#if (defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT))
/*
   Get pixel color
*/
GCOLOR ghw_getpixel(GXT x, GYT y)
   {
   glcd_err = 0;

   /* Force resonable values */
   GLIMITU(y,GDISPH-1);
   GLIMITU(x,GDISPW-1);

   #ifdef GBUFFER
   #ifdef GHW_ALLOCATE_BUF
   if (gbuf == NULL)
      {
      glcd_err = 1;
      return 0;
      }
   #endif
   /* Calculate byte index */
   return gbuf[GINDEX(x,y)];
   #else
    #ifdef GHW_NO_RDINC
     return ghw_rd(x,y);
    #else
     ghw_setxypos(x,y);
     ghw_auto_rd_start();
     return ghw_auto_rd();
    #endif
   #endif
   }

#endif /* defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT) */

#endif
