/**************************** Cy15x16o.c *********************************

   Cy15x16o font table and code page structure definitions.
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
GCODE FCODE Cy15x16osym[256] =
   {
   #include "Cy15x16o.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy15x16o =
   {
   9,       /* width */
   16,       /* height */
   sizeof(Cy15x16osym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy15x16osym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

