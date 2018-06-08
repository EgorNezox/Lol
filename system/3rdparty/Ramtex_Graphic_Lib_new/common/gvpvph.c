/************************* gvpvph.c ********************************

   Creation date: 980220

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Revision date:     20-07-2011
   Revision Purpose:  Return type for ggetvph_vp fixed

   Revision date:        14-11-2012
   Revision Purpose:  GDATA adaption

   Version number: 2.2
   Copyright (c) RAMTEX International Aps 1998-2012
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT

/********************************************************************
   Segment: Viewport
   Level: Viewport
   return viewport height
*/
GYT ggetvph(void)
   {
   gi_datacheck(); /* check internal data for errors */
   return (gcurvp->rb.y - gcurvp->lt.y) + 1;
   }

GYT ggetvph_vp(SGUCHAR vp)
   {
   GCHECKVP( vp );
   return GDATA_VIEWPORTS[vp].rb.y - GDATA_VIEWPORTS[vp].lt.y + 1;
   }

#endif

