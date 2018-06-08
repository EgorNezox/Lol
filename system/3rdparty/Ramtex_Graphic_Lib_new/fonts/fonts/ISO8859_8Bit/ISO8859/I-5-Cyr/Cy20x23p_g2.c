/**************************** Cy20x23p_g2.c *********************************

   Cy20x23p_g2 font table and code page structure definitions.
   This file has been auto-generated with the IconEdit tool.
   The IconEdit tool can be downloaded from:  www.ramtex.dk.

   Copyright(c) RAMTEX 1998-2016

**********************************************************************************/
#include <gdisphw.h>

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GCSYMHEAD sh;        /* Symbol header */
   SGUCHAR b[115];       /* Symbol data, "variable length" */
   }
GCODE FCODE Cy20x23p_g2sym[256] =
   {
   #include "Cy20x23p_g2.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Cy20x23p_g2 =
   {
   12,       /* width */
   23,       /* height */
   sizeof(Cy20x23p_g2sym[0]) - sizeof(GCSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Cy20x23p_g2sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)(NULL)
   };

