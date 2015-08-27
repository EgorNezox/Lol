/*
   Internal function for symbols.
   writes a symbol at absolute coordinates, processes symbol header
   and viewport mode information.
   Absolute coordinates used.

   Revision date:     03-11-03
   Revision Purpose:  Color support added

   Revision date:     010205
   Revision Purpose:  Correction of overflow error when large symbol is
                      cropped near screen edges.

   Revision date:     270405
   Revision Purpose:  The bw parameter to ghw_wrsym(..) changed to SGUINT
                      to accomondate all combinations of display size and
                      pixel resolutions.

   Revision date:     130106
   Revision Purpose:  The symsize parameter changed to GBUFINT type to
                      to be adaptive to large (color) symbols

   Revision date:     290606
   Revision Purpose:  GTRANSPERANT mode support added

   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption

   Revision date:     27-07-08
   Revision Purpose:  Vfont format type check added

   Revision date:     17-04-2009
   Revision Purpose:  Virtual font structure split in a RAM dependent and a constant
                      data structure
                      (to handle compilers using non-standard C conformant pointers)
   Revision date:     27-04-2009
   Revision Purpose:  Masking for illegal grey-level flags.

   Revision date:     03-06-2009
   Revision Purpose:  GHW_INVERSE flag corrected for grey-level symbols

   Version number: 2.11
   Copyright (c) RAMTEX Engineering Aps 1998-2009
*/
#include <gi_disp.h> /* gLCD prototypes */

#ifdef GSOFT_SYMBOLS

#ifdef GVIRTUAL_FONTS
#include <gvfont.h>
#endif

void gi_putsymbol(GXT xs,GYT ys,GXT xemax,GYT yemax,
      PGSYMBOL psymbol, GYT yoffset, GBUFINT symsize)
   {
   SGUINT xe;
   SGUINT ye;
   SGUINT bw;
   SGUCHAR mode; /* hw mode flags */
   PGSYMBYTE f;

   xe = (SGUINT) gsymw(psymbol);
   ye = (SGUINT) gsymh(psymbol);
   if ((ye == 0) || (xe == 0))
      {
      /* Error: illegal symbol header data. Corrupted data or a ROM/RAM pointer problem detected */
      G_WARNING( "Illegal symbol header data detected. Zero sized symbol" );
      #ifdef GVIRTUAL_FONTS
      if (gissymbolv(psymbol) && (xe == 0))
         {
         G_WARNING("Check if virtual font has been opened correctly");
         }
      #endif
      return;
      }
   if (giscolor(psymbol))
      {
      SGBOOL greymode;
      #ifdef GVIRTUAL_FONTS
      if (gissymbolv(psymbol))
         {
         #ifdef GDATACHECK
         /* Check for mixed font design error */
         if (((((PGSYMHEADV)psymbol)->type_id) & GVTYPEMASK) != 1)
            { /* Error: format is unknown to this library */
            G_WARNING( "Unsupported extended font format. Illegal virtual font" );
            return;
            }
         #endif
         f = (PGSYMBYTE) NULL; /* Signal use of virtual font to low-level driver*/
         }
      else
      #endif
         f = (PGSYMBYTE)&(((PGCSYMBOL)psymbol)->b[0]);  /* first byte in color symbol */
      mode = (SGUCHAR) gcolorbits(psymbol);
      greymode = ((mode & 0xc0) == 0x40) ? 1 : 0;
      mode &= 0x3f;
      if (symsize == 0)
         {
         if (mode <= 8)
            bw = (xe*((SGUINT)  mode)+7)/8;    /* width of symbol in bytes */
         else
            bw =  xe*((((SGUINT)mode)+7)/8);   /* width of symbol in bytes */
         }
      else
         bw = (SGUINT)(symsize/ye);
      if (mode >= 32) mode = 0x1f;
      if ((greymode != 0) && (mode <= 8))
         {
         mode |= GHW_GREYMODE;
         if (G_IS_INVERSE())
            mode |= GHW_INVERSE;
         }
      }
   else
      {
      #ifdef GVIRTUAL_FONTS
      if (gissymbolv(psymbol))
         {
         #ifdef GDATACHECK
         /* Check for mixed font design error */
         if (((((PGSYMHEADV)psymbol)->type_id) & GVTYPEMASK) != 1)
            { /* Error: format is unknown to this library */
            G_WARNING( "Unsupported extended font format. Illegal virtual font" );
            return;
            }
         #endif
         f = (PGSYMBYTE) NULL; /* Signal use of virtual font to low-level driver*/
         }
      else
      #endif
         f = (PGSYMBYTE)&(((PGBWSYMBOL)psymbol)->b[0]); /* first byte in B&W symbol */
      mode = G_IS_INVERSE() ? GHW_INVERSE : 0;
      if (symsize == 0)
         bw = (SGUINT) (xe+7)/8;  /* width of symbol in bytes */
      else
         bw = (SGUINT)(symsize/ye);
      }

    if (G_IS_TRANSPERANT())
       mode |= GHW_TRANSPERANT;

   if (yoffset != 0)
      {                          /* Only showing lower part of symbol */
      ys += yoffset;
      ye -= yoffset;
      #ifdef GVIRTUAL_FONTS
      if (!gissymbolv(psymbol))
      #endif
         f=&f[bw*yoffset];
      }

   xe = xe+((SGUINT) xs)-1;
   ye = ye+((SGUINT) ys)-1;

   /* truncate at max rect */
   if( xe > ((SGUINT) xemax) )
      xe = ((SGUINT) xemax);
   if( ye > ((SGUINT) yemax) )
      ye = ((SGUINT) yemax);

   glcd_err = 0; /* Reset HW error flag */

   #ifdef GVIRTUAL_FONTS
   if (gissymbolv(psymbol))
      gi_symv_open( psymbol, bw, yoffset ); /* Preset virtual symbol interface */
   #endif
   ghw_wrsym(xs, ys, (GXT) xe, (GYT) ye, f, bw, mode );
   }


#ifdef GVIRTUAL_FONTS

GXT gsymw(PGSYMBOL psymbol)
   {
   if (gissymbolv(psymbol))
      return psymbol->vsh.psymh_v->cxpix;
   else
      {
      if (psymbol->csh.colorid == 0) /* Is color ? */
         return ((PGCSYMBOL) psymbol)->sh.cxpix;
      else
         return ((PGBWSYMBOL)psymbol)->sh.cxpix;
      }
   }

GYT gsymh(PGSYMBOL psymbol)
   {
   if (gissymbolv(psymbol))
      return psymbol->vsh.psymh_v->cypix;
   else
      {
      if (psymbol->csh.colorid == 0) /* Is color ? */
         return ((PGCSYMBOL) psymbol)->sh.cypix;
      else
         return ((PGBWSYMBOL)psymbol)->sh.cypix;
      }
   }

/* Public information functions, not used by library, may be deleted */

/*
  Check if symbol is a color symbol
*/
SGBOOL giscolor(PGSYMBOL psymbol)
   {
   if (gissymbolv(psymbol))
      return (psymbol->vsh.psymh_v->numbits != 0) ? 1 : 0;
   return (psymbol->csh.colorid == 0) ? 1 : 0;
   }

/*
   Check bits pr pixel used by symbol,
   returns (1,2,4,8,16 or 24)
*/
SGUCHAR gcolorbits(PGSYMBOL psymbol)
   {
   if (gissymbolv(psymbol))
      {
      if (psymbol->vsh.psymh_v->numbits != 0)
         return (SGUCHAR)(psymbol->vsh.psymh_v->numbits);  /* Color symbol number of bits */
      }
   else
   if (psymbol->csh.colorid == 0)
      return (SGUCHAR) (psymbol->csh.pbits); /* Color symbol number of bits */
   return 1;  /* B&W symbol */
   }

#endif /* GVIRTUAL_FONTS */

#endif /* GSOFT_SYMBOLS */


