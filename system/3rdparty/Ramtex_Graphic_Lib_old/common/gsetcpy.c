/************************* Gvpinit.c *******************************

   Creation date: 041010

   Copy all settings from one viewport to another,
   except application defined data

   Revision date:     05-02-08
   Revision Purpose:  Added support for inter character spacing

   Version number: 1.1
   Copyright (c) RAMTEX Engineering Aps 2004-2008

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT

void gsetupcpy(SGUCHAR to_vp, SGUCHAR from_vp)
   {
   if ((to_vp < GNUMVP) && (from_vp < GNUMVP))
      {
      GVP *fromvp;
      GVP *tovp;
      fromvp = &gdata.viewports[from_vp];
      tovp   = &gdata.viewports[to_vp];
      tovp->lt.x =      fromvp->lt.x;
      tovp->lt.y =      fromvp->lt.y;
      tovp->rb.x =      fromvp->rb.x;
      tovp->rb.y =      fromvp->rb.y;
      tovp->codepagep = fromvp->codepagep;
      tovp->pfont =     fromvp->pfont;
      tovp->mode =      fromvp->mode;
      tovp->cpos.x =    (fromvp->cpos.x > fromvp->rb.x) ? fromvp->rb.x : fromvp->cpos.x; /* remove any line overflow mark */
      tovp->cpos.y =    fromvp->cpos.y;
      #ifdef GGRAPHICS
      tovp->ppos.x =    fromvp->ppos.x;
      tovp->ppos.y =    fromvp->ppos.y;
      #endif
      #ifndef GNOTXTSPACE
      tovp->chln.x =    fromvp->chln.x;
      tovp->chln.y =    fromvp->chln.y;
      #endif
      #ifdef GHW_USING_COLOR
      tovp->foreground = fromvp->foreground;
      tovp->background = fromvp->background;
      #endif
      #ifdef GDATACHECK
      /* correct VP checksum to new settings */
      fromvp = gcurvp;
      gcurvp = tovp;
      gi_calcdatacheck();
      gcurvp = fromvp;
      #endif /* GDATACHECK */
      }

   }
#endif /* GVIEWPORT */


