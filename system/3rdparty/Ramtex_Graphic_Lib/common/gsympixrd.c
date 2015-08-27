/************************* gsympixrd.c ****************************

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

   Version number: 1.3
   Copyright (c) RAMTEX Engineering Aps 2009-2011

*********************************************************************/

#include <gi_disp.h>

#ifdef GSOFT_SYMBOLS
#ifdef GVIRTUAL_FONTS
#include <gvfont.h>
#endif

/* Format variables which are stable during multiple
   gi_getsym_pixel() calls when reading symbol data */
static PGSYMBYTE psymdat;  /* Pointer to data area in (ROM) symbol */
static SGUINT symw, symh, symbw; /* symbol pixel width, pixel height and row byte width */
static SGUCHAR mode;
#ifdef GHW_USING_RGB
static SGBOOL grey_mode;
#endif
static GCOLOR fore,back;
#ifdef GHW_USING_RGB
static SGBOOL convert_flg;
static SGBOOL rgb_mode;
#endif

#if (!defined( GHW_USING_HDW_PALETTE ) && (GDISPPIXW >= 8) && (GHW_PALETTE_SIZE != 0))
extern GCOLOR ghw_palette_opr[GHW_PALETTE_SIZE];
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
GCOLOR gi_getsym_pixel( SGUINT x, SGUINT y)
   {
   SGULONG dat; /* Use SGULONG here to be able to hold worst case symbol pixel size */
   SGUCHAR by;

   if ( mode == 0)
      return (GCOLOR) 0;  /* Symbol has not been "opened" yet */

   if ( x >= symw)
      x= symw-1;   /* Just in case, should never be needed */
   if ( y >= symh)
      y = symh-1;  /* Just in case, should never be needed */

   #ifndef GVIRTUAL_FONTS
   if (psymdat == NULL)
      return (GCOLOR) 0;
   #endif

   #ifdef GHW_USING_RGB
   if (rgb_mode)
      {
      /* rgb modes */
      #ifdef GVIRTUAL_FONTS
      if (psymdat == NULL)
         {
         GFONTBUFIDX sidx;             /* Linear symbol data index */
         sidx = (x+symw*y)*(mode/8);
         dat = (SGULONG) gi_symv_by(sidx);
         if (mode > 8)
            {
            dat = (dat << 8) | (SGULONG) gi_symv_by(++sidx);
            if (mode > 16)
               {
               dat = (dat << 8) | (SGULONG) gi_symv_by(++sidx);
               if (mode > 24)
                  dat = (dat << 8) | (SGULONG) gi_symv_by(++sidx);
               }
            }
         }
      else
      #endif
         {
         PGSYMBYTE p;
         p = &psymdat[((GBUFINT)x+(GBUFINT)symw*y)*(mode/8)];
         dat = (SGULONG) (*p);
         if (mode > 8)
            {
            p++;
            dat = (dat << 8) | (SGULONG) (*p);
            if (mode > 16)
               {
               p++;
               dat = (dat << 8) | (SGULONG) (*p);
               if (mode > 24)
                  {
                  p++;
                  dat = (dat << 8) | (SGULONG) (*p);
                  }
               }
            }
         }

      if (mode != GDISPPIXW)
         {
         /* Symbol use a different color resolution than controller configuration,
            do conversion to controller color */
         return ghw_color_conv(dat,mode);
         }

      /* Return RGB directly */
      return (GCOLOR) dat;
      }
   #endif /* GHW_USING_RGB */

   /* Multiple pixels (1-8) in byte modes */
   dat = (SGULONG)symbw*y + (SGULONG)(x /(8/mode));
   #ifdef GVIRTUAL_FONTS
   if (psymdat== NULL)
      by = gi_symv_by((GBUFINT)dat);
   else
   #endif
      by = psymdat[(GBUFINT)dat];


    #ifdef GHW_INVERTGRAPHIC_SYM
    #ifdef GHW_USING_RGB
    if ((mode > 1) && (grey_mode != 0))
    #else
    if (mode > 1)
    #endif
       by ^= 0xff; /* Symbols was created with 0 as black, invert */
    #endif


   if (mode == 1)
      {
      by = (by >> (7 - (x & 7))) & 0x1;   /* Normalize to 1 bit */
      return (GCOLOR) ((by != 0) ? fore : back);
      }

   if (mode == 4)
      by = (by >> (4 - 4*(x & 1))) & 0xf; /* Normalize to 4 bits */
   else
   if (mode == 2)
      by = (by >> (6 - 2*(x & 3))) & 0x3; /* Normalize to 2 bits */

   #ifdef GHW_USING_RGB
   /* We use RGB mode (and neither hardware palette, nor grey-level hardware) */
   if (convert_flg != 0)
      {
      /* Do color conversion */
      if (grey_mode != 0)
         {
         /* Convert to uniform 0-0xff grey level */
         if (mode == 8)
            return (GCOLOR) by;
         if (mode == 4)
            return ((GCOLOR) by )*17; /* (by*0xff)/0xf */
         return ((GCOLOR) by)*85;     /* (by*0xff)/0x3 */
         }

      #if (!defined( GHW_USING_HDW_PALETTE ) && (GDISPPIXW >= 8) && (GHW_PALETTE_SIZE != 0))
      /* A RGB software palette is defined. Do lookup */
      if ((SGUINT)by >= (SGUINT)GHW_PALETTE_SIZE)
         by = GHW_PALETTE_SIZE-1; /* Just in case, should never be needed */
      return (GCOLOR) ghw_palette_opr[by]; /* Convert color */
      #endif
      }
   #else
   /* We use grey-level mode or palette */
   #if (defined( GHW_USING_GREY_LEVEL) || (GDISPPIXW == 1))
   if (mode != GDISPPIXW)
   #else
   if ((grey_mode != 0) && (mode != GDISPPIXW))
   #endif
      {
      /* Symbol is grey level.
         Assume we are using a grey level display or a grey level palette */
      if (mode > GDISPPIXW)
         /* Reduce symbol grey level to hardware format (cut of lsb bits) */
         by >>= (mode - GDISPPIXW);
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
            shift -= ((SGINT)mode);
            if (shift >= 0)
               by |= (GCOLOR)(val << shift);
            else
               by |= (GCOLOR)(val >> (0-shift));
            }
         while (shift > 0);
         }
      }
   #endif /* GHW_USING_RGB */
   return (GCOLOR) by;
   }


/*
   void gi_getsym_open( PGSYMBOL psymbol );

   Open for symbol pixel data read, i.e. prepare for read of symbol pixels
   with gi_getsym_pixel(..).

   The read posibility can be "closed" again by using gi_getsym_open( NULL )
   Then gi_getsym_pixel(..)  will just return 0 without risk of side effects.

   Includes support for pixel read of font symbols in virtual memory.
*/
SGUCHAR gi_getsym_open( PGSYMBOL psymbol, SGUINT symbytewidth)
   {
   SGBOOL colorflg;
   SGUCHAR smode;
   if (psymbol == NULL)
      {
      psymdat = (SGUCHAR PFCODE *)NULL;
      mode = 0;
      return 0;
      }

   symw = gsymw(psymbol);
   symh = gsymh(psymbol);
   #ifdef GHW_USING_RGB
   rgb_mode = 0;
   convert_flg = 0;
   #endif

   /* Bit pr pixel format  for symbol */
   if (giscolor(psymbol))
      {
      /* Color mode (RGB, palette_index, grey_level) */
      colorflg = 1;
      smode = (SGUCHAR) gcolorbits(psymbol);
      /* If grey level symbol there is no color conversion */
      #ifdef GHW_USING_RGB
      grey_mode = (smode & GHW_GREYMODE) ? 1 : 0;
      #endif
      mode = smode & 0x3f;

      #ifdef GHW_USING_RGB
      if (mode > 8)
         rgb_mode = 1;
      #if (GHW_PALETTE_SIZE < 256)
      else
      if ((mode == 8) && (grey_mode == 0))
         rgb_mode = 1;   /* 8 bit RGB mode */
      #endif
      #ifndef GHW_USING_HDW_PALETTE
      else
         {
         if (mode != GDISPPIXW)
            convert_flg = 1; /* Do soft palette lookup or grey scaling*/
         }
      #endif
      #endif
      }
   else
      {
      /* B&W mode */
      #ifdef GHW_USING_RGB
      rgb_mode = 0;
      convert_flg = 1; /* Add convert of b&w to foreground color */
      grey_mode = 1;
      #endif
      colorflg = 0;
      smode = mode = 1;
      if (ggetmode() & GINVERSE)
         {
         fore = ghw_def_background;
         back = ghw_def_foreground;
         }
      else
         {
         fore = ghw_def_foreground;
         back = ghw_def_background;
         }
      }

   /* width of symbol in bytes */
   if (symbytewidth != 0)
      symbw = symbytewidth;
   else
   if (mode <= 8)
      symbw = (symw*((SGUINT)mode)+7)/8;
   else
      symbw = symw*(SGUINT)(mode/8);


   #ifdef GVIRTUAL_FONTS
   if (gissymbolv(psymbol))
      {
      gi_symv_open( psymbol, symbw, 0 ); /* Preset virtual symbol interface */
      psymdat = (SGUCHAR PFCODE *) NULL;
      }
   else
   #endif
      {
      if (colorflg)
         psymdat = (PGSYMBYTE) (&(((PGCSYMBOL)psymbol)->b[0]));
      else
         psymdat = (PGSYMBYTE) (&(((PGBWSYMBOL)psymbol)->b[0]));
      }
   return smode;
   }

#endif /* GSOFT_SYMBOLS */







































