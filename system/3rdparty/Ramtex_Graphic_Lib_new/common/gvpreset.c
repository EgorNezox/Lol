/************************* gvpreset.c ******************************

   Creation date: 980220

   Revision date:    05-05-03
   Revision Purpose: Cursor position tracks font symheight.
                     gresetposvp() added.

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption

   Revision date:     05-02-08
   Revision Purpose:  Added support for inter character spacing

   Revision date:     13-06-08
   Revision Purpose:  Viewport reset code made an internal function and
                      shared with ginit()

   Revision date:     16-03-16
   Revision Purpose:  gresetinitvp(), gresetinitvp_vp( vp ) added
                      If vp = current vp makes sure low-layer is tracking.

   Revision date:
   Revision Purpose:

   Version number: 2.5
   Copyright (c) RAMTEX International Aps 1998-2016
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */


#ifdef GVIEWPORT

/********************************************************************
   Segment: Viewport
   Level: View-port
   Reset the current view-port to full screen,
   set the cursor position to the upper left corner,
   set the pixel position to the upper left corner,
*/
void gresetposvp(void)
   {
   gi_datacheck();     /* check internal data for errors (assure valid defaults) */
   gi_cursor( 0 );     /* kill cursor (using old font size) */

   gi_resetvp(2);

   gi_cursor( 1 );     /* set cursor (using new font size) */
   }

/********************************************************************
   Segment: Viewport
   Level: View-port
   Reset the current view-port to full screen,
   set the cursor position to the upper left corner,
   set the pixel position to the upper left corner,
   set mode to GNORMAL,
   set font to SYSFONT,
   set code-page to NULL.
*/
void gresetvp(void)
   {
   gi_datacheck();     /* check internal data for errors (assure valid defaults) */
   gi_cursor( 0 );     /* kill cursor (using old font size) */

   gi_resetvp(1);

   gi_cursor( 1 );     /* set cursor (using new font size) */
   }

/********************************************************************
   Segment: Viewport
   Level: View-port
   Reset the current view-port to the state similar to after ginit();
*/
void gresetinitvp(void)
   {
   gi_datacheck();     /* check internal data for errors (assure valid defaults) */
   gi_cursor( 0 );     /* kill cursor (using old font size) */
   gi_resetvp(0);
   ghw_setcolor(gcurvp->foreground, gcurvp->background);
   gi_cursor( 1 );     /* set cursor (using new font size) */
   }

#ifdef GFUNC_VP

void gresetposvp_vp( SGUCHAR vp )
   {
   if (vp == GDATA_CURVPNUM)
      gresetposvp(); /* Reset and assure hardware is tracking */
   else
      GSETFUNCVP(vp, gi_resetvp(2) ); /* Just reset viewport */
   }

void gresetvp_vp( SGUCHAR vp )
   {
   if (vp == GDATA_CURVPNUM)
      gresetvp();   /* Reset and assure hardware is tracking */
   else
      GSETFUNCVP(vp, gi_resetvp(1) );  /* Just reset viewport */
   }

void gresetinitvp_vp( SGUCHAR vp )
   {
   if (vp == GDATA_CURVPNUM)
      gresetinitvp(); /* Reset and assure hardware is tracking */
   else
      GSETFUNCVP(vp, gi_resetvp(0) );   /* Just reset viewport */
   }

#endif /* GFUNC_VP */
#endif

