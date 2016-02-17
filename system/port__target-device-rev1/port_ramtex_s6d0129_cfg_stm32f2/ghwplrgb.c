/************************ GHWPLRGB.C *******************************

   Update the symbol palette via a standard RGB palette file, or read
   the palette as to a standard RGB palette buffer.
   The palette is used with non-rgb symbols.

   Revision date:     26-11-07
   Revision Purpose:  Size limit check corrected
   Revision date:     20-03-09
   Revision Purpose:  Number of copied elements limited to palette size
   Revision date:     30-04-09
   Revision Purpose:  Palette feature can be turned off with GHW_PALETTE_SIZE == 0

   Version number: 1.3
   Copyright (c) RAMTEX Engineering Aps 2006-2009

*******************************************************************/
#include <s6d0129.h>   /* Display controller specific definements */

#ifdef GBASIC_INIT_ERR

#if (GHW_PALETTE_SIZE > 0)
/*
   Read pallette from controller setup to standard RGB palette format
*/
SGBOOL ghw_palette_rd(SGUINT start_index, SGUINT num_elements, GPALETTE_RGB *palette)
   {
   if ((num_elements == 0) || (palette == NULL) ||
       (start_index >= sizeof(ghw_palette_opr)/sizeof(ghw_palette_opr[0])))
      {
      glcd_err = 1;
      return 1;
      }

   if ((start_index + num_elements) > sizeof(ghw_palette_opr)/sizeof(ghw_palette_opr[0]))
         num_elements = sizeof(ghw_palette_opr)/sizeof(ghw_palette_opr[0]) - start_index;

   glcd_err = 0;

   while(num_elements-- > 0)
      ghw_color_to_rgb((SGULONG) ghw_palette_opr[start_index++], palette++, GDISPPIXW);

   return glcd_err;
   }
#endif

#endif /* GBASIC_INIT_ERR */

