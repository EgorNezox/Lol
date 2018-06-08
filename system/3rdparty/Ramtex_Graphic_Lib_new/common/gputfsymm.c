/************************* gputfsymm.c ********************************

   Variant of the gputfsym(..) function which handle auto alignment in
   viewport and flicker free viewport clear (in the same way as gputs() )

   The xpad, ypad sets distance inwards from a viewport edge to the symbol.
   The edge used as reference is defined by the GALIGN_xx mode settings:

   Supported mode flags:

      GALIGN_LEFT     Font symbol is placed xpad from the left viewport edge (default)
      GALIGN_RIGHT    Font symbol is placed xpad from the right viewport edge
      GALIGN_HCENTER  Font symbol is centered horizontally (xpad is dont care)
      none of above   or GALIGN not enabled: xpad is a normal x coordinate

      GALIGN_TOP      Font symbol is placed ypad from the top (default)
      GALIGN_BOTTOM   Font symbol is placed ypad from the bottom
      GALIGN_VCENTER  Font symbol is centered vertically (ypad is dont care)
      none of above   or GALIGN not enabled: ypad is a normal y coordinate

      GVPCLR_UP       Clear vp area above symbol
      GVPCLR_DOWN     Clear vp area below symbol
      GVPCLR_LEFT     Clear vp area to the left of symbol
      GVPCLR_RIGHT    Clear vp area to the right of symbol
      GVPCLR          Clear all around symbol

      GTRANSPERANT    Symbol background color is transperant color


   Revision date:
   Revision Purpose:

   Version number: 1
   Copyright (c) RAMTEX International Aps 2016
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/
#include <gi_disp.h>

/*
   Output symbol with support for auto alignment and auto vp clear
*/
void gputfsymm( GXT xpad, GYT ypad, SGUINT index, PGFONT pfont, GMODE mode )
   {
   PGSYMBOL psym;
   GMODE oldmode;
   GXT w,vpw;
   GYT h,vph;

   if (pfont == NULL)
      return;

   vpw = ggetvpw();
   vph = ggetvph();
   if ((xpad >= vpw) || (ypad >= vph))
      {
      G_WARNING( "outside viewport " );
      return;
      }

   /* Get pointer to symbol (also do VF open if virtual font) */
   if ((psym = ggetfsym(index,pfont)) == NULL)
      {
      /* Warning already issued in gi_getsymbol(..) */
      return;
      }

   /* Viewport data checking done here */

   w=gsymw(psym);
   h=gsymh(psym);

   #ifdef GS_ALIGN
   /* Auto alin symbol in viewport */
   switch (mode & GALIGN_HCENTER)
      {
      case GALIGN_HCENTER:
         xpad = (vpw-w)/2;
         break;
      case GALIGN_RIGHT:
         // Padding from right, but start within viewport takes precedence
         xpad = (vpw >= (w+xpad)) ? (vpw-(w+xpad)) : (w > vpw) ? 0 : vpw - w;
         break;
      case GALIGN_LEFT:
      default:
         break;
      }

   switch (mode & GALIGN_VCENTER)
      {
      case GALIGN_VCENTER:
         ypad = (vph-h)/2;
         break;
      case GALIGN_BOTTOM:
         // Padding from bottom, but start within viewport takes precedence
         ypad = (vph > (h+ypad)) ? (vph-(h+ypad)) : (h > vph) ? 0 : vph - h;
         break;
      case GALIGN_TOP:
      default:
         break;
      }
   #endif /* GS_ALIGN */

   glcd_err = 0;   /* Reset HW error flag */
   gi_cursor( 0 ); /* kill cursor */

   /* xpad, ypad to abosolute coordinates */
   xpad += gcurvp->lt.x;
   ypad += gcurvp->lt.y;

   // Override viewport defaults ( to do GTRANSPERANT handling )
   oldmode = gsetmode( mode & ~(GALIGN_HCENTER|GALIGN_VCENTER|GVPCLR));

   /* draw symbol in absolute cord */
   gi_putsymbol( xpad,ypad, gcurvp->rb.x, gcurvp->rb.y, psym,0,gi_fsymsize(pfont));


   if ((mode & GVPCLR) && ((mode & GTRANSPERANT) == 0))
      { /* Do flicker free clear of area around symbol according to mode settings */
      GXT xb,xe;
      SGUINT filldat;
      filldat = (SGUINT)(G_IS_INVERSE() ? 0xffff : 0);

      if ((mode & GVPCLR_LEFT) && (xpad > gcurvp->lt.x))
         {
         ghw_fill(gcurvp->lt.x,ypad,xpad-1,ypad+h-1,filldat);
         xb = gcurvp->lt.x; /* Include left top / bottom area in optional top / bottom clear */
         }
      else
         {
         xb = xpad;
         }
      xe = gcurvp->rb.x;
      if ((mode & GVPCLR_RIGHT) && (((xpad+w)-1) < xe))
         {
         ghw_fill(xpad+w,ypad,xe,ypad+h-1,filldat);
         }
      else
         {
         xe = (xpad+w)-1; /* Include right top / bottom area in optional top / bottom clear */
         }
      if ((mode & GVPCLR_UP) && (ypad > gcurvp->lt.y))
         {
         ghw_fill(xb,gcurvp->lt.y,xe,ypad-1,filldat);
         }
      if ((mode & GVPCLR_DOWN) && ((ypad+h) < gcurvp->rb.y))
         {
         ghw_fill(xb,ypad+h,xe,gcurvp->rb.y,filldat);
         }
      }

   /* Restore settings */
   gsetmode( oldmode );

   ghw_updatehw();
   gi_cursor( 1 );     /* set cursor */

   gi_calcdatacheck(); /* correct VP to new settings */
   }


#ifdef GFUNC_VP

void gputfsymm_vp( SGUCHAR vp, GXT xpad, GYT ypad, SGUINT index, PGFONT pfont, GMODE mode )
   {
   GSETFUNCVP(vp, gputfsymm( xpad, ypad, index, pfont, mode ));
   }

#endif /* GFUNC_VP */

