/************************* gfgetyp.c *******************************

   Controls the use of soft fonts

   Creation date: 980223

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption

   Revision date:
   Revision Purpose:

   Version number: 2.2
   Copyright (c) RAMTEX Engineering Aps 1998-2007

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GBASIC_TEXT
/********************************************************************
   Segment: Basic text
   Level: Fonts
   Return current character ypos.
*/
SGUCHAR ggetcypos(void) /* return value = ypos */
   {
   GYT ypos; /* pixel pos. in view port */
   gi_datacheck(); /* check internal data for errors */
   /* normalize to view port */
   if ((ypos = gcurvp->cpos.y) > gcurvp->rb.y)
      ypos = gcurvp->rb.y;
   ypos = ypos - gcurvp->lt.y;
   #ifndef GNOTXTSPACE
   return (SGUCHAR)(ypos/(gi_fsymh(gcurvp->pfont) + gcurvp->chln.x));
   #else
   return (SGUCHAR)(ypos/gi_fsymh(gcurvp->pfont));
   #endif
   }

#ifdef GFUNC_VP

SGUCHAR ggetcypos_vp(SGUCHAR vp)
   {
   SGUCHAR retp;
   GGETFUNCVP(vp, ggetcypos());
   return retp;
   }

#endif /* GFUNC_VP */

#endif /* GBASIC_TEXT */

