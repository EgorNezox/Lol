/**************************** Tahoma_15x13.c ***********************

   Tahoma_15x13 font table and code page structure definitions.
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
GCODE FCODE Tahoma_15x13cp =
   {
   #include "Tahoma_15x13.cp" /* Codepage table */
   };

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;         /* Symbol header */
   SGUCHAR b[26];       /* Symbol data, "variable length" */
   }
GCODE FCODE Tahoma_15x13sym[256] =
   {
   #include "Tahoma_15x13.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Tahoma_15x13 =
   {
   8,       /* averange width */
   13,       /* height */
   sizeof(Tahoma_15x13sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Tahoma_15x13sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)&Tahoma_15x13cp
   };

