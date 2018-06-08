/************************ GHWPALRD.C *******************************

   Read the current operative palette to a GPALETTE_GREY buffer.

   Creation date:
   Revision date:
   Revision Purpose:

   Version number: 1.00
   Copyright (c) RAMTEX Engineering Aps 2004

*******************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

#ifdef GBASIC_INIT_ERR

#if (GHW_PALETTE_SIZE > 0)
/*
   Read pallette from controller setup to standard greyscale palette format
*/
SGBOOL ghw_palette_grey_rd(SGUINT start_index, SGUINT num_elements, GPALETTE_GREY *palette)
   {
   if ((num_elements == 0) ||
       ((start_index + num_elements) > (1<<GDISPPIXW)) ||
       (palette == NULL))
      {
      glcd_err = 1;
      return 1;
      }

   while(num_elements-- > 0)
      {
      palette->gr = ghw_palette_opr[start_index].gr;
      start_index++;
      palette++;
      }
   return glcd_err;
   }
#endif

#endif /* GBASIC_INIT_ERR */



