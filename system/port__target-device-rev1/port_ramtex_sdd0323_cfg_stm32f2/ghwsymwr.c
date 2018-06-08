/************************** ghwsymwr.c *****************************

   Graphic symbol write functions for OLED display

   Write graphic symbol buffer to OLED display.

   The byte ordering for a symbol is horizontal byte(s) containing the
   first pixel row at the lowest address followed by the byte(s) in
   the pixel row below etc. The symbol is left aligned in the byte buffer.

   All coordinates are absolute pixel coordinate.

   ---------

   The SSD0323 controller is assumed to be used with a OLED module.

   The following OLED module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the OLED hardware.
              The memory buffer content is complied to the OLED
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)


   Creation date:
   Revision date:     04-01-05
   Revision Purpose:  Non-buffered mode speed optimized by use of
                      ghw_tmpbuf for read-modify-write operations
   Revision date:     270405
   Revision Purpose:  The bw parameter to ghw_wrsym(..) changed to SGUINT
                      to accomondate all combinations of display size and
                      pixel resolutions.
   Revision date:     03-04-2009
   Revision Purpose:  Grey-level symbol support and transmode support added.
   Revision date:     27-02-2015
   Revision Purpose:  Support for use of vertical storage units
                      and 90-(270) degree rotation added

   Version number: 1.4
   Copyright (c) RAMTEX Engineering Aps 2004-2015


*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */

#ifdef GVIRTUAL_FONTS
#include <gvfont.h>
#endif

#ifdef GSOFT_SYMBOLS

static GCOLOR color_blend(GCOLOR fore, GCOLOR back, SGUCHAR alpha)
   {
   SGUCHAR ialpha;
   if (alpha == 0)
      return back;
   if (alpha == GPIXMAX)
      return fore;
   ialpha = GPIXMAX-alpha;
   return (SGUCHAR) ((((SGUINT)fore)*alpha + ((SGUINT)back)*ialpha)/GPIXMAX);
   }

/*
   Copy a graphic area from a buffer using the common symbol and font format
   to the OLED memory or the graphic buffer
   bw = symbol row width in bytes
   mode is number of bits pr pixel in symbol (or true inverse for B&W character)
*/
void ghw_wrsym(GXT ltx, GYT lty, GXT rbx, GYT rby, PGSYMBYTE src, SGUINT bw, SGUCHAR mode)
   {
   GXT x;
   SGUINT sidx;
   SGBOOL transperant;
   GCOLOR fore = 0;
   GCOLOR back = 0;
   SGUCHAR spmask,spshift_start,smode; /* Symbol data handling */
   register SGUCHAR pval, spshift;
   GBUFINT sridx; /* Linear symbol data index */
   register GHWCOLOR dat;  /* Video storage unit */
   GHWCOLOR msk;
   #ifdef GHW_USING_VBYTE
   GHWCOLOR mskb;
   GYT y;
   #endif
   #ifdef GBUFFER
   GBUFINT gbufidx;
   GBUF_CHECK();
   #else
   SGINT xrd = 0;
   GHWCOLOR rddat = 0;
   SGBOOL updatepos;
   #endif

   #ifdef GVIRTUAL_FONTS
   if (bw == 0)
      return;
   #else
   if ((src == NULL) || (bw == 0))
      return;
   #endif

   /* Skip if illegal or completely outside screen */
   if ((ltx >= GDISPW) || (lty >= GDISPH) || (rby < lty) || (rbx < ltx)) return;
   /* Force reasonable values */
   GLIMITU(rby,GDISPH-1);
   GLIMITU(rbx,GDISPW-1);
   GLIMITD(bw,1);

   #ifdef GBUFFER
   invalrect( ltx, lty );
   invalrect( rbx, rby );
   #endif
   transperant = (mode & GHW_TRANSPERANT) ? 1 : 0;
   smode = mode & GHW_PALETTEMASK;
   if (smode == 0)
      smode = 1; /* Make compatible with old fonts */

   /* b&w, palette or grey-level. Check if symbol resolution is supported */
   if ((smode != 1)  && (smode != 2)  && (smode != 4)  && (smode != 8))
      {
      G_WARNING( "ghwsymw: symbol color pixel resolution not supported" );
      return;
      }
   /* initiate shift & spmsk  */
   spshift_start = 8-smode;
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

   #ifdef GHW_USING_VBYTE

   /* Select stop mask */
   mskb = startmask[GPIXEL(lty)];

   /* Loop rows */

   for(sridx = 0; ; )
      {
      SGUCHAR xb;
      SGUCHAR xbe = 0;
      #ifdef GBUFFER
      /* Calculate buffer start index for line */
      gbufidx = GINDEX(ltx,lty);
      #else
      updatepos = 1;
      xrd = -1;
      ghw_set_ypos(lty);
      #endif
      sidx = 0;                 /* Point to symbol row start */
      spshift = spshift_start;  /* Shift for normalize first symbol pixel */

      /* Select start mask */
      if (GPIXMSK(lty) == GPIXMSK(rby))
         mskb &= stopmask[GPIXEL(rby)];

      /* Loop columns */
      for (x = ltx, sidx = 0, xb = 0; x <= rbx; x++, sidx++)
         {
         GYT scidx;
         msk = mskb;
         scidx = sidx/(8/smode); /* Column offset */
         y = lty;
         /* Loop to assemble video byte from symbol pixels */
         for(dat = 0;;)
            {
            SGUCHAR sval;

            /* Fetch a symbol row byte */
            #ifdef GVIRTUAL_FONTS
            if (src == NULL)
               /* Load new symbol byte from virtual memory */
               sval = gi_symv_by(sridx + scidx);
            else
            #endif
               /* Load new symbol byte from normal memory */
               sval = src[sridx + scidx];
            #ifdef GHW_INVERTGRAPHIC_SYM
            if ((smode > 1) && (mode & GHW_GREYMODE))
               sval ^= 0xff; /* Grey level symbols is created with 0 as black, invert */
            #endif

            pval = (sval >> spshift) & spmask;   /* normalize bit */

            for(;;)
               {
               SGUCHAR cmpmsk;
               cmpmsk = GPIXMAX;
               if (smode == 1)
                  {
                  /* Convert B&W symbol to foreground / background color */
                  pval = (pval != 0) ? fore : back;
                  }
               else
                  {
                  /* Convert and map color pixels */
                  if (smode != GDISPPIXW)
                     {
                     /* Adjust symbol pixel resolution to controller */
                     #if (GDISPPIXW == 2)
                     pval >>= (smode - GDISPPIXW);
                     #else
                     #if (GDISPPIXW < 8)
                     if (smode > GDISPPIXW)
                        /* Reduce symbol grey level to hardware format (cut of lsb bits) */
                        pval >>= (smode - GDISPPIXW);
                     else
                      #endif
                        {
                        /* Expand symbol grey level to hardware format (replicate msb bits to lsb bits) */
                        pval = (pval << 2) | pval; /* 2 to 4 */
                        cmpmsk &= ~0x3; /* Exclude lsb bits for transperance compare */
                        }
                     #endif
                     }

                  /* pval now has controller resolution */
                  if (mode & GHW_GREYMODE)
                     {
                     /* Process antialiasing */
                     if (transperant)
                        {
                        /* Blend foreground color and video background */
                        if (pval == 0)
                           {
                           /* Totally transperant, remove pixel from update */
                           msk &= ~pixmsk[GPIXEL(y)];
                           break; /* Skip further processing of pixel data */
                           }
                        if (pval == GPIXMAX)
                           pval = (SGUCHAR) fore; /* No transperance */
                        else
                           {
                           /* Some transperance, blend with background */
                           GCOLOR bk;
                           #ifdef GBUFFER
                           bk = (GCOLOR)((gbuf[gbufidx] >> shiftmsk[GPIXEL(y)]) & GPIXMAX);
                           #else
                           if (xrd <= x)
                              {
                              /* Fetch new video storage unit for background blending */
                              ghw_set_xpos(x);
                              ghw_auto_rd_start();
                              rddat = ghw_auto_rd();
                              xrd = (SGINT)((SGUINT)x);
                              updatepos = 1;
                              }
                           bk = (GCOLOR) ((rddat >> shiftmsk[GPIXEL(y)]) & GPIXMAX);
                           #endif
                           pval = (SGUCHAR) color_blend(fore,bk,pval);
                           }
                        }
                     else
                        /* Blend foreground and background colors */
                        pval = (SGUCHAR) color_blend(fore,back,pval);
                     }
                  }
               if (transperant && ((mode & GHW_GREYMODE) == 0) &&
                   ((pval & cmpmsk) == (ghw_def_background & cmpmsk)))
                  {
                  /* Using symbol on-off transperance */
                  msk &= ~pixmsk[GPIXEL(y)];
                  break; /* Skip further processing of pixel data */
                  }
               /* Add pixel to storage unit */
               dat |= (((GHWCOLOR)pval) << shiftmsk[GPIXEL(y)]);
               break;
               }

            if ((GPIXEL(y+1) == 0) || (y == rby))
               break; /* Video byte assembly complete */

            /* next symbol row */
            scidx+=bw;
            y++;
            }

         /* Prepare shift for next symbol column */
         if (spshift == 0)
            spshift = spshift_start;
         else
            spshift -= smode;

         #ifdef GBUFFER
         if (msk != 0)
            {
            if (msk != GHWCOLOR_NO_MSK)
               dat = (gbuf[gbufidx] & ~msk) | (dat & msk);
            gbuf[gbufidx] = dat;
            }
         gbufidx++;
         #else
         if (msk == 0)
            /* Fully transperant */
            updatepos = 1;  /* Just skip pixels at position */
         else
            {
            if (updatepos)
               { /* First time init or jump in positon */
               ghw_set_ypos(y);
               ghw_set_xpos(x);
               updatepos = 0;
               }
            if (msk != GHWCOLOR_NO_MSK) /* Update only one pixel, do read-modify-write */
               {
               dat = (ghw_rd_x(x) & ~msk) | (dat & msk);
               updatepos = 0;
               }
            ghw_auto_wr(dat);
            }
         #endif
         }

      if (GPIXMSK(lty) == GPIXMSK(rby))
         break; /* Normal exit */

      mskb = GHWCOLOR_NO_MSK;
      y+=1;
      sridx += bw*(y-lty); /* Advance one or two symbol rows */
      lty = y;
      }

   #else  /* GHW_USING_VBYTE */

   for(sridx = 0; lty <= rby; lty++ )
      {
      SGUCHAR sval;
      msk = startmask[GPIXEL(ltx)];

      #ifdef GBUFFER
      gbufidx = GINDEX(ltx,lty);
      #else
      updatepos = 1;
      xrd = -1;
      #endif

      sidx = 0;                 /* Point to symbol row start */
      spshift = spshift_start;  /* Shift for normalize first pixel */

      /* Select start mask */
      for (x = ltx, sval = 0; x <= rbx; x++)
         {
         /* Loop video bytes */
         if (GPIXMSK(x) == GPIXMSK(rbx))
            msk &= stopmask[GPIXEL(rbx)];  /* Last column reached */

         /* Loop to assemble video byte from symbol pixels */
         for(dat = 0;;)
            {
            if (spshift == spshift_start)
               {
               #ifdef GVIRTUAL_FONTS
               if (src == NULL)
                  /* Load new symbol byte from virtual memory */
                  sval = gi_symv_by(sridx + sidx++);
               else
               #endif
                  /* Load new symbol byte from normal memory */
                  sval = src[sridx + sidx++];
               #ifdef GHW_INVERTGRAPHIC_SYM
               if ((smode > 1) && (mode & GHW_GREYMODE))
                  sval ^= 0xff; /* Grey level symbols is created with 0 as black, invert */
               #endif
               }

            pval = (sval >> spshift) & spmask;   /* normalize bit */
            if (spshift == 0)
               spshift = spshift_start;
            else
               spshift -= smode;

            for(;;)
               {
               SGUCHAR cmpmsk;
               cmpmsk = GPIXMAX;
               if (smode == 1)
                  {
                  /* Convert B&W symbol to foreground / background color */
                  pval = (pval != 0) ? fore : back;
                  }
               else
                  {
                  /* Convert and map color pixels */
                  if (smode != GDISPPIXW)
                     {
                     /* Adjust symbol pixel resolution to controller */
                     #if (GDISPPIXW == 2)
                     pval >>= (smode - GDISPPIXW);
                     #else
                     #if (GDISPPIXW < 8)
                     if (smode > GDISPPIXW)
                        /* Reduce symbol grey level to hardware format (cut of lsb bits) */
                        // pval >>= 4; /* 8 to 4 */
                        pval >>= (smode - GDISPPIXW);
                     else
                     #endif
                        {
                        /* Expand symbol grey level to hardware format (replicate msb bits to lsb bits) */
                        #if (GDISPPIXW == 4)
                        pval = (pval << 2) | pval; /* 2 to 4 */
                        cmpmsk &= ~0x3; /* Exclude lsb bits for transperance compare */
                        #else
                        register SGINT shift;
                        register SGUCHAR val;
                        val = pval;
                        pval = 0;
                        /* Exclude lsb bits for transperance compare */
                        cmpmsk &= (0xff << (GDISPPIXW-((SGINT)smode)));
                        shift = (SGINT) GDISPPIXW;
                        do
                           {
                           shift -= ((SGINT)smode);
                           if (shift >= 0)
                              pval |= (GCOLOR)(val << shift);
                           else
                              pval |= (GCOLOR)(val >> (0-shift));
                           }
                        while (shift > 0);
                        #endif
                        }
                     #endif
                     }

                  /* pval now has controller resolution */
                  if (mode & GHW_GREYMODE)
                     {
                     /* Process antialiasing */
                     if (transperant)
                        {
                        /* Blend foreground color and video background */
                        if (pval == 0)
                           {
                           /* Totally transperant, remove pixel from update */
                           msk &= ~pixmsk[GPIXEL(x)];
                           break; /* Skip further processing of pixel data */
                           }
                        if (pval == GPIXMAX)
                           pval = (SGUCHAR) fore; /* No transperance */
                        else
                           {
                           /* Some transperance, blend with background */
                           GCOLOR bk;
                           #ifndef GBUFFER
                           if (GXBYTE(xrd) != GXBYTE(x))
                              {
                              /* Fetch new video storage unit for background blending */
                              ghw_set_xpos(x);  /* Initiate OLED controller address pointers*/
                              ghw_auto_rd_start();
                              rddat = ghw_auto_rd();
                              xrd = (SGINT)((SGUINT)x);
                              updatepos = 1;
                              }
                           bk = (rddat >> ((8-GDISPPIXW)-GPIXEL(x)*GDISPPIXW)) & GPIXMAX;
                           #else
                           bk = (gbuf[gbufidx] >> ((8-GDISPPIXW)-GPIXEL(x)*GDISPPIXW)) & GPIXMAX;
                           #endif
                           pval = (SGUCHAR) color_blend(fore,bk,pval);
                           }
                        }
                     else
                        /* Blend foreground and background colors */
                        pval = (SGUCHAR) color_blend(fore,back,pval);
                     }
                  }
               if (transperant && ((mode & GHW_GREYMODE) == 0) &&
                   ((pval & cmpmsk) == (ghw_def_background & cmpmsk)))
                  {
                  /* Using symbol on-off transperance */
                  msk &= ~pixmsk[GPIXEL(x)];
                  break; /* Skip further processing of pixel data */
                  }
                           /* Append logical pixel to video storage unit */
               dat |= (((GHWCOLOR)pval) << shiftmsk[GPIXEL(x)]);
               break;
               }

            if ((GPIXEL(x+1) == 0) || (x == rbx))
               break; /* Video byte is assembled, exit loop */
            x++;
            }

         /* Mask video byte edges or transperant pixels */
         #ifdef GBUFFER
         if (msk != 0)
            {
            /* Not transperant */
            if (msk != GHWCOLOR_NO_MSK) /* Update only one pixel, do read-modify-write */
               dat = (gbuf[gbufidx] & ~msk) | (dat & msk);
            gbuf[gbufidx] = dat;
            }
         gbufidx++;
         #else
         if (msk == 0)
            /* Fully transperant */
            updatepos = 1;  /* Just skip pixels at position */
         else
            {
            /* Not fully transperant */
            if (updatepos)
               {
               ghw_set_ypos(lty);
               ghw_set_xpos(x);
               }
            if (msk != GHWCOLOR_NO_MSK) /* Update only one pixel, do read-modify-write */
               {
               dat = (ghw_rd_x(x) & ~msk) | (dat & msk);
               updatepos = 0;
               }
            ghw_auto_wr( dat );
            }
         #endif
         msk = GHWCOLOR_NO_MSK;
         }
      sridx += bw;    /* Set index to next symbol row */
      }
   #endif  /* GHW_USING_VBYTE */
   }

#endif
