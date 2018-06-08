/**************************** gfputch.c ************************************

   Creation date: 980223

   Revision date:     22-12-98
   Revision Purpose:  Scroll was done one pixel line too early. Is corrected
   Revision date:     03-10-01
   Revision Purpose:  Word wrapping on last character removed the following space
   Revision date:     01-11-11
   Revision Purpose:  Correction of auto wrapping with hardware fonts when
                      not modulus 8
   Revision date:     02-01-23
   Revision Purpose:  symwidth parameter added to gi_putsymbol(..)
                      support for vertical soft cursors.
   Revision date:     03-01-26
   Revision Purpose:  Bit mask on GINVERSE mode
   Revision date:     03-05-20
   Revision Purpose:  gi_putsymbol parameter types updated for large font support.
   Revision date:     13-08-04
   Revision Purpose:  Named viewport function _vp added
   Revision date:     25-01-08
   Revision Purpose:  Major redesign of gputs gfputch. char, multibyte-char,
                      and wide-char now uses a common processing body. Less ROM size.
                      More viewport format features added.
                      GWORDWRAP compilation switch is replace by mode setting
   Revision date:     10-05-2009
   Revision Purpose:  Line spacing fill corrected
   Revision date:     20-01-2010
   Revision Purpose:  Character spacing fill in GINVERSE mode corrected
   Revision date:     14-02-2011
   Revision Purpose:  Tab clear y top position corrected.
   Revision date:     15-08-2016
                      gi_clr_chsp() corrected for the case where cursor position
                      is less than current font height.

   Version number: 2.11
   Copyright (c) RAMTEX International Aps 1998-2016
   Web site, support and upgrade: www.ramtex.dk

***************************************************************************/
#include <gi_disp.h>

#ifdef GBASIC_TEXT

/* Define convinent short hand notations */
#define  ltx gcurvp->lt.x
#define  lty gcurvp->lt.y
#define  rbx gcurvp->rb.x
#define  rby gcurvp->rb.y
#define  cposx gcurvp->cpos.x
#define  cposy gcurvp->cpos.y

#ifndef GNOTXTSPACE
/* extra character spacing */
#define  chlnx gcurvp->chln.x
/* extra line spacing */
#define  chlny gcurvp->chln.y
#endif

SGBOOL gi_fullheightline;

#ifdef GVIRTUAL_FONTS_DYN
  #include <gvfont.h>
#endif

#ifndef GHW_NO_HDW_FONT
/* Align cursor pos to hw char matrix */
void gi_xyposalign(void)
   {
   if (((SGUINT) cposx + GDISPCW-1) <= rbx)
      /* Not at end of line */
      if ((cposx = (cposx / GDISPCW)*GDISPCW) < ltx)
         cposx += GDISPCW;

   cposy = ((cposy / GDISPCH)*GDISPCH) + GDISPCH-1;
   if (cposy < lty)
      cposy += GDISPCH;
   if (cposy > rby)
      cposy -= GDISPCH;
   }
#endif

SGUCHAR gi_process_newline(GYT lnsp)
   {
   if (lnsp == 0)
      lnsp = gfgetfh( gcurvp->pfont );
   #ifndef GNOTXTSPACE
   #ifndef GHW_NO_HDW_FONT
   if (!gishwfont())
   #endif
      /* Add extra line spacing */
      lnsp += chlny;
   #endif

   if (((SGUINT) cposy+lnsp) <= ((SGUINT)rby))
      {
      ghw_updatehw();   /* Update hardware here to speed buffered mode */
     #ifndef GNOTXTSPACE
      if (chlny > 0)

         {
         if ((G_IS_VPCLR_LEFT() || G_IS_VPCLR_RIGHT()) &&
            !(G_IS_TRANSPERANT() && !G_IS_INVERSE()))
            {
            /* Clear extra vertical space below current text from cursor to margin */
            ghw_fill(ltx, cposy+1,
                     G_IS_VPCLR_RIGHT() ? rbx : cposx, cposy+chlny+1,
                     (SGUINT)(G_IS_INVERSE() ? 0xffff : 0x0000));
            }
         }
      #endif
      cposy += lnsp;    /* Inside vp area, just advance position */
      }
   else
      {
      #if ( !defined(GHW_NO_LCD_READ_SUPPORT) || defined( GBUFFER ))
      if (G_IS_NOSCROLL())
      #endif
         {
         /* No scroll mode enabled for viewport (or hardware does not support read) */
         #ifndef GNOTXTSPACE
         if ( gi_fullheightline || ((cposy+chlny) >= rby))
         #else
         if ( gi_fullheightline || (cposy >= rby))
         #endif
            return 1;
         /* else Room for a partial line below current line */
         cposy += lnsp; /* Ok that cposy exceeds viewport here to compensate for ancher position */
         }
      #if ( !defined(GHW_NO_LCD_READ_SUPPORT) || defined( GBUFFER ))
      else
         {
         /* Process scroll modes */
         GYT numlines; /* flag or number of scroll lines */
         if ( gi_fullheightline )
            { /* Scroll so relative line positions are the same */
            numlines = lnsp;
            }
         else
            { /* Scroll so line is aligned with viewport bottom */
            numlines = (GYT)(((SGUINT)cposy+lnsp)- rby);
            cposy = rby;
            }

         /* Activate viewport scroll */
         #if (defined(GSOFT_FONTS) || defined(GSYMBOLS) || defined(GGRAPHIC))
         /* Scroll graphic symbols */
         ghw_gscroll(
                  gcurvp->lt.x,
                  gcurvp->lt.y,
                  gcurvp->rb.x,
                  gcurvp->rb.y,
                  numlines,
                  (SGUINT)(G_IS_INVERSE() ? 0xFFFF : 0x0000));
         #endif
         #ifndef GHW_NO_HDW_FONT
         /* Scroll text fonts (to at least numlines) */
         for(;;)
            {
            if ((lnsp = (lty / GDISPCH)*GDISPCH) < lty)
               lnsp += GDISPCH;
            ghw_tscroll(
               ltx,
               lnsp,
               gcurvp->rb.x,
               gcurvp->rb.y);
            if (numlines <= GDISPCH)
               break;
            numlines -= GDISPCH;
            }
         #endif
         }
      #endif /* defined( GBUFFER ) || !defined(GHW_NO_LCD_READ_SUPPORT) */
      }
   cposx = ltx;
   #ifndef GHW_NO_HDW_FONT
   if (gishwfont())
      gi_xyposalign();
   #endif
   return 0;
   }

/* Do checks, kill cursor and prepare for viewport relative calculations */
void gi_put_prepare(void)
   {
   gi_datacheck(); /* check internal data for errors */
   glcd_err = 0;   /* Reset HW error flag */

   gi_cursor( 0 ); /* kill cursor */

   #ifndef GHW_NO_HDW_FONT
   if (gishwfont())
      {
      gi_fullheightline = 1;
       /* Align cursor pos to hw char matrix */
      gi_xyposalign();
      }
   #endif
   #if (defined( GSOFT_FONTS ) && !defined (GHW_NO_HDW_FONT))
   else
   #endif
   #ifdef GSOFT_FONTS
      {
      #ifdef GVIRTUAL_FONTS_DYN
      if (gvf_open( gcurvp->pfont )) /* Reload font, if needed, ignore if not a dynamic vfont */
         glcd_err = 1;   /* Load failed */
      #endif
      gi_fullheightline = G_IS_PARTIAL_LINE() ? 0 : 1;
      }
   #endif
   }

/* Do checks, reposition cursor, update absolute viewport values */
void gi_put_complete(void)
   {
   #ifndef GNOCURSOR
   SGBOOL cursor_on = 1;
   if( (GDATA_CURSOR & GCURON) != 0 ) /* Cursor was on ? */
      {
      /* Check if there is space for a full cursor on the line */
      /* or a line feed and eventually a scroll should be made */
      if (((SGUINT)cposx + gcursor_width()) >= (SGUINT)rbx)
         {
         if (G_IS_NOWRAP())
            {
            if (gishwfont() || !G_IS_PARTIAL_CHAR())
               cursor_on = 0; /* Not room for cursor, Temp remove cursor */
            }
         else
            {
            GYT fh;
            fh = gfgetfh( gcurvp->pfont );
            if (((SGUINT)cposy+fh ) <= (SGUINT)rby)
               {
               cposy += fh; /* Inside vp area, just insert a \n */
               cposx = ltx;
               }
            else
               {
               if (!G_IS_NOSCROLL())
                  gi_process_newline(fh); /* Scroll to make room for cursor */
               if (gishwfont() || !G_IS_PARTIAL_CHAR())
                  cursor_on = 0; /* Not room for cursor, Temp remove cursor */
               }
            }
         }
      }

   #ifndef GHW_NO_HDW_FONT
   if (gishwfont())
      {
      /* Assure that hardware cursor tracks vp cursor */
      ghw_setcabspos( cposx, cposy );
      cposx = ghw_getcursorxpos(); /* back to abs pixel pos */
      cposy = ghw_getcursorypos();
      }
   #endif
   #endif /* GNOCURSOR */

   /* update viewport cursor data */
   if (cposx > rbx) cposx = rbx;
   if (cposy > rby) cposy = rby;

   gi_cursor( cursor_on ); /* set cursor on if it was on */
   #ifdef GGRAPHICS
   gcurvp->ppos.x = cposx; /* update graphics pos also */
   gcurvp->ppos.y = cposy;
   #endif

   ghw_updatehw();
   gi_calcdatacheck(); /* correct VP to new settings */
   }

/* return aboslute step position  */
GXT gi_tabstep(GXT x)
   {
   #ifndef GCONSTTAB
   SGUCHAR i;
   #endif
   x-=ltx;
   #ifdef GCONSTTAB
   /* Calculate next tab */
   x = ((x/GTABSIZE)+1)*GTABSIZE+ltx;
   #else
   /* find next dynamic tab */
   for( i=0; i<GMAXTABS; i++ )
      {
      if( x < GDATA_TABS[i] )  break;
      }
   if( i < GMAXTABS) /* Tab found */
      {
      x = GDATA_TABS[i]+ltx;
      if( x > rbx )
         x = rbx;
      }
   else /* tab not found */
      x = rbx;
   #endif
   return x;
   }

#ifndef GNOTXTSPACE
/*
   Clear character space
*/
void gi_clr_chsp(GYT h)
   {
   if (!G_IS_TRANSPERANT() ||                  /* Normal mode, In transperant mode space between characters is invisible */
       (G_IS_TRANSPERANT() && G_IS_INVERSE())) /* The "hole in plate" look. Text is invisible. Space between characters is blanked */
      {
      /* Clear extra character space area */
      ghw_fill(cposx, (GYT)((cposy < lty+h) ? lty : (cposy-(h-1))),
            cposx+gcurvp->chln.x, cposy,(SGUINT)(G_IS_INVERSE() ? 0xffff : 0x0000));
      }
   cposx += gcurvp->chln.x;
   }
#endif
/*
   Output character to screen,
   assuming that it is printable
   ('\n' and '\r' is handled at a higher level)

   Returns 0 if printed
   Returns symbol width if not printed (not room in vp, and ! GLINECUT)
*/
char gi_putch( GWCHAR val )
   {
   GXT w;
   if ((val == (GWCHAR)'\n') || (val == (GWCHAR)'\r'))
      return 0; /* No output, positions are handled at the level above */
   if (val == (GWCHAR)'\t')
      { /* Tabulator is handled like a single variable width symbol */
      if ((w = gi_tabstep(cposx)) != ltx)
         {
         GYT fh = gfgetfh( gcurvp->pfont );
         ghw_fill(cposx, (GYT)((cposy < lty+fh) ? lty : (cposy-(fh-1))),
                 w, cposy,(SGUINT)(G_IS_INVERSE() ? 0xffff : 0x0000));
         cposx = w; /* Move position to tab setting or end of viewport */
         }
      }
   else
      {
      #ifndef GHW_NO_HDW_FONT
      if( gishwfont() )
         {
         /* Assure absolute matrix positions is ok */
         ghw_setcabspos( cposx, cposy );
         cposx = ghw_getcursorxpos(); /* back to abs pixel pos */
         cposy = ghw_getcursorypos();
         if(((SGUINT) cposx + (GDISPCW-1)) > (SGUINT) rbx )
            return 1;   /* not room for symbol, skip to avoid viewport overrun */

         ghw_putch( (SGUCHAR) val );
         if ((SGUINT) cposx + GDISPCW > (SGUINT) rbx)
            cposx = rbx;
         else
            cposx += GDISPCW;
         }
      #endif
      #if (defined( GSOFT_FONTS ) && !defined (GHW_NO_HDW_FONT))
      else
      #endif
      #ifdef GSOFT_FONTS
         {
         PGSYMBOL psymbol; /* pointer to a symbol */
         GYT fh;
         fh = gfgetfh( gcurvp->pfont );

         if (cposx > rbx)
            {
            cposx = rbx+1;
            return 0; /* Line overflow by previous char, skip (waiting for \n or \r) */
            }

         psymbol = gi_getsymbol( val , gcurvp->pfont, gcurvp->codepagep);

         if( psymbol == NULL )
            {
            G_WARNING( "gputch: Character have undefined symbol" );
            return 0;
            }
         w = gsymw(psymbol);
         if (!G_IS_PARTIAL_CHAR() && ((SGUINT) cposx+w-1 > (SGUINT) rbx))
            return 1; /* not room for symbol, skip to avoid cut */

         gi_putsymbol( cposx,
                       (GYT)((cposy+1)-fh),
                       rbx, rby,
                       psymbol,
                       (GYT)(( fh <= cposy-lty ) ? 0 : fh - ((cposy-lty)+1)), /* yoffset*/
                       gi_fsymsize(gcurvp->pfont));

         #ifndef GNOTXTSPACE
         if ((SGUINT) cposx + w + chlnx > (SGUINT) rbx)
         #else
         if ((SGUINT) cposx + w > (SGUINT) rbx)
         #endif
            cposx = rbx;
         else
            {
            cposx += w;
            #ifndef GNOTXTSPACE
            if (chlnx > 0)
               gi_clr_chsp( gfgetfh(gcurvp->pfont) );
            #endif
            }
         }
      #endif /* GSOFT_FONTS */
      }
   return 0;

   }


/*
   gputchw  use these mode setting attributes
      GNORMAL,        Normal mode (not inversed, not aligned)
      GINVERSE        Inverse color (typical white on black)
      GNOSCROLL       Scroll at viewport end is suppressed
      GNO_WRAP        Use no character wrapping (only \n processing)
      GPARTIAL_LINE   Show only lines where full symbol height is visible
      GPARTIAL_CHAR   Show only symbols where full symbol width is visible
      GTRANSPERANT    Symbol background color is transperant color

      gputch  maps to gputchw using an appropriate cast
*/
void gputchw( GWCHAR val )
   {
   gi_put_prepare();

   switch( val )
      {
      case ((GWCHAR)'\n'):
         /* Make new line processing */
         gi_process_newline(0);
         break;
      case ((GWCHAR)'\r'):
         cposx = ltx;
         #ifndef GHW_NO_HDW_FONT
         if (gishwfont())
            gi_xyposalign();
         #endif
         break;
      default:
         {
         if (gi_putch( val )) /* process character, incl '\t' */
            {
            /* end of viewport line reached, character put skipped */
            if (!G_IS_NOWRAP())
               {
               if (gi_process_newline(0)) /* Make new line processing */
                  break;  /* a no-scroll condition reached, no more characters needed */
               gi_putch( val ); /* Retry on new position */
               }
            }
         }
      }

   gi_put_complete();
   }

#ifdef GFUNC_VP

void gputchw_vp( SGUCHAR vp, GWCHAR val )
   {
   GSETFUNCVP(vp, gputchw(val) );
   }

#endif /* GFUNC_VP */

#endif /* GBASIC_TEXT */

