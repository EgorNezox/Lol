/************************* gfgeth.c ********************************

   Creation date: 980223

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption
   Revision date:     06-02-08
   Revision Purpose:  Support for additional character spacing

   Revision date:
   Revision Purpose:

   Version number: 2.2
   Copyright (c) RAMTEX Engineering Aps 1998-2008

*********************************************************************/

#include <gi_disp.h> /* gLCD protoypes */

#if defined( GBASIC_TEXT ) || defined( GSOFT_FONTS )
/********************************************************************
   Segment: Soft Fonts
   Level: Fonts
   return font height
*/
GYT gfgetfh( PGFONT fp )
   {
   if (fp == NULL)
      return SYSFONT.symheight;  /* default height */
   return gi_fsymh(fp);
   }

GYT ggetfh(void)
   {
   gi_datacheck();    /* check internal data for errors */
   #ifndef GNOTXTSPACE
   return gfgetfh( gcurvp->pfont ) + gcurvp->chln.y;
   #else
   return gfgetfh( gcurvp->pfont );
   #endif
   }

GYT ggetfh_vp(SGUCHAR vp)
   {
   GCHECKVP(vp);
   #ifndef GNOTXTSPACE
   return gfgetfh(gdata.viewports[vp].pfont) + gdata.viewports[vp].chln.y;
   #else
   return gfgetfh(gdata.viewports[vp].pfont);
   #endif
   }

#endif  /* GBASIC_TEXT */

