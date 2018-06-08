/**************************** Cy8X15Th.c *********************************

   Cy8X15Th font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[15];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy8X15Thsym[256] =
   {
   #include "Cy8X15Th.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy8X15Th =
   {
   8,       /* width */
   15,       /* height */
   sizeof(Cy8X15Thsym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy8X15Thsym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

