/************************* gvpsel.c ********************************

   Creation date: 980220

   Revision date:     01-07-10
   Revision Purpose:  gcurvpnum introduced to optimize viewport handling speed.

   Revision date:        14-1-2012
   Revision Purpose:  GDATA adaption

   Revision date:
   Revision Purpose:

   Version number: 2.1
   Copyright (c) RAMTEX International Aps 1998-2010
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT

/********************************************************************
   Segment: View-port
   Level: View-port
   Returns current view-port
   Set GNUMVP in gdispcfg.h to define the number of VP's supported.
*/
SGUCHAR ggetvpnum( void )
   {
   return ( GDATA_CURVPNUM < GNUMVP ) ? GDATA_CURVPNUM : 0;
   }

/* VP select function without returning old vp (selects viewport faster)*/
void gi_selvp( SGUCHAR vp )
   {
   GCHECKVP(vp);
   GDATA_CURVPNUM = vp;
   gcurvp = &GDATA_VIEWPORTS[ vp ];
   #ifdef GHW_USING_COLOR
   /* Let active colors track palette*/
   ghw_setcolor(gcurvp->foreground,gcurvp->background);
   #endif
   }

/********************************************************************
   Segment: View-port
   Level: View-port
   Selects the view-port to become current,
   Set GNUMVP in gdispcfg.h to define the number of VP's supported.
   After ginit() view-port 0 is set as default.
   Returns previous current view-port
*/
SGUCHAR gselvp( SGUCHAR vp /* view-port to select */ )
   {
   SGUCHAR vpold;
   vpold = ggetvpnum();   /* Get old viewport number (and do data check) */
   gi_selvp(vp);
   return vpold;
   }

#endif

