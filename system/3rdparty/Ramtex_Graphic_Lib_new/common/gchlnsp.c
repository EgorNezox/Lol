/************************* gchlnsp.c *******************************

   Creation date: 07-02-08

   Functions for setting extra space in between lines and report line space
   Functions for setting extra space in between characters and report extra
   space

   Revision date:     15-05-2008
   Revision Purpose:  gsetspln_vp corrected

   Version number: 1.1
   Copyright (c) RAMTEX International Aps 2008
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* glcd prototypes */

#ifndef GNOTXTSPACE

#ifdef GS_ALIGN

/* Report extra character space pixel columns (default = 0) */
GXT ggetspch(void)
   {
   gi_datacheck(); /* check internal data for errors */
   return gcurvp->chln.x;
   }

/* Report extra line space pixel rows (default = 0) */
GYT ggetspln(void)
   {
   gi_datacheck(); /* check internal data for errors */
   return gcurvp->chln.y;
   }

/* Set extra character space pixel columns (default = 0) */
void gsetspch(GXT chsp)
   {
   if (gcurvp == NULL)
      return;
   #ifdef GDATACHECK
   if (chsp >= gcurvp->rb.x - gcurvp->lt.x)
      {
      G_WARNING( "gsetspch: character spacing exceeds viewport size" );
      chsp = gcurvp->rb.y - gcurvp->lt.y;
      }
   #endif
   gcurvp->chln.x = chsp;
   gi_calcdatacheck(); /* correct VP to new settings */
   }

/* Set extra line space pixel rows (default = 0) */
void gsetspln(GYT lnsp)
   {
   if (gcurvp == NULL)
      return;
   #ifdef GDATACHECK
   if (lnsp >= gcurvp->rb.y - gcurvp->lt.y)
      {
      G_WARNING( "gsetspln: line spacing exceeds viewport size" );
      lnsp = gcurvp->rb.y - gcurvp->lt.y;
      }
   #endif
   gcurvp->chln.y = lnsp;
   gi_calcdatacheck(); /* correct VP to new settings */
   }

#ifdef GFUNC_VP

GXT ggetspch_vp(SGUCHAR vp)
   {
   GCHECKVP(vp);
   return GDATA_VIEWPORTS[vp].chln.x;
   }

GYT ggetspln_vp(SGUCHAR vp)
   {
   GCHECKVP(vp);
   return GDATA_VIEWPORTS[vp].chln.y;
   }

void gsetspch_vp(SGUCHAR vp, GXT chsp)
   {
   GSETFUNCVP(vp, gsetspch(chsp) );
   }

void gsetspln_vp(SGUCHAR vp, GYT lnsp)
   {
   GSETFUNCVP(vp, gsetspln(lnsp) );
   }

#endif  /* GFUNC_VP */

#endif  /* GS_ALIGN */

#endif  /* GNOTXTSPACE */

