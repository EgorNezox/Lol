/************************* Gcsetbak.c *******************************

   Creation date: 031010

   Revision date:
   Revision Purpose:

   Version number: 1
   Copyright (c) RAMTEX International Aps 2003
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/
#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT
#ifdef GHW_USING_COLOR

GCOLOR gsetcolorb(GCOLOR back)
   {
   GCOLOR oldback = gcurvp->background;
   gcurvp->background = back;
   ghw_setcolor(gcurvp->foreground, gcurvp->background);
   return oldback;
   }

#ifdef GFUNC_VP

GCOLOR gsetcolorb_vp(SGUCHAR vp, GCOLOR back)
   {
   GCOLOR retp;
   GGETFUNCVP(vp, gsetcolorb(back) );
   return retp;
   }

#endif /* GFUNC_VP */

#endif
#endif

