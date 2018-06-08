/************************* gvpcset.c *******************************

   Creation date: 980223

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added

   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption

   Revision date:     14-11-12
   Revision Purpose:  Dyn vfont optimize

   Revision date:     11-02-2013
   Revision Purpose:  cxe,cye rounding matches grapic pixel settings.

   Version number: 2.4
   Copyright (c) RAMTEX International Aps 1998-2013
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT
/********************************************************************
   Segment: Viewport
   Level: Fonts
   Set view-port in characters coordinates
   0,0 is upper left corner.
   For soft fonts the used character-line distance is defined by
   the current character width, height
*/
void gsetcvp( SGUCHAR cxs, SGUCHAR cys, SGUCHAR cxe, SGUCHAR cye )
   {
   GXT xs,xe;
   GYT ys,ye;

   gi_datacheck(); /* check internal data for errors */
   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
      {
      xs = (GXT)(GDISPCW * (GXT)cxs);
      ys = (GYT)(8 * (GYT)cys);
      xe = (GXT)(GDISPCW * (GXT)(cxe+1))-1;
      ye = (GYT)(8 * (GYT)(cye+1))-1;
      }
   else
   #endif
      {
      xs = (GXT)(gcurvp->fsize.x * (GXT)cxs);
      ys = (GYT)(gcurvp->fsize.y * (GYT)cys);
      xe = (GXT)(gcurvp->fsize.x * ((GXT)cxe+1))-1;
      ye = (GYT)(gcurvp->fsize.y * ((GYT)cye+1))-1;
      }

   if (xe >= GDISPW) xe = GDISPW-1;
   if (ye >= GDISPH) ye = GDISPH-1;

   /* call vp function */
   gsetvp( xs,ys,xe,ye );
   }

#ifdef GFUNC_VP

void gsetcvp_vp( SGUCHAR vp, SGUCHAR cxs, SGUCHAR cys, SGUCHAR cxe, SGUCHAR cye )
   {
   GSETFUNCVP(vp, gsetcvp(cxs,cys,cxe,cye) );
   }

#endif /* GFUNC_VP */
#endif /* GVIEWPORT */

