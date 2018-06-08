/************************* gfsetcp.c *******************************

   Creation date: 980223

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption
   Revision date:     14-11-12
   Revision Purpose:  Dyn Vfont adaption


   Revision date:
   Revision Purpose:

   Version number: 2.2
   Copyright (c) RAMTEX International Aps 1998-2012
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GBASIC_TEXT
/********************************************************************
   Segment: Basic text
   Level: Fonts
   Set character pos. in view-port in a character based fashion.
   With proportional soft fonts the default character width for the
   font is used for calculation. Calculations and range limits are
   done relative to the current view-port.
*/
void gsetcpos( SGUCHAR xpos, SGUCHAR ypos )
   {
   GXT xs;
   GYT ys;

   gi_datacheck(); /* check internal data for errors */
   #if GDISPCW != 8
   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
      {
      xs = GDISPCW * (GXT)xpos;
      ys = 8 * (GYT)ypos;
      ys += 8-1; /* bottom of char */
      }
   else
   #endif
   #endif
      {
      xs = (GXT) gcurvp->fsize.x * (GXT)xpos;
      ys = (GYT) gcurvp->fsize.y * (GYT)ypos + (gcurvp->fsize.y-1);  /* bottom of char */
      #ifndef GNOTXTSPACE
      ys+=gcurvp->chln.y;
      #endif
      }

   /* normalize to view-port done in gsetpos() */
   gsetpos( xs,ys );
   }


#ifdef GFUNC_VP

void gsetcpos_vp( SGUCHAR vp, SGUCHAR xpos, SGUCHAR ypos )
   {
   GSETFUNCVP(vp, gsetcpos(xpos,ypos) );
   }

#endif /* GFUNC_VP */

#endif /* GBASIC_TEXT */


