/**************************** gputsrot.c *****************************

   Functions for rotated write of text strings.

   One or more text lines defines a "text frame" area. This area is
   virtually rotated as a whole. The connection point for the rotation
   anchor is defined by the normal alignment mode settings.

   The viewport mode alignment setting therefore influences both where
   the acher point is connected to the text frame and the alignment of text
   within the frame.

   Effect of anchor point alignment on text output start position in text frame
      GALIGN_LEFT      Left align text, anchor at left edge of text frame
      GALIGN_HCENTER   Center text lines, anchor at horizontal center of
                       text frame
      GALIGN_RIGHT     Right align text, anchor at right frame edge
      (default)        Normal, Ancher is left corner of first text symbol.

      GALIGN_TOP       Top adjust text, Ancher at Top edge
      GALIGN_VCENTER   Center text vertically, anchor at vertical center
                       of text frame
      GALIGN_BOTTOM    Bottom align text, anchor at bottom frame edge
      (default)        Normal, Ancher is lower corner of first text symbol.

   Creation date: 18-02-2009

   Revision date:     09-05-2009
   Revision Purpose:  Cursor position coordinate update now include viewport offsets.
   Revision date:     24-10-2011
   Revision Purpose:  Double line space (y) settings corrected. (Spacing is included in ggetfh());
   Revision date:     02-12-2015
   Revision Purpose:  Internal use of fixed point character x,y positions to get more smooth
                      string rotation.
   Revision date:     05-01-2016
   Revision Purpose:  Correction of fixed point calculation for vertical string alignment

   Version number: 1.31
   Copyright (c) RAMTEX International Aps 2009-2016
   Web site, support and upgrade: www.ramtex.dk

***********************************************************************/
#include <gi_disp.h>
#include <gfixedpt.h>

#if defined( GBASIC_TEXT ) && defined( GSOFT_SYMBOLS )

/*
   Output of rotated text string.
   This internal function common for char, widechar and multibyte char strings
*/

static void gi_puts_rotate( GSTRINGPTR *sm, float angle)
   {
   SGINT xb, yb, h, line, ht;
   SGFIXP x, y, w, dy;
   GSTRINGPTR cp;
   GWCHAR val;
   #ifdef GBUFFER
   GUPDATE update;
   #endif

   #ifndef GHW_NO_HDW_FONT
   if (gishwfont())
      return; /* No support for hardware fonts. Use gputs() instead */
   #endif
   if( sm->s == NULL )
      return;
   cp.s = sm->s;
   if  ((val = GETCHAR(cp)) != 0)
      {
      #ifdef GBUFFER
      update = gsetupdate(GUPDATE_OFF);  /* Enable drawing completion before flush */
      #endif

      gi_setfpsincos(angle);  /* Prepare for fixed point calculations */

          // xb,yb is viewport relative
      xb = (SGINT)((SGUINT) gcurvp->cpos.x);
      yb = (SGINT)((SGUINT) gcurvp->cpos.y);
      h = _GTOI(ggetfh()); /* Includes line spacing settings */
      line = ht = 0;
      w = x = y = dy =  0;

      do
         {
         if ((line == 0) || (val == (GWCHAR)'\n') || (val == (GWCHAR)'\r'))
            {
            if ((line == 0) || (val == (GWCHAR)'\n'))
               line++; /* Start first line or move to next line */

            if ((val == (GWCHAR)'\n') || (val == (GWCHAR)'\r'))
               GINCPTR(cp);

            #ifdef GS_ALIGN
            if ((gcurvp->mode & GALIGN_VCENTER)!=0)
               {
               switch (gcurvp->mode & GALIGN_VCENTER)
                  {
                  case GALIGN_TOP:
                     dy = SGI_TO_FIXP(h*line)-SGU_TO_FIXP(1);
                     break;
                  case GALIGN_VCENTER:
                     if (ht == 0)
                        ht = (-h * gi_strlines( cp.s ))/2;
                     dy = SGI_TO_FIXP(ht + line*h);
                     break;
                  //case GALIGN_BOTTOM:
                  default:
                     if (ht == 0)
                        ht = gi_strlines( cp.s );
                     dy = SGI_TO_FIXP((line-ht)*h);
                     break;
                  }
               }
            else
            #endif
               dy = SGI_TO_FIXP(h*(line-1));

            #ifdef GS_ALIGN
            if ((gcurvp->mode & GALIGN_HCENTER)!=0)
               {
               switch (gcurvp->mode & GALIGN_HCENTER)
                  {
                  case GALIGN_LEFT:
                     w = 0;
                     break;
                  case GALIGN_HCENTER:
                     w = SGI_TO_FIXP(((SGINT)gi_strlen( cp.s, 0)+1))/(-2);
                     break;
                  //case GALIGN_RIGHT:
                  default:
                     w = SGI_TO_FIXP(((SGINT)gi_strlen( cp.s, 0))*(-1));
                     break;
                  }
               }
            else
            #endif
               w = 0;
            x = SGU_TO_FIXP(xb)+ ((gi_fp_cos*w + gi_fp_sin*dy)/SGFIXP_SCALE);
            y = SGU_TO_FIXP(yb)+ ((gi_fp_cos*dy - gi_fp_sin*w)/SGFIXP_SCALE);
            if ((val == (GWCHAR)'\n') || (val == (GWCHAR)'\r'))
               continue;
            }

         if (val == (GWCHAR)'\t')
            val = (GWCHAR)' ';      /* Potentially fetch full multibyte char */

         w += gi_putchw_rotate( val, x, y );
         x = SGI_TO_FIXP(xb)+((gi_fp_cos*w  + gi_fp_sin*dy)/SGFIXP_SCALE);
         y = SGI_TO_FIXP(yb)+((gi_fp_cos*dy - gi_fp_sin*w )/SGFIXP_SCALE);
         GINCPTR(cp);
         }
      while ((val = GETCHAR(cp)) != 0);

      /* Update viewport coordinates with text end point */
      /* Force end coordinate inside viewport, just in case */
      if (x < 0) x = 0;
      x += SGU_TO_FIXP(gcurvp->lt.x);
      if (x > SGU_TO_FIXP(gcurvp->rb.x))
         x = SGU_TO_FIXP(gcurvp->rb.x);

      if (y < 0) y = 0;
      y += SGU_TO_FIXP(gcurvp->lt.y);
      if (y > SGU_TO_FIXP(gcurvp->rb.y))
         y = SGU_TO_FIXP(gcurvp->rb.y);

      gcurvp->cpos.x = (GXT)SGFIXP_TO_U(x);
      gcurvp->cpos.y = (GYT)SGFIXP_TO_U(y);
      #ifdef GGRAPHICS
      gcurvp->ppos.x = gcurvp->cpos.x; /* update graphics pos also */
      gcurvp->ppos.y = gcurvp->cpos.y;
      #endif

      #ifdef GBUFFER
      gsetupdate(update);  /* Enable drawing completion before flush */
      #endif

      ghw_updatehw();
      gi_calcdatacheck(); /* correct VP to new settings */
      }
   }

void gputsrot( PGCSTR str, float angle)
   {
   GSTRINGPTR sm;
   sm.s = str;
   #ifdef GWIDECHAR
   gdata.strtype = 0;
   #endif
   gi_puts_rotate( &sm, angle );
   }

#ifdef GWIDECHAR
void gputswrot( PGCWSTR str, float angle)
   {
   GSTRINGPTR sm;
   sm.ws = str;
   gdata.strtype = 1;
   gi_puts_rotate( &sm, angle );
   }
#endif

#ifdef GFUNC_VP

void gputsrot_vp( SGUCHAR vp, PGCSTR str, float angle )
   {
   GSETFUNCVP(vp, gputsrot( str, angle ));
   }

#ifdef GWIDECHAR
void gputswrot_vp( SGUCHAR vp, PGCWSTR str, float angle )
   {
   GSETFUNCVP(vp, gputswrot( str, angle ));
   }
#endif

#endif /* GFUNC_VP */

#endif /* GBASIC_TEXT && GSOFT_SYMBOLS  */


