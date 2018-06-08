/************************* Gcgetfor.c *******************************

   Creation date: 031010

   Revision date:
   Revision Purpose:

   Version number: 1
   Copyright (c) RAMTEX Engineering Aps 2003

*********************************************************************/
#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT
#ifdef GHW_USING_COLOR

GCOLOR ggetcolorf(void)
   {
   return gcurvp->foreground;
   }

#ifdef GFUNC_VP

GCOLOR ggetcolorf_vp(SGUCHAR vp)
   {
   return gdata.viewports[vp].foreground;
   }

#endif /* GFUNC_VP */

#endif
#endif
