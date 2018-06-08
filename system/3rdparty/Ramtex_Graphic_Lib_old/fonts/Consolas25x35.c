/**************************** Consolas25x35.c ***********************

   Consolas25x35 font table and code page structure definitions.
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
GCODE FCODE Consolas25x35cp =
   {
   #include "Consolas25x35.cp" /* Codepage table */
   };

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;         /* Symbol header */
   SGUCHAR b[120];       /* Symbol data, "variable length" */
   }
GCODE FCODE Consolas25x35sym[256] =
   {
   #include "Consolas25x35.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Consolas25x35 =
   {
   23,       /* averange width */
   30,       /* height */
   sizeof(Consolas25x35sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Consolas25x35sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)&Consolas25x35cp
   };

