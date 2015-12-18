/**************************** Tahoma_bold_18x14.c ***********************

   Tahoma_bold_18x14 font table and code page structure definitions.
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
GCODE FCODE Tahoma_bold_18x14cp =
   {
   #include "Tahoma_bold_18x14.cp" /* Codepage table */
   };

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;         /* Symbol header */
   SGUCHAR b[42];       /* Symbol data, "variable length" */
   }
GCODE FCODE Tahoma_bold_18x14sym[256] =
   {
   #include "Tahoma_bold_18x14.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Tahoma_bold_18x14 =
   {
   8,       /* averange width */
   14,       /* height */
   sizeof(Tahoma_bold_18x14sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Tahoma_bold_18x14sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)&Tahoma_bold_18x14cp
   };

