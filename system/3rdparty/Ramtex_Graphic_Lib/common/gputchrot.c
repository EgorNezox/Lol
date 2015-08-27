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

   Version number: 1.2
   Copyright (c) RAMTEX Engineering Aps 2009-2011

***********************************************************************/
#include <gi_disp.h>

#if defined( GBASIC_TEXT ) && defined( GSOFT_FONTS )

/*
  Output a rotated (widechar) font character.
*/
void gputchwrot( GWCHAR ch, float angle)
   {
   SGINT x, y, w;
   #ifndef GHW_NO_HDW_FONT
   if (gishwfont())
      return;
   #endif

   x = (SGINT)((SGUINT) gcurvp->cpos.x);
   y = (SGINT)((SGUINT) gcurvp->cpos.y);

   gi_setfpsincos(angle); /* Prepare for fixed point calculations */
   if ((w = gi_putchw_rotate( ch, x, y )) != 0)
      {
      /* Add rotated width distance to next symbol (using 50 %rounding)*/
      x += (SGINT)((gi_fp_cos*(SGLONG)w + 32768)>>16);
      y -= (SGINT)((gi_fp_sin*(SGLONG)w + 32768)>>16);
      }

   /* Force end coordinate inside viewport, just in case */
   if (x < 0) x = 0;
   if (y < 0) y = 0;
   x += (SGINT)((SGUINT)gcurvp->lt.x);
   y += (SGINT)((SGUINT)gcurvp->lt.y);
   if (x > (SGINT)((SGUINT)gcurvp->rb.x))
      x = (SGINT)((SGUINT)gcurvp->rb.x);
   if (y > (SGINT)((SGUINT)gcurvp->rb.y))
      y = (SGINT)((SGUINT)gcurvp->rb.y);

   gcurvp->cpos.x = (GXT)x;
   gcurvp->cpos.y = (GYT)y;
   #ifdef GGRAPHICS
   gcurvp->ppos.x = (GXT)x; /* update graphics pos also */
   gcurvp->ppos.y = (GYT)y;
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

