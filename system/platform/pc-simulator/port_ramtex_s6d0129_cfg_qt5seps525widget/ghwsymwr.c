/************************** ghwsymwr.c *****************************

   Graphic symbol write functions for LCD display

   Write graphic symbol buffer to LCD display.

   The byte ordering for a symbol is horizontal byte(s) containing the
   first pixel row at the lowest address followed by the byte(s) in
   the pixel row below etc. The symbol is left aligned in the byte buffer.

   All coordinates are absolute pixel coordinate.

   ---------

   The S6D0129 controller is assumed to be used with a LCD module.

   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is complied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)


   Revision date:    31-01-08
   Revision Purpose: GHW_TRANSPERANTsupport added.
   Revision date:    07-03-09
   Revision Purpose: Extended symbol color modes added. Now supports:
            Normal modes
               1 bit b&w symbols (fore / back color)
               2,4,8 bit grey level symbols (fore / back color blending)
               2,4 bit palette mode
               8,16,24,32 bit RGB symbol
            Transperance modes
               1 bit b&w symbols (pixel off v background color match)
               2,4,8 bit grey level (color blending with screen pixel color)
               2,4 bit palette mode (pixel off v background color match)
               8,16,24 bit RGB symbol (pixel off v background color match)
               32 bit RGBA symbol (color blending with screen pixel color)
   Revision date:     30-04-09
   Revision Purpose:  Palette feature can be turned off with GHW_PALETTE_SIZE == 0
   Revision date:
   Revision Purpose:

   Version number: 1.2
   Copyright (c) RAMTEX Engineering Aps 2007-2009

*********************************************************************/
#include <s6d0129.h>   /* lcd controller specific definements */

#ifdef GVIRTUAL_FONTS
#include <gvfont.h>
#endif

#ifdef GSOFT_SYMBOLS

/*
   Copy a graphic area from a buffer using the common symbol and font format
   to the LCD memory or the graphic buffer
   bw = symbol row width in bytes
   mode is number of bits pr pixel in symbol (or true inverse for B&W character)
*/

void ghw_wrsym(GXT ltx, GYT lty, GXT rbx, GYT rby, PGSYMBYTE src, SGUINT bw, SGUCHAR mode)
   {
   GCOLOR fore = 0;
   GCOLOR back = 0;
   SGUCHAR sval = 0;
   SGUCHAR spshift_start = 0;
   SGUCHAR spmask = 0;
   SGUCHAR spshift;
   GCOLOR color;
   GXT x;
   SGUINT sidx;
   SGBOOL transperant;
   GBUFINT sridx; /* Linear symbol data index */
   SGUCHAR smode;

   #ifdef GBUFFER
   GBUFINT gbufidx;
   GBUF_CHECK();
   #else
   SGBOOL updatepos;
   #endif

   #ifdef GVIRTUAL_FONTS
   if (bw == 0)
      return;
   #else
   if ((src == NULL) || (bw == 0))
      return;
   #endif

   /* Force reasonable values */
   GLIMITU(ltx,GDISPW-1);
   GLIMITU(lty,GDISPH-1);
   GLIMITD(rby,lty);
   GLIMITU(rby,GDISPH-1);
   GLIMITD(rbx,ltx);
   GLIMITU(rbx,GDISPW-1);
   GLIMITD(bw,1);

   #ifdef GBUFFER
   invalrect( ltx, lty );
   invalrect( rbx, rby );
   #endif

   transperant = (mode & GHW_TRANSPERANT) ? 1 : 0;
   smode = mode & GHW_PALETTEMASK;

   /* Is it a color/gray scale symbol or a 'b&w' symbol ? */
   if (smode == 0)
      smode = 1; /* Make compatible with old fonts */
   if (smode <= 8)
      {
      /* b&w, palette or grey-level. Check if symbol resolution is supported */
      if ((smode != 1)  && (smode != 2)  && (smode != 4)  && (smode != 8))
         {
         G_WARNING( "ghwsymw: symbol color pixel resolution not supported" );
         return;
         }
      /* initiate shift & spmsk ( only used when mode < 8 or ==8 and greysymbol) */
      spshift_start = 8-smode;
      /* initiate spmsk */
      spmask = (1<<smode)-1;    /* 1 -> 0x01, 2->0x03,  4->0x0f, 8->0xff */

      if ((smode == 1) || (mode & GHW_GREYMODE))
         {
         /* "B&W" symbol or grey level */
         if ((mode & GHW_INVERSE) == 0)
            {
            fore = ghw_def_foreground;  /* Normal 'b&w' */
            back = ghw_def_background;
            }
         else
            {
            fore = ghw_def_background;  /* Inverse 'b&w' */
            back = ghw_def_foreground;
            }
         }
      }
   else
      {
      /* RGB modes. Check if symbol resolution is supported */
      if ((smode != 16) && (smode != 18) && (smode != 24) && (smode != 0x1f))
         {
         G_WARNING( "ghwsymw: symbol color pixel resolution not supported" );
         return;
         }
      mode &= ~GHW_GREYMODE; /* Just in case */
      }

   #ifndef GBUFFER
   if (!transperant)
      ghw_set_xyrange(ltx,lty,rbx,rby);
   updatepos = 0;
   #endif

   for (sridx = 0;lty <= rby; lty++)
      {
      /* Loop character rows */
      /* Calculate buffer start index for line */
      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty);
      #else
      if (transperant)
         {
         ghw_set_xyrange(ltx,lty,rbx,rby);
         updatepos = 0;
         }
      #endif

      sidx = 0;                 /* Point to symbol row start */
      spshift = spshift_start;  /* Shift for normalize first pixel */

      for (x = ltx; x <= rbx; x++)
         {
         SGUCHAR pval = 0;
         if ((smode < 8) || ((smode == 8) && (mode & GHW_GREYMODE)))
            {
            /* Extract color from symbol byte */
            if (spshift == spshift_start)
               {
               #ifdef GVIRTUAL_FONTS
               if (src == NULL)
                  /* Load new symbol byte from virtual memory */
                  sval = gi_symv_by(sridx + sidx);
               else
               #endif
                  /* Load new symbol byte from normal memory */
                  sval = src[sridx + sidx];
               sidx++;
               }

            pval = (sval >> spshift) & spmask;   /* normalize bit */
            if (spshift == 0)
               spshift = spshift_start;
            else
               spshift -= smode;

            if (smode == 1)
               {
               /* Convert B&W symbol to foreground / background color */
               color = (pval != 0) ? fore : back;
               }
            else
               {
               /* Convert and map color pixels */
               if (mode & GHW_GREYMODE)
                  {
                  #ifdef GHW_INVERTGRAPHIC_SYM
                  pval = (pval ^ 0xff) & spmask; /* Grey level symbols assume 0 is black, invert */
                  #endif
                  /* normalize grey level to blending value (0-255) */
                  pval = (pval * (SGUINT)0xff)/(((SGUINT)1<<smode)-1);
                  if (!transperant)
                     /* Create color from blending foreground and background colors */
                     color = ghw_color_blend(fore,back,pval);
                  else
                     color = fore;
                  }
               else
                  {
                  #if (GHW_PALETTE_SIZE > 0)
                  /* Make a palette lookup */
                  color = ghw_palette_opr[pval]; /* 2,4 bit pr pixel modes */
                  #else
                  G_WARNING( "ghwsymw: palette lookup not enabled" );
                  return;
                  #endif
                  }
               }
            }
         else
            {
            SGULONG col; /* Use long here to hold worst case color size */
            #ifdef GVIRTUAL_FONTS
            if (src == NULL)
               {
               col = (SGULONG)(gi_symv_by(sridx + sidx++));
               if (smode >= 16)
                  {
                  col = (col<<8) + (SGULONG)(gi_symv_by(sridx + sidx++));
                  if (smode > 16)
                     {
                     col = (col<<8) + (SGULONG)(gi_symv_by(sridx + sidx++));
                     if (smode > 24)
                        {
                        if (transperant)
                           pval = gi_symv_by(sridx + sidx);
                        else
                           pval = 0xff;
                        sidx++;
                        }
                     }
                  }
               }
            else
            #endif
               {
               col = (SGULONG)(src[sridx + sidx++]);
               if (smode >= 16)
                  {
                  col = (col<<8) + (SGULONG)(src[sridx + sidx++]);
                  if (smode > 16)
                     {
                     col = (col<<8) + (SGULONG)(src[sridx + sidx++]);
                     if (smode > 24)
                        {
                        if (transperant)
                           pval = src[sridx + sidx];
                        else
                           pval = 0xff;
                        sidx++;
                        }
                     }
                  }
               }
            if (smode != GDISPPIXW)
               {
               /* Symbol use a different color resolution than controller configuration,
                  do conversion to controller color */
               color = ghw_color_conv(col,smode);
               }
            else
               color = (GCOLOR) col;
            }

         for (;;)
            {
            if (transperant)
               {
               if ((mode & GHW_GREYMODE) || (smode > 24))
                  { /* Blending with background pixel color */
                  if (pval == 0x00)
                     {
                     /* Fully transperant just skip pixel */
                     #ifndef GBUFFER
                     updatepos = 1;
                     #endif
                     break;
                     }
                  if (pval != 0xff)
                     {
                     #if (!defined( GHW_NO_LCD_READ_SUPPORT ) || defined(GBUFFER))
                      /* Transperant symbol mode, Read background pixel to do blending */
                      #ifdef GBUFFER
                      back = gbuf[gbufidx];
                      #else
                      ghw_set_xyrange(x,lty,rbx,rby);
                      #ifndef GHW_NO_RDINC
                      ghw_auto_rd_start();
                      back = ghw_auto_rd();
                      #else
                      back = ghw_rd(x,lty);
                      #endif
                      updatepos = 1;
                      #endif
                     #endif
                     color = ghw_color_blend(color,back,pval);
                     }
                  }
               else
                  {
                  /* On off transperance by symbol color match */
                  if (color == ghw_def_background)
                     {
                     /* Fully transperant just skip pixel */
                     #ifndef GBUFFER
                     updatepos = 1;
                     #endif
                     break;
                     }
                  }
               }

            #ifdef GBUFFER
            gbuf[gbufidx] = color;
            #else
            /* Write and auto increment */
            if (updatepos)
               { /* previous pixels was transperant, update position */
               ghw_set_xyrange(x,lty,rbx,rby);
               updatepos = 0;
               }
            ghw_auto_wr( color );
            #endif
            break;
            }
         #ifdef GBUFFER
         gbufidx++;
         #endif
         }
      sridx += bw;    /* Set index to next symbol row */
      }
   }

#endif

