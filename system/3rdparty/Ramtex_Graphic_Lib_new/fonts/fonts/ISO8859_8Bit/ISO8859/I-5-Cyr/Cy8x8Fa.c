/**************************** Cy8x8Fa.c *********************************

   Cy8x8Fa font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[8];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy8x8Fasym[256] =
   {
   #include "Cy8x8Fa.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy8x8Fa =
   {
   8,       /* width */
   8,       /* height */
   sizeof(Cy8x8Fasym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy8x8Fasym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

