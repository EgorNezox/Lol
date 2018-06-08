/**************************** gsputs.c ************************************

   Creation date: 980223

   Revision date:     02-01-27
   Revision Purpose:  Support for soft cursors change.
   Revision date:     03-01-26
   Revision Purpose:  Bit mask on GINVERSE mode
   Revision date:     03-02-07
   Revision Purpose:  Support for extended alignment
   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     7-01-05
   Revision Purpose:  GNOSCROLL, GALIGN_TOP, GALIGN_BOTTOM, GALIGN_LEFT
                      support added.
   Revision date:     05-05-05
   Revision Purpose:  Correction to GALIGN_BOTTOM when using build-in hardware fonts
   Revision date:     25-01-08
   Revision Purpose:  Major redesign of gputs gfputch. char, multibyte-char,
                      and wide-char now uses a common processing body. Less ROM size.
                      More viewport format features added.
                      GWORDWRAP compilation switch is replace by mode setting
   Revision date:     25-03-08
   Revision Purpose:  Multibyte handling fixed.
   Revision date:     24-04-08
   Revision Purpose:  Empty (space) string(s) now handled correctly in G_IS_ALIGN_V_CENTER mode
   Revision date:     08-08-08
   Revision Purpose:  Corrected tabulator clear when SYSFONT is a hardware fonts
   Revision date:     18-03-09
   Revision Purpose:  Corrected tabulator position setting
   Revision date:     08-05-09
   Revision Purpose:  Character position rewind with oversized multibyte
                      strings corrected.
   Revision date:     06-07-09
   Revision Purpose:  Prototypes for internal function in gfputch.c moved to
                      gi_disp.h
   Revision date:     14-11-12
   Revision Purpose:  GDATA adaption
   Revision date:     03-02-15
   Revision Purpose:  Fixed GVPCLR clear when viewport smaller than font.
   Revision date:     13-10-16
   Revision Purpose:  gi_sizestrseg(..) funtion and GSTRMARKS structure made public
                      for potential reuse and prototyped in gi_disp.h
   Revision date:     3-4-17
   Revision Purpose:  gi_sizestrseg(..) correction for the special combination case:
                         " long_line_starts_with_space" +
                         GWORD_WRAP +
                         first_string_word_wider_than_viewport (so it triggers character based wrap)

   Version number: 2.17
   Copyright (c) RAMTEX International Aps 1998-2017
   Web site, support and upgrade: www.ramtex.dk

***************************************************************************/

#include <gi_disp.h>

#ifdef GBASIC_TEXT

/* internal functions and data located in gfputch.c which are
   only externally called from this module */

/* Define convinent short hand notations */
#define  ltx gcurvp->lt.x
#define  lty gcurvp->lt.y
#define  rbx gcurvp->rb.x
#define  rby gcurvp->rb.y
#define  cposx gcurvp->cpos.x
#define  cposy gcurvp->cpos.y

#ifndef GNOTXTSPACE
/* Do extra space alignment */
#define  chlnx gcurvp->chln.x
#define  chlny gcurvp->chln.y
#endif

/* Line segment marker structure */
/*
typedef struct
   {
   GSTRINGPTR cpbeg;
   GSTRINGPTR cpend;
   GSTRINGPTR cpnext;
   SGUINT slen;
   } GSTRMARKS, *PGSTRMARKS;
*/
/*
   Find line segment which fit in viewport
   Return 0 if empty line
   Return 1 if single (or last) line segment
   Return 2 if line segment is created by (followed by) \r
   Return 3 if more line segments follows (execute a new line caused by wrap or \n)

   When horizontal alignment is enabled the \t and \r is converted to a space
*/
SGUCHAR gi_sizestrseg( PGSTRMARKS sm )
   {
   SGUINT sw; /* symbol width */
   GSTRINGPTR cp,cpsp,cpcstart;
   SGUINT slensp = 0;
   GWCHAR val;
   SGUCHAR first;
   cp.s = sm->cpnext.s;
   if (GETCHAR(cp) == 0)
      return 0;       /* fast skip of last string segment */
   #ifdef GMULTIBYTE
   cp.s = sm->cpnext.s;
   #endif

   #ifdef GS_ALIGN  /* Do alignment */
   if (G_IS_ALIGN_HR())
      {
      /* Skip leading whites */
      for(;;)
         {
         #ifdef GMULTIBYTE
         GSTRINGPTR cptmp;
         cptmp = cp;
         #endif
         val = GETCHAR(cp);
         if (!((val == ' ') || (val == '\t') || (val == '\r'))) /* leading white ? */
            {
            #ifdef GMULTIBYTE
            cp = cptmp; /* Restore multibyte position */
            #endif
            break;
            }
         GINCPTR(cp);
         }
      cposx = ltx;  /* cposx should not participate in width evaluation */
      }
   else
      {
      if (G_IS_ALIGN_LEFT() || G_IS_ALIGN_BOTTOM())
         cposx = ltx;   /* Start at left edge */
      }
   #endif

   sm->cpbeg.s = cp.s;  /* Begin of string */
   sm->cpend.s = cp.s;
   cpsp.s  = (PGCSTR)((void*)NULL);    /* No word separator detected */
   sm->slen = 0;        /* Length = 0 */

   for(first=1;;first=0)
      {
      cpcstart = cp; /* Mark start of (multibyte) character */
      val = GETCHAR(cp);
      if ((val == (GWCHAR) 0) || (val == (GWCHAR) '\n'))
         break;  /* End of line segment reached */

      if (val == (GWCHAR)'\r')
         {
         #ifdef GS_ALIGN  /* Do alignment */
         if (G_IS_ALIGN_H())
            {
            val = (GWCHAR)' ';  /* Process \r as space in horizontal alignment modes */
            goto process_space;
            }
         else
         #endif
            {
            if (first)  /* Process \r only as first character */
               {
               cposx = ltx;
               goto skipchar;
               }
            break;  /* \r end of line segment */
            }
         }

      if (val == '\t')
         { /* Tabulator is handled as a single variable width symbol */
         #ifdef GS_ALIGN  /* Do alignment */
         if (G_IS_ALIGN_H())
            {
            val = (GWCHAR)' ';  /* process \t as space in horizontal alignment modes */
            goto process_space;
            }
         #endif
         cpsp.s = sm->cpend.s;      /* Mark word boundary at previous (or first character) */
         slensp = sm->slen;
         sw = gi_tabstep((GXT)(cposx+sm->slen))-cposx; /* Viewport tab position in viewport */
         sw -= sm->slen;            /* Virtual tab character width */
         }
      else
         {
         if (val == ' ')
            {
            #ifdef GS_ALIGN  /* Do alignment */
            process_space:
            #endif
            cpsp.s = sm->cpend.s;      /* Mark word boundary at previous (or first) character */
            slensp = sm->slen;
            }

         /* Get width of (multibyte) character */
         sw = ggetsymw(val);       /* = GDISPCW ifndef GGSOFT_FONT */
         }

      if (sw >= (rbx-ltx+1))
         {                         /* (first) symbol larger than viewport */
         sm->slen = ((SGUINT)(rbx-ltx)+1);   /* Limit size (for calculations) to within viewport */
         GINCPTR(cp);              /* Point to next character */
         cpcstart = cp;            /* Mark start of (multibyte) character */
         break;
         }
      #ifdef GSOFT_FONTS
      if (G_IS_PARTIAL_CHAR())
         {
         if ((sm->slen+cposx+(G_IS_ALIGN_RIGHT() ? sw-1 : 0)) > rbx)
            break;  /* reached last (partly) visible character on line */
         }
      else
      #endif
         {
         #ifndef GNOTXTSPACE
         if (((sm->slen+sw)-1)+cposx+chlnx > rbx)
         #else
         if (((sm->slen+sw)-1)+cposx > rbx)
         #endif
            {
            /* Word exceeds viewport */
            if ((cpsp.s != NULL) && G_IS_WORDWRAP())
               {
               /* Break word at previous word boundary */
               sm->cpend.s = cpsp.s;
               sm->slen = slensp;
               cp.s = cpsp.s;
               if (slensp > 0)
                  GINCPTR(cp); /* Move point to word separator (if there is a word before word separator) */
               GINCPTR(cp);    /* Move point to character after word separator */
               cpcstart = cp;  /* Mark start of (multibyte) character */
               }

            /* else Break word at character boundary */
            break;
            }
         }
      sm->slen+=sw;
      #ifndef GNOTXTSPACE
      if (cp.s != sm->cpbeg.s)
         sm->slen+=chlnx; /* Add extra inter character space */
      #endif

      skipchar:

      sm->cpend.s = cp.s;
      GINCPTR(cp); /* Move point to character after word separator */
      cpcstart = cp; /* Mark start of (multibyte) character */
      }

   /* Set next character */
   sm->cpnext = cpcstart;  /* Set start of (multibyte) character
   (for rewind if the current character is not used due to wrapping ) */
   if (GETCTRLCHAR(sm->cpbeg) == 0)
      return 0;  /* Empty line segment */

   if ((GETCTRLCHAR(sm->cpnext) != 0)
       #ifdef GS_ALIGN  /* Do alignment */
       || G_IS_ALIGN_H_CENTER()
       #endif
       )
      {
      /* skip trailing whites from line segment */
      val = GETCTRLCHAR(sm->cpend);
      if (((val == ' ') || (val == '\t')) &&
           (sm->cpend.s != sm->cpbeg.s) && (cpsp.s != NULL))
         {
         sm->cpend.s = cpsp.s;
         sm->slen = slensp;
         }
      }

   for (;;)
      {
      val = GETCTRLCHAR(sm->cpnext);
      if (!G_IS_NOWRAP())
         break;
      /* Linecut is active, skip to end of line or end of string */
      if ((val == 0) || (val == '\n'))
         break;
      GINCPTR(sm->cpnext);
      }

   if (val == 0)
      return 1;
   if (val == '\r')
      return 2;
   if ((val == '\n') || (val == '\t'))
      GINCPTR(sm->cpnext);
   return 3;
   }

/*
   Put string
*/
static void gi_puts( PGSTRMARKS sm )
   {
   SGUCHAR ln;
   GYT fh;
   GYT vph;
   SGBOOL transmode;

   if( sm->cpnext.s == NULL )
      return;

   gi_put_prepare();
   fh = gfgetfh( gcurvp->pfont );
   vph = (rby-lty)+1;
   transmode = (G_IS_TRANSPERANT() && !G_IS_INVERSE()) ? 1 : 0;

   if (gi_fullheightline && (vph < fh))
      {
      G_WARNING( "gputch: Viewport height too small for character" );
      return; /* Viewport too small for font, skip output */
      }

   #ifdef GS_ALIGN  /* Do alignment */
   if (G_IS_ALIGN_V_CENTER() || G_IS_ALIGN_BOTTOM())
      {
      /* Preprocess whole string for y size calculation */
      /* (incl any wrap lines or multibyte characters) */
      SGUINT lines;
      #ifndef GNOTXTSPACE
      GYT linespaces;
      #endif
      GXT cposxtmp;
      GSTRINGPTR cp;

      for(lines = 0, cposxtmp = cposx, cp.s = sm->cpnext.s;;)
         {
         if ((ln = gi_sizestrseg(sm)) <= 1)
            {
            if (ln == 1)
               lines += 1; /* single line */
            /* else empty line detected */
            break;
            }
         /* Multiline string detected (optionally caused by character or word wrapping) */
         /* Find total number of lines for prescroll or vertical alignment */
         if (ln == 3)
            lines += 1;  /* \n or line wrap (else \r) */
         cposx = ltx;
         }

      /* Restore start position after size calculation */
      sm->cpnext.s = cp.s;
      cposx = cposxtmp;
      #ifndef GNOTXTSPACE
       /* extra line space is only used in between wrapping lines */
      linespaces = (lines > 1) ? chlny*(lines-1) : 0;
      #endif

      if (G_IS_ALIGN_V_CENTER())
         {
         cposy = fh-1 + lty; /* Overflow, align to top */
         #ifndef GNOTXTSPACE
         if ((SGUINT)fh*lines + linespaces < (SGUINT)vph)
            cposy += (vph-(fh*lines+linespaces))/2; /* Center lines */
         #else
         if ((SGUINT)fh*lines < (SGUINT)vph)
            cposy += (GYT)(vph-(fh*lines))/2;            /* Center lines */
         #endif
         }
      else /* G_IS_ALIGN_BOTTOM()*/
         {
         #ifndef GNOTXTSPACE
         if ((SGUINT)fh*lines + linespaces > (SGUINT)vph)
            {
            /* Some lines are skipped or cut */
            cposy = lty+chlny+((vph-1)%(fh+chlny));       /* Set top (partial) line pos */
            if (gi_fullheightline)
               {
               lines -= (vph-chlny)/fh;        /* partial top line is not shown */
               cposy += fh;
               }
            else
               lines -= ((SGUINT)vph+fh-1)/((SGUINT)fh+chlny); /* top line is partial shown */

            /* Skip invisible lines from string in advance */
            while (lines-- > 0)
               {
               gi_sizestrseg(sm);
               }
            }
         else
            cposy = rby-(fh+chlny)*(lines-1);
         #else
         if ((SGUINT)fh*lines > (SGUINT)vph)
            {
            /* Some lines are skipped or cut */
            cposy = lty+((vph-1)%fh);       /* Set top (partial) line pos */
            if (gi_fullheightline)
               {
               lines -= vph/fh;        /* partial top line is not shown */
               cposy += fh;
               }
            else
               lines -= ((SGUINT)vph+fh-1)/fh; /* top line is partial shown */

            /* Skip invisible lines from string in advance */
            while (lines-- > 0)
               {
               gi_sizestrseg(sm);
               }
            }
         else
            cposy = (GYT)(rby-fh*(lines-1));
         #endif  /* GNOTXTSPACE */
         }
      }
   else
      if (G_IS_ALIGN_TOP())
         cposy = lty+fh-1;  /* Move to top, compensate for font offset */
   #endif /* GS_ALIGN */

   #ifndef GHW_NO_HDW_FONT
   if (gishwfont())
      gi_xyposalign();
   #endif

   /* Output symbols */
   /* get line segment (handle all word wrap or word cut calculations) */
   ln = gi_sizestrseg(sm);

   /* Clear viewport above string ? */
   if (G_IS_VPCLR_UP() && (cposy >= fh) && !transmode)
      ghw_fill(ltx,lty,rbx,cposy-fh,(SGUINT)(G_IS_INVERSE() ? 0xffff : 0x0000));

   if (ln != 0)
      {
      do
         {
         #ifdef GS_ALIGN  /* Do alignment */
         if (G_IS_ALIGN_H_CENTER())
            cposx = (sm->slen >= ((SGUINT)rbx-ltx+1)) ? ltx : (GXT)((SGUINT)ltx + (((SGUINT)rbx-ltx+1) - sm->slen)/2);
         else
         if (G_IS_ALIGN_RIGHT())
            cposx = (sm->slen >= ((SGUINT)rbx-ltx+1)) ? ltx : (GXT)(((SGUINT)rbx+1) - sm->slen);
         #endif /* GS_ALIGN */

         /* Clear viewport to the left of the string segment ? */
         if (G_IS_VPCLR_LEFT() && (cposx > ltx) && !transmode)
            ghw_fill(ltx, (GYT)((cposy < lty+fh) ? lty : cposy-(fh-1)),
                     (GXT)(cposx-1),
                     (GYT)((cposy > rby) ? rby : cposy),
                     (SGUINT)(G_IS_INVERSE() ? 0xffff : 0x0000));

         for(;;)
            {
            GWCHAR val;
            val = GETCHAR(sm->cpbeg);

            #ifdef GS_ALIGN  /* Do alignment */
            if (G_IS_ALIGN_H() && ((val == '\r') || (val == '\t')))
               val = ' '; /* Process \t and \r as space in horizontal alignment modes */
            #endif /* GS_ALIGN */

            gi_putch( val );
            if (sm->cpbeg.s == sm->cpend.s)
               break;
            GINCPTR(sm->cpbeg);
            }

         /* Clear viewport to the left of the string segment ? */
         if (G_IS_VPCLR_RIGHT() && (cposx <= rbx) && !transmode)
            ghw_fill((GXT) cposx,(GYT)((cposy < lty+fh) ? lty : cposy-(fh-1)),
                      rbx,
                      (GYT)((cposy > rby) ? rby : cposy),
                      (SGUINT)(G_IS_INVERSE() ? 0xffff : 0x0000));

         if (ln == 3)
            {
            if (gi_process_newline(fh)) /* Make new line processing */
               break;  /* a no-scroll condition reached, no more characters needed */
            }
         ln = gi_sizestrseg(sm);
         }
      while (ln != 0);
      }
   else
      {
      /* The string only contained whites, or was empty */
      if ((G_IS_VPCLR_LEFT() || G_IS_VPCLR_RIGHT()) && !transmode)
         {
         /* clear of empty line */
         ghw_fill((GXT)(G_IS_VPCLR_LEFT() ? ltx : cposx),
                 (GYT)((cposy < lty+fh) ? lty : cposy-(fh-1)),
                 (GXT)(G_IS_VPCLR_RIGHT() ? rbx : cposx-1),
                 (GYT)((cposy > rby) ? rby : cposy),
                 (SGUINT)(G_IS_INVERSE() ? 0xffff : 0x0000));
         }
      }
   /* Clear viewport from line to bottom ? */
   if (G_IS_VPCLR_DOWN() && (cposy < rby) && !transmode)
      ghw_fill(ltx, cposy+1 ,rbx, rby,(SGUINT)(G_IS_INVERSE() ? 0xffff : 0x0000));

   gi_put_complete();
   }


void gputs( PGCSTR str )
   {
   GSTRMARKS sm;
   sm.cpnext.s = str;
   #ifdef GWIDECHAR
   GDATA_STRTYPE = 0;
   #endif
   gi_puts( &sm );
   }

#ifdef GWIDECHAR
void gputsw( PGCWSTR str )
   {
   GSTRMARKS sm;
   sm.cpnext.ws = str;
   GDATA_STRTYPE = 1;
   gi_puts( &sm );
   }
#endif

#ifdef GFUNC_VP

void gputs_vp( SGUCHAR vp, PGCSTR str )
   {
   GSETFUNCVP(vp, gputs( str ));
   }

#ifdef GWIDECHAR
void gputsw_vp( SGUCHAR vp, PGCWSTR str )
   {
   GSETFUNCVP(vp, gputsw( str ));
   }
#endif

#endif /* GFUNC_VP */

#endif /* GBASIC_TEXT */

