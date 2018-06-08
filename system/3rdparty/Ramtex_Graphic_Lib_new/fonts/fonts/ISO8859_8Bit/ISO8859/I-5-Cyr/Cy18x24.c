/**************************** Cy18x24.c *********************************

   Cy18x24 font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[72];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy18x24sym[256] =
   {
   #include "Cy18x24.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy18x24 =
   {
   18,       /* width */
   24,       /* height */
   sizeof(Cy18x24sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy18x24sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

