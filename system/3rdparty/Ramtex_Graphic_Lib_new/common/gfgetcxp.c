/************************** gfgetcxp.c ******************************

   Creation date: 980223

   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption

   Version number: 2.2
   Copyright (c) RAMTEX International Aps 1998-2007
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GBASIC_TEXT

/********************************************************************
   Segment: Basic text
   Level: Fonts
   Return current character xpos.
   With proportional soft fonts the values are based on the default
   character width
*/
SGUCHAR ggetcxpos(void) /* return value = xpos */
   {
   GXT xpos; /* pixel pos. in view port */
   gi_datacheck(); /* check internal data for errors */
   /* normalize to view port */
   if ((xpos = gcurvp->cpos.x) > gcurvp->rb.x)
      xpos = gcurvp->rb.x;
   xpos = xpos - gcurvp->lt.x;
   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
      return xpos/GDISPCW;
   #endif
   #ifndef GNOTXTSPACE
   return (SGUCHAR)(xpos/(gcurvp->fsize.x + gcurvp->chln.x));
   #else
   return (SGUCHAR)(xpos/gcurvp->fsize.x);
   #endif
   }

#ifdef GFUNC_VP

SGUCHAR ggetcxpos_vp(SGUCHAR vp)
   {
   SGUCHAR retp;
   GGETFUNCVP(vp, ggetcxpos() );
   return retp;
   }

#endif /* GFUNC_VP */

#endif /* GBASIC_TEXT */

