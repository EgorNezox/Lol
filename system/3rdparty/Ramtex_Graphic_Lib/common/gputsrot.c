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

   Revision Purpose:  Cursor position coordinate update now include viewport offsets.
   Revision date:     09-05-2009
   Revision Purpose:  Double line space (y) settings corrected. (Spacing is included in ggetfh());
   Revision date:     24-10-2011

   Version number: 1.2
   Copyright (c) RAMTEX Engineering Aps 2009-2011

***********************************************************************/
#include <gi_disp.h>

#if defined( GBASIC_TEXT ) && defined( GSOFT_SYMBOLS )

/*
   Output of rotated text string.
   This internal function common for char, widechar and multibyte char strings
*/

static void gi_puts_rotate( GSTRINGPTR *sm, float angle)
   {
   SGINT xb, yb, x, y, dy, h, w, line;
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

      xb = (SGINT)((SGUINT) gcurvp->cpos.x - gcurvp->lt.x);
      yb = (SGINT)((SGUINT) gcurvp->cpos.y - gcurvp->lt.y);
      line = w = dy = x = y = 0;
      h = ggetfh(); /* Includes line spacing settings */
      do
         {
         if ((line == 0) || (val == (GWCHAR)'\n') || (val == (GWCHAR)'\r'))
            {
            if ((line == 0) || (val == (GWCHAR)'\n'))
               line++;

            if ((val == (GWCHAR)'\n') || (val == (GWCHAR)'\r'))
               GINCPTR(cp);

            #ifdef GS_ALIGN
            if ((gcurvp->mode & GALIGN_VCENTER)!=0)
               {
               switch (gcurvp->mode & GALIGN_VCENTER)
                  {
                  case GALIGN_TOP:
                     dy = h*line-1;
                     break;
                  case GALIGN_BOTTOM:
                     dy = ((SGINT)gi_strlines( cp.s )-1)*(-h);
                     break;
                  case GALIGN_VCENTER:
                     dy = (((SGINT)gi_strlines( cp.s ))*(-h))/2+(h-1);
                     break;
                  default:
                     if (line <= 1)
                        dy = (((SGINT)gi_strlines( cp.s ))*(-h))/2+h;
                     else
                        dy+=h;
                     break;
                  }
               }
            else
            #endif
               dy = h*(line-1);

            #ifdef GS_ALIGN
            if ((gcurvp->mode & GALIGN_HCENTER)!=0)
               {
               switch (gcurvp->mode & GALIGN_HCENTER)
                  {
                  case GALIGN_RIGHT:
                     w = ((SGINT)gi_strlen( cp.s, 0))*(-1);
                     break;
                  case GALIGN_HCENTER:
                     w = ((SGINT)gi_strlen( cp.s, 0)+1)/(-2);
                     break;
                  default:
                     w = 0;
                     break;
                  }
               }
            else
            #endif
               w = 0;
            x = xb+(SGINT)((gi_fp_cos*(SGLONG)w + gi_fp_sin*(SGLONG)dy) >> 16);
            y = yb+(SGINT)((gi_fp_cos*(SGLONG)dy - gi_fp_sin*(SGLONG)w) >> 16);
            if ((val == (GWCHAR)'\n') || (val == (GWCHAR)'\r'))
               continue;
            }

         if (val == (GWCHAR)'\t')
            val = (GWCHAR)' ';      /* Potentially fetch full multibyte char */

         w += gi_putchw_rotate( val, x, y );
         x = xb+(SGINT)((gi_fp_cos*(SGLONG)w  + gi_fp_sin*(SGLONG)dy + 32768) >> 16);
         y = yb+(SGINT)((gi_fp_cos*(SGLONG)dy - gi_fp_sin*(SGLONG)w  + 32768) >> 16);
         GINCPTR(cp);
         }
      while ((val = GETCHAR(cp)) != 0);

      /* Update viewport coordinates with text end point */
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

      gsetupdate(update);  /* Enable drawing completion before flush */

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

