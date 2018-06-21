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
   Revision date:     01-01-17
   Revision Purpose:  narrow15, narrow13_e, narrow18_e, narrow24_e added
   Revision date:     10-01-18
   Revision Purpose:  Font name update to let font name and file names be the same.
                      rtmono8_8 renamed to mono8_8
                      msfont renamed to msfont58

   Version number: 2.6
   Copyright (c) RAMTEX International Aps 1998-2018
   Web site, support and upgrade: www.ramtex.dk


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
extern GCODE GFONT FCODE Cy14x16;
extern GCODE GFONT FCODE Cy14x16o;
extern GCODE GFONT FCODE Cy12x16p;

extern GCODE GFONT FCODE Cy8X15Th;
extern GCODE GFONT FCODE Cy8X15Th_new;
//extern GCODE GFONT FCODE Tahoma_bold_18x14;
//extern GCODE GFONT FCODE Consolas25x58;
extern GCODE GFONT FCODE Tahoma26;

#if memory_hack
extern GCODE GFONT FCODE HackConsolas25x35;
#else
extern GCODE GFONT FCODE Consolas25x35;
#endif
//extern GCODE GFONT FCODE Tahoma26;
//extern GCODE GFONT FCODE ariel18;
//extern GCODE GFONT FCODE ariel9;
//extern GCODE GFONT FCODE cp8859_9;
//extern GCODE GFONT FCODE cp8859_14;
//extern GCODE GFONT FCODE footnote;
//extern GCODE GFONT FCODE mono5_8;
//extern GCODE GFONT FCODE mono8_8;
//extern GCODE GFONT FCODE ms58p;
//extern GCODE GFONT FCODE msfont58;
//extern GCODE GFONT FCODE msfont78;
//extern GCODE GFONT FCODE narrow10;    /* Primarily ASCII */
//extern GCODE GFONT FCODE narrow15;
//extern GCODE GFONT FCODE narrow20;
//extern GCODE GFONT FCODE narrow10_w;  /* Unicode Western + Eastern European (+ Greek + Cyrillic) */
//extern GCODE GFONT FCODE narrow15_w;
//extern GCODE GFONT FCODE narrow20_w;
//extern GCODE GFONT FCODE narrow13_e;  /* Like narrowx_w + Extended Additional Latin (ex Vietnamese) */
//extern GCODE GFONT FCODE narrow18_e;
//extern GCODE GFONT FCODE narrow24_e;
//extern GCODE GFONT FCODE times13;
//extern GCODE GFONT FCODE times16;
//extern GCODE GFONT FCODE times9;

#if (defined(GMULTIBYTE) || defined(GMULTIBYTE_UTF8) || defined(GWIDECHAR))
/* This font contains more than 256 symbols so it must be
   used with either GMULTIBYTE, GMULTIBYTE_UTF8, or GWIDECHAR defined */
extern GCODE GFONT FCODE uni_16x16;
#endif

/*
   You may add definitions of your own fonts here.
*/

#ifdef __cplusplus
}
#endif

#endif /* GIFONTS_H */

