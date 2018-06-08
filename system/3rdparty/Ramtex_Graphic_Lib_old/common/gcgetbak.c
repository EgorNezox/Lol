/************************* Gcgetbak.c *******************************

   Creation date: 031010

   Revision date:
   Revision Purpose:

   Version number: 1
   Copyright (c) RAMTEX Engineering Aps 2003

*********************************************************************/
#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT
#ifdef GHW_USING_COLOR

GCOLOR ggetcolorb(void)
   {
   return gcurvp->background;
   }

#ifdef GFUNC_VP

GCOLOR ggetcolorb_vp(SGUCHAR vp)
   {
   return gdata.viewports[vp].background;
   }

#endif /* GFUNC_VP */

#endif
#endif

