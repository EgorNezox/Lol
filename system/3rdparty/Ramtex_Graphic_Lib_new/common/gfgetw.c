/************************* gfgetw.c *******************************

   Creation date: 980223

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption
   Revision date:     06-02-08
   Revision Purpose:  Support for additional character spacing
   Revision date:     14-11-12
   Revision Purpose:  Named dynamic virtual font support, font size optimization

   Version number: 2.3
   Copyright (c) RAMTEX International Aps 1998-2012
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#if defined( GBASIC_TEXT ) || defined( GSOFT_FONTS )

#ifdef GVIRTUAL_FONTS_DYN
  #include <gvfont.h>
#endif

/********************************************************************
   Segment: SoftFonts
   Level: Fonts
   return (default) font width
*/
GXT gfgetfw( PGFONT fp )
   {
   #ifdef GVIRTUAL_FONTS_DYN
   if (gvf_open(fp))
      return GDISPCW;  /* Was a NULL pointer or Was a dynamic font, and open failed, use default width */
   #else
   if (fp == NULL)
      return GDISPCW;  /* default width */
   #endif
   return gi_fsymw(fp);
   }

GXT ggetfw(void)
   {
   gi_datacheck();    /* check internal data for errors */

   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
       return GDISPCW; /* default size of symbol */
   #endif

   #ifndef GNOTXTSPACE
   return gcurvp->fsize.x + gcurvp->chln.x;
   #else
   return gcurvp->fsize.x;
   #endif
   }

GXT ggetfw_vp(SGUCHAR vp)
   {
   GCHECKVP(vp);
   #ifndef GNOTXTSPACE
   return GDATA_VIEWPORTS[vp].fsize.x + GDATA_VIEWPORTS[vp].chln.x;
   #else
   return GDATA_VIEWPORTS[vp].fsize.x;
   #endif
   }

#endif  /* GBASIC_TEXT                         */

