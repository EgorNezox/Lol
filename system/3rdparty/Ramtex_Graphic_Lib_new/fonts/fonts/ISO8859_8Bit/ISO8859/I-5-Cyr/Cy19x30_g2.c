/**************************** Cy19x30_g2.c *********************************

   Cy19x30_g2 font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GCSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[150];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy19x30_g2sym[256] =
   {
   #include "Cy19x30_g2.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy19x30_g2 =
   {
   19,       /* width */
   30,       /* height */
   sizeof(Cy19x30_g2sym[0]) - sizeof(GCSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy19x30_g2sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

