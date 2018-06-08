/************************ GHWPLRGB.C *******************************

   Update the SSD0323 grey scale palette via a standard RGB palette
   file, or read the palette as to a standard RGB palette buffer

   Version number: 1.0
   Copyright (c) RAMTEX Engineering Aps 2004

*******************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

#ifdef GBASIC_INIT_ERR

/*
   Read pallette from controller setup to standard RGB palette format
*/
#if (GHW_PALETTE_SIZE > 0)

SGBOOL ghw_palette_rd(SGUINT start_index, SGUINT num_elements, GPALETTE_RGB *palette)
   {
   if ((num_elements == 0) ||
       ((start_index + num_elements) > sizeof(ghw_palette_opr)/sizeof(ghw_palette_opr[0])) ||
       (palette == NULL))
      {
      glcd_err = 1;
      return 1;
      }

   while(num_elements-- > 0)
      {
      /* All colors are writen in library gray mode, however in */
      /* gray mode only the green color has significance */
      palette->r = ghw_palette_opr[start_index].gr;
      palette->g = ghw_palette_opr[start_index].gr;
      palette->b = ghw_palette_opr[start_index].gr;
      start_index++;
      palette++;
      }
   return glcd_err;
   }

/*
   Load a new palette to the controller or update the existing palette
*/
SGBOOL ghw_palette_wr(SGUINT start_index, SGUINT num_elements, GCONSTP GPALETTE_RGB PFCODE *palette)
   {
   if ((num_elements == 0) ||
       ((start_index + num_elements) > sizeof(ghw_palette_opr)/sizeof(ghw_palette_opr[0])) ||
       (palette == NULL))
      {
      glcd_err = 1;
      return 1;
      }
   glcd_err = 0;

   /* (Partial) update of operative palette values */
   while(num_elements-- > 0)
      {
      ghw_palette_opr[start_index++].gr = palette->g;  /* Only green is used */
      palette++;
      }

   /* Copy operative palette to controller so it become visually active */
   ghw_activate_palette();
   return glcd_err;
   }

#endif

#endif /* GBASIC_INIT_ERR */

