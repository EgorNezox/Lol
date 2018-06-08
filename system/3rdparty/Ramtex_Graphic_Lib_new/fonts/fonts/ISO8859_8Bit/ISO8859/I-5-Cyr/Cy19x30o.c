/**************************** Cy19x30o.c *********************************

   Cy19x30o font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[90];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy19x30osym[256] =
   {
   #include "Cy19x30o.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy19x30o =
   {
   14,       /* width */
   30,       /* height */
   sizeof(Cy19x30osym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy19x30osym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

