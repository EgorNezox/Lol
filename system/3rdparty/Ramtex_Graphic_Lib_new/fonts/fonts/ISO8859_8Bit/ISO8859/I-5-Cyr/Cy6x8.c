/**************************** Cy6x8.c *********************************

   Cy6x8 font table and code page structure definitions.
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
GCODE FCODE Cy6x8sym[256] =
   {
   #include "Cy6x8.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy6x8 =
   {
   6,       /* width */
   8,       /* height */
   sizeof(Cy6x8sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy6x8sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

