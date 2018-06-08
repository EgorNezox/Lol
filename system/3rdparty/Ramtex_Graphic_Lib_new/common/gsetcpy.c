/************************* gsetcpy.c *******************************

   Creation date: 041010

   Copy all settings from one viewport to another,
   except application defined data

   Revision date:     05-02-08
   Revision Purpose:  Added support for inter character spacing

   Revision date:     14-11-12
   Revision Purpose:  GDATA adaption

   Revision date:     25-03-14
   Revision Purpose:  Code optimized. Made automatic adaptable to VP configurations.

   Revision date:     10-10-17
   Revision Purpose:  If destination vp is current vp, then let low-level states
                      track always immediately track the changes
                      (Simpler high-level management in most cases)

   Version number: 1.2
   Copyright (c) RAMTEX International Aps 2004-2017
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GVIEWPORT

void gsetupcpy(SGUCHAR to_vp, SGUCHAR from_vp)
   {
   if ((to_vp < GNUMVP) && (from_vp < GNUMVP))
      {
      PGVP fromvp, tovp;
      fromvp = &GDATA_VIEWPORTS[from_vp];
      tovp   = &GDATA_VIEWPORTS[to_vp];
      #ifdef GEXTMODE
      /* Copy only VP data before application specific data */
      G_POBJ_CPY(tovp, fromvp, ((char*)(&gcurvp->vpapp)- (char*)gcurvp)+1);
      #else
      /* Copy VP data */
      G_POBJ_CPY(tovp, fromvp, sizeof(GVP));
      #endif

      GLIMITU(tovp->cpos.x, tovp->rb.x); /* remove any copied line overflow mark from tovp */
      #ifdef GDATACHECK
      /* correct VP checksum to new settings */
      fromvp = gcurvp;
      gcurvp = tovp;
      gi_calcdatacheck();
      gcurvp = fromvp;
      #endif
      if (tovp == gcurvp)
         {
         /* Current viewport changed.
            Activate any settings which may influence low-level module states */
         gselfont(gcurvp->pfont);
         #ifdef GHW_USING_COLOR
         /* Let active colors track palette*/
         ghw_setcolor(gcurvp->foreground,gcurvp->background);
         #endif
         }
      }
   }
#endif /* GVIEWPORT */


