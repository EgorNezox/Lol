/**************************** Tahoma26.c ***********************

   Tahoma26 font table and code page structure definitions.
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
GCODE FCODE Tahoma26cp =
   {
   #include "Tahoma26.cp" /* Codepage table */
   };

/* Symbol table entry with fixed sized symbols */
static struct
   {
   GSYMHEAD sh;         /* Symbol header */
   SGUCHAR b[104];       /* Symbol data, "variable length" */
   }
GCODE FCODE Tahoma26sym[256] =
   {
   #include "Tahoma26.sym" /* Include symbols */
   };

/* Font structure */
GCODE GFONT FCODE Tahoma26 =
   {
   13,       /* averange width */
   26,       /* height */
   sizeof(Tahoma26sym[0]) - sizeof(GSYMHEAD), /* number of bytes in a symbol (including any alignment padding)*/
   (PGSYMBOL)Tahoma26sym, /* pointer to array of SYMBOLS */
   256,      /* num symbols */
   (PGCODEPAGE)&Tahoma26cp
   };

