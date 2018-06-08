/**************************** Cy20x23.c *********************************

   Cy20x23 font table and code page structure definitions.
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
GCODE FCODE Cy20x23sym[256] =
   {
   #include "Cy20x23.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy20x23 =
   {
   20,       /* width */
   23,       /* height */
   sizeof(Cy20x23sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy20x23sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

