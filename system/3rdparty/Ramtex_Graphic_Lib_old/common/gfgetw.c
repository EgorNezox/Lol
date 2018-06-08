/************************* gfgetw.c *******************************

   Creation date: 980223

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption
   Revision date:     06-02-08
   Revision Purpose:  Support for additional character spacing

   Version number: 2.2
   Copyright (c) RAMTEX Engineering Aps 1998-2008

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#if defined( GBASIC_TEXT ) || defined( GSOFT_FONTS )
/********************************************************************
   Segment: SoftFonts
   Level: Fonts
   return (default) font width
*/
GXT gfgetfw( PGFONT fp )
   {
   if (fp == NULL)
      return GDISPCW;  /* default width */
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
   return gfgetfw(gcurvp->pfont) + gcurvp->chln.x;
   #else
   return gfgetfw(gcurvp->pfont);
   #endif
   }

GXT ggetfw_vp(SGUCHAR vp)
   {
   GCHECKVP(vp);
   #ifndef GNOTXTSPACE
   return gfgetfw(gdata.viewports[vp].pfont) + gdata.viewports[vp].chln.x;
   #else
   return gfgetfw(gdata.viewports[vp].pfont);
   #endif
   }

#endif  /* GBASIC_TEXT                         */

