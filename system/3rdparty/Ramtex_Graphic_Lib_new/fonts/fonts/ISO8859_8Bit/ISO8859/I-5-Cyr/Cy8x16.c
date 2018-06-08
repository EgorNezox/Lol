/**************************** Cy8x16.c *********************************

   Cy8x16 font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[16];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy8x16sym[256] =
   {
   #include "Cy8x16.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy8x16 =
   {
   8,       /* width */
   16,       /* height */
   sizeof(Cy8x16sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy8x16sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

