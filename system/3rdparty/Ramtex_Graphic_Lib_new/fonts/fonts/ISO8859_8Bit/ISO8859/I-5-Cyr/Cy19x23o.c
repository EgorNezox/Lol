/**************************** Cy19x23o.c *********************************

   Cy19x23o font table and code page structure definitions.
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
GCODE FCODE Cy19x23osym[256] =
   {
   #include "Cy19x23o.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy19x23o =
   {
   11,       /* width */
   23,       /* height */
   sizeof(Cy19x23osym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy19x23osym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

