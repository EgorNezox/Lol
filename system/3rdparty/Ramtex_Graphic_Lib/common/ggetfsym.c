/************************* ggetfsym.c *******************************

   Get a pointer to symbol in a font identified by the c character.
   If font = NULL the current viewport font is used.
   If a code page is used with the font then codepage lookup is done.

   PGSYMBOL ggetfsym(SGUCHAR c, PGFONT fp) is mapped to this function.

   Creation date: 10-10-04

   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption

   Revision date:     17-04-2009
   Revision Purpose:  Virtual font structures split in RAM dependent and constant
                      data structures
                      (to handle compilers using non-standard C conformant pointers)

   Revision date:
   Revision Purpose:

   Version number: 2.2
   Copyright (c) RAMTEX Engineering Aps 2004-2009

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
#endif

/* Assosiate a (wide) character with a symbol in a font */
PGSYMBOL ggetfsymw(GWCHAR c, PGFONT fp)
   {
   PGCODEPAGE cp;
   if (fp == NULL)
      {
      fp = gcurvp->pfont;     /* Current font */
      cp = gcurvp->codepagep; /* Use font codepage or overrided codepage */
      #ifndef GHW_NO_HDW_FONT
      if( fp == NULL )
         return (PGSYMBOL) NULL;  /* Current fon is SYSFONT, It is a hardware font, so No symbol can be found */
      #endif
      }
   else
      cp = (PGCODEPAGE) NULL; /* Use font codepage if any */
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
      vsymbol_v.device_id = gvfdevice(fp);

      /* Return pointer to constant part of virtual symbol structure */
      return (PGSYMBOL)(&vsymbol);
      }
   #endif
   return gi_getsymbol(c, fp, cp);
   }
#endif

