/************************* gvpyb.c ********************************

   Creation date: 24-04-2004

   Revision date:     14-11-2012
   Revision Purpose:  GDATA adaption

   Version number: 1.1
   Copyright (c) RAMTEX International Aps 2004-2012
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT

/********************************************************************
   Segment: View-port
   Level: View-port
   Returns the bottom edge coordinate of the specified view-port
   GNUMVP in gdispcfg.h must
   be set to number of VP's to support.
*/
GYT gvpyb( SGUCHAR vp )
   {
   GCHECKVP( vp );
   return GDATA_VIEWPORTS[vp].rb.y;
   }

#endif

