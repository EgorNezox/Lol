/**************************** Cy24x32.c *********************************

   Cy24x32 font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[96];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy24x32sym[256] =
   {
   #include "Cy24x32.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy24x32 =
   {
   24,       /* width */
   32,       /* height */
   sizeof(Cy24x32sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy24x32sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

