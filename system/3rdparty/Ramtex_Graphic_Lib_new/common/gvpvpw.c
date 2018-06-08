/************************* gvpvpw.c ********************************

   Creation date: 980220

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Revision date:     14-11-2012
   Revision Purpose:  GDATA adaption

   Revision date:
   Revision Purpose:

   Version number: 2.2
   Copyright (c) RAMTEX International Aps 1998-2012
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT

/********************************************************************
   Segment: View-port
   Level: View-port
   return view-port width
*/
GXT ggetvpw(void)
   {
   gi_datacheck(); /* check internal data for errors */
   return (gcurvp->rb.x - gcurvp->lt.x) + 1;
   }

GXT ggetvpw_vp(SGUCHAR vp)
   {
   GCHECKVP( vp );
   return GDATA_VIEWPORTS[vp].rb.x - GDATA_VIEWPORTS[vp].lt.x + 1;
   }

#endif

