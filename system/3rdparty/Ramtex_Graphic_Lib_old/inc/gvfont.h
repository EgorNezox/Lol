#ifndef GVFONT_H
#define GVFONT_H
/*******************************************************************

   Virtual fonts are fonts which codepage data and symbol data
   tables are accessed via a generic device driver function.

   This enables memory consuming parts of a font to be stored
   in virtual memory devices like a serial eeprom, memory cards,
   a flashdisk etc.

   The library can distinguise between compiled-in fonts and
   virtual fonts at runtime. This enables the two font types
   to be mixed in a program in a way so it become transperant
   for the application program code.

   Version number: 1.0
   Copyright (c) RAMTEX Engineering Aps 2001-2007

*******************************************************************/

#include <gdisphw.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef GVIRTUAL_FONTS
/* internal support functions */

/*
   Prepare for fast codepage and symbol data lookup (if needed)
   Init and preload a GSYMHEADV structure for the font related parameters
   (Default to first symbol in font)
*/
void gi_fontv_open(PGFONTV fp);

/*
   Return pointer to font range for virtual font
*/
PGCP_RANGE_V gi_fontv_cp( GWCHAR index );

/*
   Return pointer to font range for virtual font
   Initialze a GSYMHEADV structure and load (at least) symbol width information
*/
PGSYMBOL gi_fontv_sym( GWCHAR index );

/*
   Preset virtual symbol interface symbol
*/
void gi_symv_open( PGSYMBOL psymbol, SGUINT bw, GYT offset);

/*
   Return symbol byte for current symbol from virtual storage
   symdatidx = index for byte in symbol data array
   (Called by low-level drivers)
*/
SGUCHAR gi_symv_by(GBUFINT symdatidx);

#ifdef GHW_ACCELERATOR
/*
   Return 1 if symbol is loaded in memory directly accessible by
   display controller (accelerator accessible memory) and if so update the
   offset with logical offset of symbol
*/
SGUCHAR gi_symv_isloaded( SGULONG *offset );
#endif

#endif /* GVIRTUAL_FONTS */

#ifdef __cplusplus
}
#endif

#endif
