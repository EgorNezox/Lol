/**************************** Cy8x14.c *********************************

   Cy8x14 font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[14];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy8x14sym[256] =
   {
   #include "Cy8x14.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy8x14 =
   {
   8,       /* width */
   14,       /* height */
   sizeof(Cy8x14sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy8x14sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

