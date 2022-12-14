/**************************** gputchrot.c *****************************

   Functions for rotated write of character symbols from the current font.

   The viewport mode alignment setting influences where the acher point
   is connected to the symbol frame

   Effect of ancher point alignment on text output start position in text frame
      GALIGN_LEFT      Ancher at left edge of character symbol
      GALIGN_HCENTER   Ancher at horizontal center of character symbol
      GALIGN_RIGHT     Ancher at right character symbol edge
      (default)        Normal, Ancher is left corner of character symbol.

      GALIGN_TOP       Ancher at Top edge
      GALIGN_VCENTER   Ancher at vertical center of character symbol
      GALIGN_BOTTOM    Ancher at bottom symbol edge
      (default)        Normal, Ancher is lower corner of symbol.

   Creation date: 18-02-2009

   Revision Purpose:  Cursor position coordinate update now include viewport offsets.
   Revision date:     09-05-2009
   Revision Purpose:  Bug in y position update corrected. Characters can not be output successively
   Revision date:     26-08-2011
   Revision Purpose:  Internal use of fixed point character x,y positions to get accurate position
                      adjustment.
   Revision date:     02-12-2015

   Version number: 1.3
   Copyright (c) RAMTEX International Aps 2009-2015
   Web site, support and upgrade: www.ramtex.dk

***********************************************************************/
#include <gi_disp.h>
#include <gfixedpt.h>

#if defined( GBASIC_TEXT ) && defined( GSOFT_FONTS )

/*
  Output a rotated (widechar) font character.
*/
void gputchwrot( GWCHAR ch, float angle)
   {
   SGFIXP fx, fy, fw;
   #ifndef GHW_NO_HDW_FONT
   if (gishwfont())
      return;
   #endif
   gi_setfpsincos(angle); /* Prepare for fixed point calculations */
   fx = SGU_TO_FIXP(gcurvp->cpos.x);
   fy = SGU_TO_FIXP(gcurvp->cpos.y);

   if ((fw = gi_putchw_rotate( ch, fx, fy )) != 0)
      {
      /* Add rotated width distance to next symbol (using 50 %rounding)*/
      fx += (gi_fp_cos*fw)/SGFIXP_SCALE;
      fy -= (gi_fp_sin*fw)/SGFIXP_SCALE;
      }

   /* Update viewport coordinates for start of next char */
   /* Force end coordinate inside viewport (and screen), just in case */
   if (fx < SGU_TO_FIXP(gcurvp->lt.x))
      gcurvp->cpos.x = gcurvp->lt.x;
   else
   if (fx > SGU_TO_FIXP(gcurvp->rb.x))
      gcurvp->cpos.x = gcurvp->rb.x;
   else
      gcurvp->cpos.x = (GXT)SGFIXP_TO_U(fx);

   if (fy < SGU_TO_FIXP(gcurvp->lt.y))
      gcurvp->cpos.y = gcurvp->lt.y;
   else
   if (fy > SGU_TO_FIXP(gcurvp->rb.y))
      gcurvp->cpos.y = gcurvp->rb.y;
   else
      gcurvp->cpos.y = (GYT)SGFIXP_TO_U(fy);
   #ifdef GGRAPHICS
   gcurvp->ppos.x = gcurvp->cpos.x; /* update graphics pos also */
   gcurvp->ppos.y = gcurvp->cpos.y;
   #endif

   ghw_updatehw();
   gi_calcdatacheck(); /* correct VP to new settings */
   }


#ifdef GFUNC_VP
void gputchwrot_vp( SGUCHAR vp, GWCHAR ch, float angle )
   {
   GSETFUNCVP(vp, gputchwrot( ch, angle ));
   }
#endif

#endif /* GBASIC_TEXT && GSOFT_SYMBOLS  */

