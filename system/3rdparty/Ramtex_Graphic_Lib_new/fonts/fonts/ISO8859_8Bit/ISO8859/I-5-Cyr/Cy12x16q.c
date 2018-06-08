/**************************** Cy12x16q.c *********************************

   Cy12x16q font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[32];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy12x16qsym[256] =
   {
   #include "Cy12x16q.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy12x16q =
   {
   9,       /* width */
   16,       /* height */
   sizeof(Cy12x16qsym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy12x16qsym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

