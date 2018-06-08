/**************************** Lucida_Console_8x12.c ***********************

   Lucida_Console_8x12 font table and code page structure definitions.
   This file has been auto-generated with the IconEdit / FontEdit tool.

   Copyright(c) RAMTEX 1998-2009

*****************************************************************/
#include <gdisphw.h>

/* Code page entry (one codepage range element) */
static struct
   {
   GCPHEAD chp;
   GCP_RANGE cpr[1];     /* Adjust this index if more codepage segments are added */
   }
GCODE FCODE Lucida_Console_8x12cp =
   {
   #include "Lucida_Console_8x12.cp" /* Codepage table */
   };

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;         /* Symbol header */
   SGUCHAR b[12];       /* Symbol data, "variable length" */
   }
GCODE FCODE Lucida_Console_8x12sym[152] =
   {
   #include "Lucida_Console_8x12.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Lucida_Console_8x12 =
   {
   8,       /* averange width */
   12,       /* height */
   sizeof(Lucida_Console_8x12sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Lucida_Console_8x12sym, /* pointer to array of SYMBOLS */
   152,      /* num symbols */
   (PGCODEPAGE)&Lucida_Console_8x12cp
   };

