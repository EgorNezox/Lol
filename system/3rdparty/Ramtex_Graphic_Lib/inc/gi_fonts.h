/**************************** gi_fonts.h **************************

   Extern definition of fonts and code pages which are supplied with
   the GLCD system

   Custom defined fonts may also be added here.

   Creation date: 980223

   Revision date:     030205
   Revision Purpose:  New fonts added
   Revision date:     041406
   Revision Purpose:  arial18, narrow10 added
   Revision date:     160507
   Revision Purpose:  ms58p, narrow20, uni_16x16 added
   Revision date:     09-05-12
   Revision Purpose:  narrow10_w, narrow20_w added

   Version number: 2.4
   Copyright (c) RAMTEX Engineering Aps 1998-2012

*******************************************************************/
#ifndef GIFONTS_H
#define GIFONTS_H

#include <gdisphw.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Font definitions */
extern GCODE GFONT FCODE Lucida_Console_8x12;
extern GCODE GFONT FCODE Tahoma_15x13;
extern GCODE GFONT FCODE Tahoma_bold_18x14;
extern GCODE GFONT FCODE Consolas25x58;
extern GCODE GFONT FCODE Tahoma26;


#if (defined(GMULTIBYTE) || defined(GMULTIBYTE_UTF8) || defined(GWIDECHAR))
/* This font contains more than 256 symbols so it must be
   used with either GMULTIBYTE or GWIDECHAR defined */
extern GCODE GFONT FCODE uni_16x16;
#endif

/*
   You may add definitions of your own fonts here.
*/

#ifdef __cplusplus
}
#endif

#endif /* GIFONTS_H */

