/************************* ggetfsym.c *******************************

   Get a pointer to symbol in a font identified by the c character.
   If font = NULL the current viewport font is used.
   If a code page is used with the font then codepage lookup is done.

   PGSYMBOL ggetfsym(GWCHAR c, PGFONT fp) is mapped to this function.

   Creation date: 10-10-04

   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption

   Revision date:     17-04-2009
   Revision Purpose:  Virtual font structures split in RAM dependent and constant
                      data structures
                      (to handle compilers using non-standard C conformant pointers)
   Revision date:     14-11-12
   Revision Purpose:  Named dynamic virtual font support, font size optimization
                      The c paremeter is an symbol table index. Any code page is ignored.
   Revision date:     06-12-16
   Revision Purpose:  Use of font codepage corrected.

   Version number: 2.4
   Copyright (c) RAMTEX International Aps 2004-2016
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/
#include <gi_disp.h> /* gLCD prototypes */

#ifdef GSOFT_FONTS

#ifdef GVIRTUAL_FONTS
/* Vsymbol header, variable (RAM) part */
static GSYMHEADV_V vsymbol_v;
/* Vsymbol header, constant (ROM) part */
static GCODE GSYMHEADV FCODE vsymbol =
   {
   0,0,1,        /* Virtual font symbol type identifier */
   (PGSYMHEADV_V) &vsymbol_v /* Pointer to variable part*/
   };

#ifdef GVIRTUAL_FONTS_DYN
/* Vsymbol header, constant (ROM) part */
static GCODE GSYMHEADV FCODE vsymbol_named =
   {
   0,0,3,        /* Named Virtual font symbol type identifier (constant object) */
   (PGSYMHEADV_V) &vsymbol_v       /* Pointer to variable part*/
   };
#endif

#include <gvfont.h>

#endif

/* Assosiate a (wide) character with a symbol in a font */
PGSYMBOL ggetfsymw(GWCHAR c, PGFONT fp)
   {
   PGCODEPAGE cp;
   #ifdef GVIRTUAL_FONTS_DYN
   if (gvf_open(fp))
   #else
   if (fp == NULL)
   #endif
      {
      fp = gcurvp->pfont;     /* Current font */
      #ifndef GHW_NO_HDW_FONT
      if( fp == NULL )
         return (PGSYMBOL) NULL;  /* Current font is SYSFONT, It is a hardware font, so No symbol can be found */
      #endif
      }
   cp = gi_fpcodepage( fp );  /* gfsel.c */
   #ifdef GVIRTUAL_FONTS
   if (gisfontv(fp))
      {
      PGSYMBOL psym;
      PGSYMHEADV_V psymh;
      psym = gi_getsymbol(c, fp, cp);
      if (psym == NULL)
         return NULL;
      /* Take local copy of virtual symbol header so
         gputsym output can be mixed with gputs output for virtual fonts */
      psymh = psym->vsh.psymh_v;
      vsymbol_v.numbits = psymh->numbits;
      vsymbol_v.cxpix = psymh->cxpix;
      vsymbol_v.cypix = psymh->cypix;
      vsymbol_v.bidx = psymh->bidx;
      vsymbol_v.symsize = psymh->symsize;
      vsymbol_v.device_id = gvfdevice(fp);
      vsymbol_v.symbol_font = (PGVOIDC) fp;
      #ifdef GVIRTUAL_FONTS_DYN
      if (gisfontv_named(fp))
         /* Return pointer to constant part of virtual named symbol structure */
         return (PGSYMBOL)(&vsymbol_named);
      #endif
      /* Return pointer to constant part of virtual symbol structure */
      return (PGSYMBOL)(&vsymbol);
      }
   #endif
   return gi_getsymbol(c, fp, cp);
   }
#endif

