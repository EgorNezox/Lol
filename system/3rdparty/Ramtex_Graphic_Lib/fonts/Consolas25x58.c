/**************************** Consolas25x58.c ***********************

   Consolas25x58 font table and code page structure definitions.
   This file has been auto-generated with the IconEdit / FontEdit tool.

   Copyright(c) RAMTEX 1998-2009

*****************************************************************/
#include <gdisphw.h>

/* Code page entry (one codepage range element) */
static struct
   {
   GCPHEAD chp;
   GCP_RANGE cpr[6];     /* Adjust this index if more codepage segments are added */
   }
GCODE FCODE Consolas25x58cp =
   {
   #include "Consolas25x58.cp" /* Codepage table */
   };

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;         /* Symbol header */
   SGUCHAR b[232];       /* Symbol data, "variable length" */
   }
GCODE FCODE Consolas25x58sym[15] =
   {
   #include "Consolas25x58.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Consolas25x58 =
   {
   25,       /* averange width */
   58,       /* height */
   sizeof(Consolas25x58sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Consolas25x58sym, /* pointer to array of SYMBOLS */
   15,      /* num symbols */
   (PGCODEPAGE)&Consolas25x58cp
   };

