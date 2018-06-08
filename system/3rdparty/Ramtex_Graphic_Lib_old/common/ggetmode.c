/************************* ggetmode.c ********************************

   Creation date: 040725

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Revision date:
   Revision Purpose:

   Version number: 1.1
   Copyright (c) RAMTEX Engineering Aps 2004

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT

/********************************************************************
   Segment: View-port
   Level: View-port
   Returns the mode setting for a named viewport
   GNUMVP in gdispcfg.h must be set to number of VP's to support.
*/

GMODE ggetmode( void )
   {
   gi_datacheck(); /* check internal data for errors */
   return gcurvp->mode;
   }

#ifdef GFUNC_VP

GMODE ggetmode_vp( SGUCHAR vp )
   {
   GCHECKVP( vp );
   return gdata.viewports[vp].mode;
   }

#endif /* GFUNC_VP */

#endif
