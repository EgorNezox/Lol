/************************** gsympixrd.c ***************************

   Internal functions for read of individual symbol pixel data.

   To provided increased reading speed the symbol is "opened" in
   advance by a call to getsym_open(..) before the getsym_pixel(..)
   function is used to fetch the data. All symbol data formats are
   supported.

   This is a generic, random location, pixel-by-pixel fetch in contrast
   to the faster, but target controller specific, storage unit streaming
   used during symbol write via the low-level drivers.
   Also support use of symbol data stored in virtual fonts.

   Creation date:     10-02-2009

   Revision date:     10-03-2009
   Revision Purpose:  vfont support corrected
   Revision date:     27-03-2009
   Revision Purpose:  Improved grey-level support for grey-shade displays
   Revision date:     08-05-2013
   Revision Purpose:  Correction of cast in gi_getsym_open(..) to
                      remove compiler warning.
   Revision date:     04-12-2015
   Revision Purpose:  gi_getsym_pixel(..) changed to include transperance
                      preprocessing. Return a alpha+color value.
                      Encapsulates symbol format knowledge and simplifies
                      symbol pixel data fetch, used during symbol rotation
                      gi_getsym_open() prototype changed.
   Revision date:     10-03-2016
   Revision Purpose:  Adding set of GSYM_PROCESSING back color for RGB 8-24 modes.

   Version number: 1.6
   Copyright (c) RAMTEX International Aps 2009-2016
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/
#include <gi_disp.h>

#ifdef GSOFT_SYMBOLS

typedef struct
   {
   PGSYMBYTE psymdat;         /* Pointer to data area in (ROM) symbol */
   SGUINT  symw, symh, symbw; /* symbol pixel width, pixel height and row byte width */
   SGUCHAR symbpp;
   GACOLOR fore,back;
   SGUCHAR trans_color;
   SGBOOL grey_mode;
   #ifdef GHW_USING_RGB
   SGUCHAR rgb_mode;
   #endif
   } GSYM_PROCESSING;

static GSYM_PROCESSING symp;

#if (!defined( GHW_USING_HDW_PALETTE ) && (GDISPPIXW >= 8) && (GHW_PALETTE_SIZE != 0))
extern GCOLOR ghw_palette_opr[GHW_PALETTE_SIZE];
#endif

#ifdef GVIRTUAL_FONTS
  #include <gvfont.h>
#endif

/*
    Generic symbol pixel read function
    The symbol must have been selected in advance witn the getsym_open() function.

    Also support read from symbols located in virtual memory.

    Pixel data is fetched to a long to be able to accept all kind of
    pixel data formats.

    Symbols can be:
       1 bit pr pixel  (B&W)
       2 bit pr pixel  (palette index or grey level)
       4 bit pr pixel  (palette index or grey level)
       8 bit pr pixel  (palette index or grey level or RGB8)
      16 bit pr pixel  (RGB16)
      24 bit pr pixel  (RGB24)
      32 bit pr pixel  (ARGB = Transperance+RGB24)

    After fetched the symbol pixel data may be pre-processed to
    match the display controller color mode, ex doing color lookup
    or scaling.
*/
GACOLOR gi_getsym_pixel(SGUINT x, SGUINT y)
   {
   register GBUFINT sidx;
   SGUCHAR alpha = 0xff;
   SGUCHAR by;
   register GACOLOR c;

   if (symp.symbpp == 0)
      return (GACOLOR) 0;  /* Symbol has not been "opened" yet */

   if ((x >= symp.symw) || ( y >= symp.symh))
      return (GACOLOR) 0;

   #ifndef GVIRTUAL_FONTS
   if (symp.psymdat == NULL)
      return (GCOLOR) 0;
   #endif

   #ifdef GHW_USING_RGB
   if (symp.rgb_mode)
      {
      SGULONG dat; /* Use SGULONG here to be able to hold worst case symbol pixel size */
      /* rgb modes */
      sidx = ((GBUFINT)x+(GBUFINT)y*symp.symw)*(symp.symbpp>>3); /* Linear symbol data index */
      #ifdef GVIRTUAL_FONTS
      if (symp.psymdat == NULL)
         {
         dat = (SGULONG) gi_symv_by((GFONTBUFIDX)sidx);
         if (symp.symbpp > 8)
            {
            dat = (dat << 8) | (SGULONG) gi_symv_by((GFONTBUFIDX)(++sidx));
            if (symp.symbpp > 16)
               {
               dat = (dat << 8) | (SGULONG) gi_symv_by((GFONTBUFIDX)(++sidx));
               if (symp.symbpp > 24)
                  alpha = gi_symv_by(++sidx);
               }
            }
         }
      else
      #endif
         {
         dat = (SGULONG) (symp.psymdat[sidx]);
         if (symp.symbpp > 8)
            {
            dat = (dat << 8) | (SGULONG) (symp.psymdat[++sidx]);
            if (symp.symbpp > 16)
               {
               dat = (dat << 8) | (SGULONG) (symp.psymdat[++sidx]);
               if (symp.symbpp > 24)
                  alpha = symp.psymdat[++sidx];
               }
            }
         }

      if (symp.symbpp != GDISPPIXW)
         {
         /* Symbol use a different color resolution than controller configuration,
            do conversion to controller color */
         c = (GACOLOR) ghw_color_conv(dat,symp.symbpp);
         }
      else
         c = (GACOLOR) dat;

      if (symp.symbpp <= 24)
         goto check_on_off_trans;
      goto add_alpha;
      }
   #endif /* GHW_USING_RGB */

   /* Multiple pixels (1-8) in byte modes */
   sidx = (GBUFINT)symp.symbw*y + (GBUFINT)(x /(8/symp.symbpp));
   #ifdef GVIRTUAL_FONTS
   if (symp.psymdat == NULL)
      by = gi_symv_by(sidx);
   else
   #endif
      by = symp.psymdat[sidx];

   if (symp.symbpp == 1)
      {
      by = (by >> (7 - (x & 7))) & 0x1;  /* Normalize to 1 bit */
      return (by != 0) ? symp.fore : symp.back;    /* Quick return (colors is preprocessed) */
      }

   #ifdef GHW_INVERTGRAPHIC_SYM
   //#ifdef GHW_USING_RGB
   if (symp.grey_mode != 0)
   //#endif
      by ^= 0xff; /* Grey-level symbols was created with 0 as black, invert */
   #endif

   /* 2 or 4 bit pr pixel */
   if (symp.symbpp == 4)
      by = (((x&1)!=0) ? by : (by>>4)) & 0xf; /* Normalize to 4 bits */
   else
   if (symp.symbpp == 2)
      by = (by >> ((~(x<<1))&0x6)) & 0x3;     /* Normalize to 2 bits */

   if (symp.grey_mode != 0)
      {
      /* Convert to uniform 0-0xff grey level */
      if (symp.symbpp == 4)
         by *= 17; /* (by*0xff)/0xf */
      else
      if (symp.symbpp == 2)
         by *= 85; /* (by*0xff)/0x3 */

      if (symp.trans_color)
         /* Return semi-transperant color. Actual background blending done in caller */
         return GI_ALPHA_TO_ACOLOR(by) | symp.fore;
      else
         /* Return opaque color (created by blending foreground and background) */
         return GI_ALPHA_TO_ACOLOR(0xff) | GACOLOR_BLEND( symp.fore, by, symp.back, (0xff-by));
      }

   #ifdef GHW_USING_RGB
   /* We use RGB mode (and neither hardware palette, nor grey-level hardware) */
   #if (!defined( GHW_USING_HDW_PALETTE ) && (GDISPPIXW >= 8) && (GHW_PALETTE_SIZE != 0))
   /* A RGB software palette is defined. Do lookup */
   if ((SGUINT)by >= (SGUINT)GHW_PALETTE_SIZE)
      by = GHW_PALETTE_SIZE-1; /* Just in case, should never be needed */
   c = (GACOLOR) ghw_palette_opr[by]; /* Convert index to color */
   #else
   return 0; /* Just in case, should never be needed, 2,4,8 bit palette symbols not supported */
   #endif

   #else
   /* We use grey-level mode or grey hardware palette mode (i.e. using pixel value directly) */
   if (symp.symbpp != GDISPPIXW)
      {
      /* Convert pixel to bit resolution used by display controller */
      if (symp.symbpp > GDISPPIXW)
         /* Reduce symbol grey level to hardware format (cut of lsb bits) */
         by >>= (symp.symbpp - GDISPPIXW);
      else
         {
         /* Expand symbol grey level to hardware format (replicate msb bits to lsb bits) */
         GCOLOR val;
         SGINT shift;
         val = by;
         by = 0;
         shift = (SGINT) GDISPPIXW;
         do
            {
            shift -= ((SGINT)symp.symbpp);
            if (shift >= 0)
               by |= (GCOLOR)(val << shift);
            else
               by |= (GCOLOR)(val >> (0-shift));
            }
         while (shift > 0);
         }
      }
   c = (GACOLOR) by;
   #endif /* GHW_USING_RGB */

   #ifdef GHW_USING_RGB
   check_on_off_trans:
   #endif
   if (symp.trans_color)
      if (c == symp.back )
         return (GACOLOR) 0;
   #ifdef GHW_USING_RGB
   add_alpha:
   #endif
   return GI_ALPHA_TO_ACOLOR(alpha) | c;
   }


/*
   void gi_getsym_open( PGSYMBOL psymbol );

   Open for symbol pixel data read, i.e. prepare for read of symbol pixels
   with gi_getsym_pixel(..).

   The read posibility can be "closed" again by using gi_getsym_open( NULL )
   Then gi_getsym_pixel(..)  will just return 0 without risk of side effects.

   Includes support for pixel read of font symbols in virtual memory.

   symmode can be the mode flags GNORMAL GINIVERSE GTRANSPERANT
*/
SGUCHAR gi_getsym_open( PGSYMBOL psymbol, SGUINT symbytewidth, GMODE symmode)
   {
   SGBOOL colorflg;
   SGUCHAR smode;
   #ifdef GVIRTUAL_FONTS_DYN
   if (gvfsym_open(psymbol))
   #else
   if (psymbol == NULL)
   #endif
      {
      symp.psymdat = (PGSYMBYTE) NULL;
      symp.symbpp = 0;
      return 0;
      }

   symp.symw = gsymw(psymbol);
   symp.symh = gsymh(psymbol);
   symp.trans_color = (symmode & GTRANSPERANT) ? 1 : 0;
   #ifdef GHW_USING_RGB
   symp.rgb_mode = 0;
   #endif
   symp.grey_mode = 0;

   /* Bit pr pixel format  for symbol */
   if (giscolor(psymbol))
      {
      /* Color mode (RGB, palette_index, grey_level) */
      colorflg = 1;
      smode = (SGUCHAR) gcolorbits(psymbol);
      switch (smode)
         {
         /* Palette or direct color modes (grey mode symbol) */
         case 2:
         case 4:
         #if ((GHW_PALETTE_SIZE == 256) || !defined(GHW_USING_RGB))
         case 8:
         #endif
            /* Transperant mode on-off transperance color */
            symp.back = (GACOLOR)(gcurvp->background);
            break;
         /* Grey level modes */
         case 0x42:
         case 0x44:
         case 0x48:
            symp.grey_mode = 1;
            symp.fore = (GACOLOR)(symmode & GINVERSE) ? gcurvp->background : gcurvp->foreground;
            if (symp.trans_color == 0)
               symp.back = (GACOLOR)(symmode & GINVERSE) ? gcurvp->foreground : gcurvp->background;
            break;
         /* RGB modes */
         #ifdef GHW_USING_RGB
         case 31:  /* (old style palette bit limiting ) 0x1f = 32 bpp */
            {
            smode = 32;
            } /* fallthrough */
         #if (GHW_PALETTE_SIZE < 256)
         case 8:
         #endif
         case 16:
         case 24:
            {
            if (symp.trans_color)
               symp.back = gcurvp->background; /* Pixel color for on-off transperance */
            } /* fallthrough */
         case 32:
            symp.rgb_mode  = 1;
            break;
         #endif
         default:
            symp.psymdat = (PGSYMBYTE) NULL;
            symp.symbpp = 0;
            return 0; /* Illegal / unsupported symbol format type */
         }
      symp.symbpp = smode & 0x3f;
      }
   else
      {
      /* B&W mode */
      colorflg = 0;
      symp.symbpp = 1;
      if (symmode & GINVERSE)
         {
         symp.fore = GI_OPAQUE | ((GACOLOR)(gcurvp->background));
         symp.back = symp.trans_color ? GI_FULLTRANS : GI_OPAQUE | ((GACOLOR)(gcurvp->foreground));
         }
      else
         {
         symp.fore = GI_OPAQUE | ((GACOLOR)(gcurvp->foreground));
         symp.back = symp.trans_color ? GI_FULLTRANS : GI_OPAQUE | ((GACOLOR)(gcurvp->background));
         }
      }

   /* width of symbol in bytes */
   if (symbytewidth != 0)
      symp.symbw = symbytewidth; /* Use of font symbol storage width */
   else
   /* Use default, symbol storage width matches content */
   if (symp.symbpp <= 8)
      symp.symbw = (symp.symw*((SGUINT)symp.symbpp)+7)/8;
   else
      symp.symbw = symp.symw*(SGUINT)(symp.symbpp/8);


   #ifdef GVIRTUAL_FONTS
   if (gissymbolv(psymbol))
      {
      gi_symv_open( psymbol, symp.symbw, 0 ); /* Preset virtual symbol interface */
      symp.psymdat = (PGSYMBYTE) NULL;
      }
   else
   #endif
      {
      if (colorflg)
         symp.psymdat = (PGSYMBYTE) (&(((PGCSYMBOL)psymbol)->b[0]));
      else
         symp.psymdat = (PGSYMBYTE) (&(((PGBWSYMBOL)psymbol)->b[0]));
      }
   return 1;
   }

#endif /* GSOFT_SYMBOLS */

