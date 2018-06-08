/**************************** Cy20x23o.c *********************************

   Cy20x23o font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[69];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy20x23osym[256] =
   {
   #include "Cy20x23o.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy20x23o =
   {
   12,       /* width */
   23,       /* height */
   sizeof(Cy20x23osym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy20x23osym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

