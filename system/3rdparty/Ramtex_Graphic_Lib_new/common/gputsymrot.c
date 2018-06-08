/*************************  gsymrotate.c  **************************

   Draw a symbol rotated optionally using a rotation ancher different
   from symbol center

   void gsymrotate( GXT x, GYT y,
                     float angle,
                     PGSYMBOL sym,
                     SGINT xancher, SGINT yancher,
                     GMODE symflag)

   This function rotates a bitmap to any angle.
      sym            Symbol to write in an rotated fashion
      angle          Angle to rotate symbol in radians.
      x,y            The viewport destination point for the symbol ancker
                     (0,0 is upper left corner of viewport)
      xancher, yancher The ancher point for symbol rotation relative to symbol
                     connection point. (0,0) => rotation around connection point.
                     The symbol connection point is defined by the flags
      symflags       Define symbol output mode flags

        GALIGN_LEFT    Symbol connection point is aligned to the left edge
        GALIGN_RIGHT   Symbol connection point is aligned to the right edge
        GALIGN_HCENTER Symbol connection point is centered horizontally
        GALIGN_TOP     Symbol connection point is aligned to the top
        GALIGN_BOTTOM  Symbol connection point is aligned to the bottom
        GALIGN_VCENTER Symbol connection point is centered vertically
        GTRANSPERANT   Enable transperant mode for output
        GSYMCLR        Set exact area covered by symbol to viewport background
                       or foreground color depending on GINVERSE
                       (i.e. do an exact clear of a rotated symbol drawn earlier)
        GINVERSE       Enable inverse color fill or inverse transperance

    Nine possible symbol connection alignment points
     ___
    *   |    GALIGN_TOP | GALIGN_LEFT
    |   |
    |___|
     ___
    | * |    GALIGN_TOP | GALIGN_HCENTER
    |   |    or just GALIGN_TOP
    |___|
     ___
    |   *    GALIGN_TOP | GALIGN_RIGHT
    |   |
    |___|

     ___
    |   |
    *   |    GALIGN_VCENTER | GALIGN_LEFT
    |___|    or just GALIGN_LEFT
     ___
    |   |
    | * |    GALIGN_VCENTER | GALIGN_HCENTER
    |___|    (or just 0 to use default)
     ___
    |   |
    |   *    GALIGN_VCENTER | GALIGN_RIGHT
    |___|    or just GALIGN_RIGHT

     ___
    |   |
    |   |
    *___|    GALIGN_BOTTOM | GALIGN_LEFT
     ___
    |   |
    |   |    GALIGN_BOTTOM | GALIGN_HCENTER
    |_*_|    or just GALIGN_BOTTOM
     ___
    |   |
    |   |
    |___*    GALIGN_BOTTOM | GALIGN_RIGHT


   Creation date:     10-02-2009

   Revision date:     16-04-2009
   Revision Purpose:  Improved grey-level support for grey-shade displays
                      Color blending algoritm improved to minimize color
                      noise caused by integer rounding.

   Revision date:     29-05-2009
   Revision Purpose:  Casts inserted
   Revision date:     03-02-2012
   Revision Purpose:  1 bit pr pixel mode supported for 0,90,180,270 degree
   Revision date:     02-12-2015
   Revision Purpose:  Complete internal redesign.
                      Internal use of fixed point character x,y positions to get more smooth
                      string rotation.
   Revision date:     10-3-2016
   Revision Purpose:  Fixed transperant mode alpha blending error with RGBA and
                      grey-level symbols.

   Version number: 1.5
   Copyright (c) RAMTEX International Aps 2009-2016
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/
#include <math.h>  /* Sinus and cosinus functions */
#include <gi_disp.h>
#include <gfixedpt.h>

#ifdef GSOFT_SYMBOLS
SGFIXP gi_fp_sin;  /* sin values as a fractional (16:16) number 65536==1.0 */
SGFIXP gi_fp_cos;  /* cosine values as a fractional (16:16) number 65536==1.0 */
static SGFIXP sx_max;
static SGFIXP sy_max;
static SGFIXP w;
static SGFIXP h;
static SGBOOL fill, nocolorblend;
static GCOLOR pixcolor;
#if (!defined( GHW_NO_LCD_READ_SUPPORT ) || defined(GBUFFER))
static SGBOOL trans_background;
#endif

/*
   Prepare sin and cos values and convert to fix point numbers
   for faster processing, i.e. for the rest of the rotated symbol processing
   avoid floating point calculations and use fast integer calculations instead
*/
void gi_setfpsincos( float angle )
   {
   gi_fp_sin = SGFLOAT_TO_FIXP(sin(angle));
   gi_fp_cos = SGFLOAT_TO_FIXP(cos(angle));

   /* Snap to axis when angle is close to ortogonal (give higher processing speed) */
   if (SGFIXP_ABS(gi_fp_sin) <= SGFIXP_SCALE/GDISPW)
      {
      gi_fp_sin = 0;
      gi_fp_cos = SGI_TO_FIXP((gi_fp_cos > 0) ? 1 : -1);
      }
   else
   if (SGFIXP_ABS(gi_fp_cos) <= SGFIXP_SCALE/GDISPH)
      {
      gi_fp_cos = 0;
      gi_fp_sin = SGI_TO_FIXP((gi_fp_sin > 0) ? 1 : -1);
      }
   }

/*
   Get represemtative pixel value for the fixed point coordinate.

   If not aligned to symbol coordinate raster then scale and blend pixels
   in a 2 x 2 raster to get a reprensative coordinate.

   If some part of a raster pixel is outside the symbol then the alpha value
   is scaled down correspondingly, and the remaining pixel parts are blended.
*/
/* Macros which control pixel blending resolution */
#if (GDISPPIXW >= 4)
  #define ALPHAMSK 0xf                 /* 0xf = 16 intensity levels */
  #define ALPHASHIFT 4                 /* 4 bits */
#elif (GDISPPIXW == 2)
  #define ALPHAMSK 0x3                 /* 0x3 = 4 intensity levels */
  #define ALPHASHIFT 2                 /* 2 bits */
#else
  #define ALPHAMSK 0x1                 /* 0x1 = 2 intensity levels */
  #define ALPHASHIFT 1                 /* 1 bit */
#endif
#define ALPHAMAX (ALPHAMSK*ALPHAMSK)
#define SUBC(fp) (((fp)>>(SGFIXP_BITS-ALPHASHIFT))&ALPHAMSK) /* Extract sub pixel weight */


#ifdef GHW_USING_RGB
 /* Macros for extracting and setting LSB aligned of color lane values
   (LSB alignment prevent variable saturation during alpha scaling
    and sub color blending calculations, without the need to use C double type */
 #if (GDISPPIXW >= 24)
  #define G_GET_RED_LANE( c )   (((c) >> 16) & 0xff)
  #define G_GET_GREEN_LANE( c ) (((c) >> 8) & 0xff)
  #define G_GET_BLUE_LANE( c )  ((c) & 0xff)
  #define G_SET_LANE_RED( c )   ((c) << 16)
  #define G_SET_LANE_GREEN( c ) ((c) << 8)
  #define G_SET_LANE_BLUE( c )  (c)
 #elif (GDISPPIXW == 18)
  #define G_GET_RED_LANE( c )   (((c) >> 12) & 0x3f)
  #define G_GET_GREEN_LANE( c ) (((c) >> 6) & 0x3f)
  #define G_GET_BLUE_LANE( c )  ((c) & 0x3f)
  #define G_SET_LANE_RED( c )   ((c) << 12)
  #define G_SET_LANE_GREEN( c ) ((c) << 6)
  #define G_SET_LANE_BLUE( c )  (c)
 #elif (GDISPPIXW == 16)
  #define G_GET_RED_LANE( c )   (((c) >> 11) & 0x1f)
  #define G_GET_GREEN_LANE( c ) (((c) >> 5) & 0x3f)
  #define G_GET_BLUE_LANE( c )  ((c) & 0x1f)
  #define G_SET_LANE_RED( c )   ((c) << 11)
  #define G_SET_LANE_GREEN( c ) ((c) << 5)
  #define G_SET_LANE_BLUE( c )  (c)
 #elif (GDISPPIXW == 8)
  #define G_GET_RED_LANE( c )   (((c) >> 5) & 0x7)
  #define G_GET_GREEN_LANE( c ) (((c) >> 2) & 0x7)
  #define G_GET_BLUE_LANE( c )  ((c) & 0x3)
  #define G_SET_LANE_RED( c )   ((c) << 5)
  #define G_SET_LANE_GREEN( c ) ((c) << 2)
  #define G_SET_LANE_BLUE( c )  (c)
 #endif
#endif /* GHW_USING_RGB */

static void process_pixel(SGFIXP xo, SGFIXP yo, GXT xp, GYT yp)
   {
   SGFIXP xot;
   SGUCHAR x,y;
   SGUINT blendsum;
   SGULONG a,g;  /* Use SGULONG here to be able to hold worst case intermediate color lane value */
   #ifdef GHW_USING_RGB
   SGULONG r,b;
   #endif
   if (fill)
      goto pixwrite;  /* pixcolor value is already preset */
   else
   if (nocolorblend)
      { /* Quick draw (Skip 2x2 sub-pixel blend) */
      register GACOLOR ct;
      if ((xo < 0) || (yo < 0))
         return;
      ct = gi_getsym_pixel(SGFIXP_TO_U(xo), SGFIXP_TO_U(yo));
      /* Process symbol alpha info */
      a = G_ICOLOR_ALPHA(ct);

      if (a == GALPHA_FULLTRANS)
         return; /* Full transperant, skip pixel completely */

      if (a == GALPHA_OPAQUE)
         { /* Opaque, skip background blend */
         pixcolor = (GCOLOR) GI_CMSK(ct);
         goto pixwrite;
         }

      /* Semi-transperant or grey-level symbol. Prepare for background blending */
      #ifdef GHW_USING_RGB
      r = (SGULONG) (ct & G_RED_MSK);
      g = (SGULONG) (ct & G_GREEN_MSK);
      b = (SGULONG) (ct & G_BLUE_MSK);
      #else
      g = GI_CMSK(ct);
      #endif
      goto background_blend;
      }
   else
      {
      /* Create symbol pixel color by weighted blending of 1-4 symbol pixels */
      /* (sub-pixel alignment) */
      #ifdef GHW_USING_RGB
      r=g=b=0;
      #else
      g=0;
      #endif
      for (y=0,a=0,blendsum=0;(y<=1)&&(yo < h);y++,yo+=SGI_TO_FIXP(1))
         {
         SGUCHAR ay;
         if (yo < 0)
            continue;  /* Quick test. Completely ourside symbol */
         ay = (SGUCHAR)(SUBC(yo));
         if (y == 0)
            ay = ALPHAMSK-ay;
         if (ay == 0)
            continue;  /* Sub-pixel not active, skip it */
         for (xot=xo, x=0; (x<=1)&&(xot<w);x++,xot+=SGI_TO_FIXP(1))
            {
            register SGUINT alpha;
            if (xot < 0)
               continue; /* Quick test. Completely ourside symbol */
            /* Get scaling fraction part and find blending weigth for pixel color */
            alpha = (SGUINT)(SUBC(xot));
            if (x == 0)
               alpha = (ALPHAMSK-alpha);
            if (alpha != 0)
               { /* Sub pixel is active */
               register GACOLOR ct;
               register SGUINT at;
               ct = gi_getsym_pixel(SGFIXP_TO_U(xot), SGFIXP_TO_U(yo));
               if ((at = (SGUINT)G_ICOLOR_ALPHA(ct)) != GALPHA_FULLTRANS)
                  { /* Sub pixel is not full transperant */
                  /* Combine subpixel weight, and symbol pixel alpha variations, into one color scaling value */
                  alpha *= at;
                  alpha *= ay;
                  /* Create sum of subpixel contributions */
                  blendsum += alpha;
                  a += (SGULONG) alpha;
                  #ifdef GHW_USING_RGB
                  r+= ((SGULONG) G_GET_RED_LANE(ct))  * alpha;
                  g+= ((SGULONG) G_GET_GREEN_LANE(ct))* alpha;
                  b+= ((SGULONG) G_GET_BLUE_LANE(ct)) * alpha;
                  #else
                  g += (SGULONG)GI_CMSK(ct) * alpha;
                  #endif
                  }
               }
            }
         }

      /* Sub-pixel blending done */
      if (a == GALPHA_FULLTRANS)
         return; /* Full transperant, skip pixel */
      }

   /* Normalize scaled color lane values to match normal color lane
      bit size as defined by GDISPPIXW */
//   a /= GALPHAMSK;
   a /= ALPHAMAX;
   #ifdef GHW_USING_RGB
   r = G_SET_LANE_RED(   r / blendsum );
   g = G_SET_LANE_GREEN( g / blendsum );
   b = G_SET_LANE_BLUE(  b / blendsum );
   #else
   g /= blendsum;
   #endif
   if (a != GALPHA_OPAQUE)
      { /* Pixel not full opaque. Blend with background color */
      GCOLOR bg;
      background_blend:

      /*  Real color is a scaling between foreground and background color */
      #if (!defined( GHW_NO_LCD_READ_SUPPORT ) || defined(GBUFFER))
      if (trans_background)
         bg = ghw_getpixel(xp, yp); /* Use screen background color */
      else
      #endif
         bg = gcurvp->background;   /* Use viewport background color */
      #ifdef GHW_USING_RGB
      r = ((r*a + ((SGULONG)(bg & G_RED_MSK  ))*(GALPHAMSK-a))/GALPHAMSK) & G_RED_MSK;
      g = ((g*a + ((SGULONG)(bg & G_GREEN_MSK))*(GALPHAMSK-a))/GALPHAMSK) & G_GREEN_MSK;
      b = ((b*a + ((SGULONG)(bg & G_BLUE_MSK ))*(GALPHAMSK-a))/GALPHAMSK) & G_BLUE_MSK;
      #else
      g = (g*a + ((GACOLOR)bg)*(GALPHAMSK-a))/GALPHAMSK;
      #endif
      }
   #ifdef GHW_USING_RGB
   pixcolor = (GCOLOR) (r|g|b);
   #else
   pixcolor = (GCOLOR) GI_CMSK(g);
   #endif

   pixwrite:
   ghw_setpixel( xp, yp, pixcolor);
   }

/*
   Fill line in destination rectangle with data from source symbol.
   The routine make a simplified flood fill to copy every pixel in the
   destination line from the source symbol. There are no guarantees that
   every pixel from the source is used, but every pixel in the destination
   is updated.
   cpypixline tries to go as far left and right in the destination as the source
   rectangle permits. While doing this, it copies the pixels from the source
   to the destination.
*/
static void cpypixline(SGFIXP xop, SGFIXP yop, SGINT xp, SGINT yp)
   {
   SGFIXP xo,yo;
   SGINT x;
   xo = xop;
   yo = yop;

   /* Copy to right */
   x = xp;
   for(;;)
      {
      if (x > _GTOI(gcurvp->rb.x))
         break;
      if (x >= gcurvp->lt.x)
         {
         if ((xo <= SGI_TO_FIXP(-1)) || (xo > sx_max) ||
             (yo <= SGI_TO_FIXP(-1)) || (yo > sy_max))
            break;
         process_pixel(xo,yo, _GTOX(x), _GTOY(yp));
         }
      x++;                  /* Move destination right */
      xo += gi_fp_cos;  /* Same move within source */
      yo += gi_fp_sin;
      }

   /* Copy to left */
   x = xp-1;  /* Skip pixel already drawn */
   xo = xop - gi_fp_cos;
   yo = yop - gi_fp_sin;

   for(;;)
      {
      if (x < gcurvp->lt.x)
         break;
      if (x <= gcurvp->rb.x )
         {
         if ((xo <= SGI_TO_FIXP(-1)) || (xo > sx_max) ||
             (yo <= SGI_TO_FIXP(-1)) || (yo > sy_max))
               break;
         process_pixel(xo,yo, _GTOX(x), _GTOY(yp));
         }
      x--;              /* Move destination left */
      xo -= gi_fp_cos;  /* Same move within source */
      yo -= gi_fp_sin;
      }
   }

/* Macros for packing / unpacking drawing endpoint info for connection points
   (packs info in two signed integers into one unsigned char) */
#define PK(x,y) (((((SGUCHAR)((x)+2))<<4)&0x70) | ((((SGUCHAR)((y)+2)))&0x7)) /* Compile time packing */
#define UPKX(xys) (((SGINT)(((xys)>>4)&0x7))-2) /* unpack x info */
#define UPKY(xys) (((SGINT)((xys)&0x7))-2)      /* unpack y info */

/* The table describes relative distance from a connection point to the symbol corner
   which will be the highest and lowest point respectively for a rotation within
   the specific coordinate system quadrant. Simplifies drawing calculations below */
static GCODE SGUCHAR FCODE fpos[9*4*2] =
   {
   PK( 2, 0),PK( 0, 2), PK( 2, 2),PK( 0, 0), PK( 0, 2),PK( 2, 0), PK( 0, 0),PK( 2, 2),  /*  left, top       */
   PK( 1, 0),PK(-1, 2), PK( 1, 2),PK(-1, 0), PK(-1, 2),PK( 1, 0), PK(-1, 0),PK( 1, 2),  /*  hcenter, top    */
   PK( 0, 0),PK(-2, 2), PK( 0, 2),PK(-2, 0), PK(-2, 2),PK( 0, 0), PK(-2, 0),PK( 0, 2),  /*  right, top      */
   PK( 2,-1),PK( 0, 1), PK( 2, 1),PK( 0,-1), PK( 0, 1),PK( 2,-1), PK( 0,-1),PK( 2, 1),  /*  left, vcenter   */
   PK( 1,-1),PK(-1, 1), PK( 1, 1),PK(-1,-1), PK(-1, 1),PK( 1,-1), PK(-1,-1),PK( 1, 1),  /*  hcenter, vcenter*/
   PK( 0,-1),PK(-2, 1), PK( 0, 1),PK(-2,-1), PK(-2, 1),PK( 0,-1), PK(-2,-1),PK( 0, 1),  /*  right, vcenter  */
   PK( 2,-2),PK( 0, 0), PK( 2, 0),PK( 0,-2), PK( 0, 0),PK( 2,-2), PK( 0,-2),PK( 2, 0),  /*  left, bottom    */
   PK( 1,-2),PK(-1, 0), PK( 1, 0),PK(-1,-2), PK(-1, 0),PK( 1,-2), PK(-1,-2),PK( 1, 0),  /*  hcenter, bottom */
   PK( 0,-2),PK(-2, 0), PK( 0, 0),PK(-2,-2), PK(-2, 0),PK( 0,-2), PK(-2,-2),PK( 0, 0)   /*  right, bottom   */
   };

/*
   Output gcurlay symbol rotated,
      Process anchor point offsets, anchor symbol connection point offset, a outmode settings

   gi_fp_sin, gi_fp_cos must have been preset before calling this function.
*/
void gi_symrotate( SGFIXP x, SGFIXP y,  SGFIXP axo, SGFIXP ayo,
                   GMODE  symflag,  SGUINT symw, SGUINT symh)
   {
   SGINT py,px,yend;
   SGFIXP xo, yo, xstep, dx;
   SGUCHAR connectpt,updown;
   #ifdef GBUFFER
   GUPDATE update;

   /* Use completion of drawing before flush to screen */
   update = gsetupdate(GUPDATE_OFF);
   #endif

   /* Set pixel processing mode flags */
   #if (!defined( GHW_NO_LCD_READ_SUPPORT ) || defined(GBUFFER))
   trans_background = (symflag & GTRANSPERANT) ? 1 : 0;
   #endif
   if ((fill = (symflag & GSYMCLR) ? 1 : 0) != 0)
     {
     /* Preset color once, used for whole drawing */
     pixcolor = (symflag & GINVERSE) ? gcurvp->foreground : gcurvp->background;
     }

   if ((axo != 0) || (ayo != 0))
      {
      /* Calculate connection point position change caused by rotation around anchor */
      x += (gi_fp_cos*axo + gi_fp_sin*ayo)/SGFIXP_SCALE;
      y += (gi_fp_cos*ayo - gi_fp_sin*axo)/SGFIXP_SCALE;
      }

   /* Skip color blending if requested, OR if symbol parameters are already
      completely aligned with both coordinate axis and integer coordinate values */
   nocolorblend =
        ((symflag & GNOCOLORBLEND) ||
         (((gi_fp_cos == 0) || (gi_fp_sin == 0)) && (SUBC(x) == 0) && (SUBC(y) == 0)))
          ? 1 : 0; /* Use quick processing */

   /* Max symbol coordinates */
   w = SGU_TO_FIXP(symw-1);
   h = SGU_TO_FIXP(symh-1);

   /* Set connection point position in symbol +
      index for lookup of farest corner position */
   switch (symflag & GALIGN_VCENTER)
      {
      case GALIGN_BOTTOM:
         connectpt = 6*4*2;
         ayo = h;
         break;
      case GALIGN_TOP:
         connectpt = 0;
         ayo = 0;
         break;
      default: /* GALIGN_VCENTER (or default) */
         connectpt = 3*4*2;
         ayo = h>>1;
         break;
      }

   switch (symflag & GALIGN_HCENTER)
      {
      case GALIGN_RIGHT:
         axo = w;
         connectpt += 2*4*2;
         break;
      case GALIGN_LEFT: /* Specifed anker point is relative to left edge*/
         axo = 0;
         break;
      default: /* GALIGN_HCENTER (or default) */
         connectpt += 1*4*2;
         axo = w>>1;
         break;
      }

   /* Symbol width, height as fixed point */
   w+=SGU_TO_FIXP(1);
   h+=SGU_TO_FIXP(1);

   /* Max symbol rotate processing coordinates, incl max fraction to assure edge blending */
   sx_max = w+(SGU_TO_FIXP(1)-1);
   sy_max = h+(SGU_TO_FIXP(1)-1);

   /* Align x pixel position to integer coordinates */
   px = SGFIXP_TO_I(x);
   xo = x - SGI_TO_FIXP(px);
   if (xo != 0)
      { /* Compensate change in symbol coordinates */
      axo -= (gi_fp_cos*xo)/SGFIXP_SCALE;
      ayo -= (gi_fp_sin*xo)/SGFIXP_SCALE;
      }
   /* Align y pixel position to integer coordinates */
   py = SGFIXP_TO_I(y);
   yo = y - SGI_TO_FIXP(py);
   if (yo != 0)
      { /* Compensate change in symbol coordinates */
      y+= SGI_TO_FIXP(1);
      yo = SGI_TO_FIXP(1) - yo;
      axo -= (gi_fp_sin*yo)/SGFIXP_SCALE;
      ayo += (gi_fp_cos*yo)/SGFIXP_SCALE;
      }

   /* Add kvadrant offset */
   connectpt += (gi_fp_sin >= 0) ? ((gi_fp_cos >= 0) ? 0:2) : ((gi_fp_cos >= 0) ? 6:4);

   /* Do drawing in two passes, from connection point and up, and from connection point down, */
   for (updown = 0; updown < 3; updown++)
      {
      /* Calculate x movement step of line rendering start (goal is to stay inside symbol area all the time) */
      xo = (SGI_TO_FIXP(UPKX(fpos[connectpt]))  * w)/SGI_TO_FIXP(2);
      yo = (SGI_TO_FIXP(UPKY(fpos[connectpt++]))* h)/SGI_TO_FIXP(2);
      dx    = (gi_fp_cos*yo - gi_fp_sin*xo)/SGFIXP_SCALE; /* Temp offset to farest y position after rotation*/
      xstep = (gi_fp_cos*xo + gi_fp_sin*yo)/(SGFIXP_SCALE/2); /* Temp offset to farest x position after rotation*/
      if (SGFIXP_ABS(dx) > SGI_TO_FIXP(1))
         xstep = SGFIXP_DIV(xstep, SGFIXP_ABS(dx));       /* More than one x adjustments needed */

      /* Set end of y movement loop */
      yend = SGFIXP_TO_I(dx+y);

      px = SGFIXP_TO_I(x);
      py = SGFIXP_TO_I(y);
      xo = axo;
      yo = ayo;
      dx = 0;
      for(;;)
         {
         register SGINT delta;

         /* Check yo */
         if (yo <= SGI_TO_FIXP(-1))
            delta = ( gi_fp_sin > 0 ) ? 1 : (( gi_fp_sin < 0 ) ? -1 : 0);
         else
         if (yo >= sy_max)
            delta = ( gi_fp_sin < 0 ) ? 1 : (( gi_fp_sin < 0 ) ? -1 : 0);
         else
            delta = 0;

         /* Check xo */
         if (xo <= SGI_TO_FIXP(-1))
            delta += ( gi_fp_cos > 0 ) ? 1 : (( gi_fp_cos < 0 ) ? -1 : 0);
         else
         if (xo >= sx_max)
            delta += ( gi_fp_cos < 0 ) ? 1 : (( gi_fp_cos > 0 ) ? -1 : 0);

         /* Adjust start x towards line center */
         if ( delta!=0 )
            {
            xo += gi_fp_cos*delta;
            yo += gi_fp_sin*delta;
            px += delta;
            }

         if (updown == 0)
            {  /* Draw lines at connection point and up */
            if (py < gcurvp->lt.y)
               break; /* Moved outside viewport */
            if (py <= gcurvp->rb.y)
               {
               cpypixline(xo,yo,px,py);  /* Symbol (part) inside viewport */
               }
            if ( --py < yend) /* Next line */
               break;
            xo += gi_fp_sin; /* Same move in source bitmap coordinates */
            yo -= gi_fp_cos;
            }
         else
            {  /* Draw lines at connection point and down */
            if (py > gcurvp->rb.y)
               break; /* Moved outside viewport */
            if (updown == 1)
               updown++; /* skip center line which is already drawn, just move position */
            else
               {
               if (py >= gcurvp->lt.y)
                  {
                  cpypixline(xo,yo,px,py);  /* Symbol (part) inside viewport */
                  }
               }
            if ( ++py > yend)  /* Next y line */
               break;          /* Last y line reached */
            xo -= gi_fp_sin;   /* Same move in source bitmap coordinates */
            yo += gi_fp_cos;
            }

         /* Adjust line rendering x start position (to stay within symbol) */
         dx += xstep;
         if ((delta = SGFIXP_TO_I(dx)/2)!=0)
            { /* Adjust x position one or more pixels */
            dx-=SGI_TO_FIXP(delta*2);
            xo = xo + delta*gi_fp_cos; /* Same move in source bitmap coordinates */
            yo += delta*gi_fp_sin;
            px+=delta;
            }
         }
      }
   #ifdef GBUFFER
   /* Flush changes to screen */
   gsetupdate(update);
   #endif
   }

#if defined( GBASIC_TEXT ) && defined( GSOFT_FONTS )

/*
   Internal function for write of character symbol to position using a
   rotated angle
   The anker position is the lower left corner of the character symbol.
   The current font and viewport is used,
   The layer has been preset for font type

   Returns width of symbol
*/
SGFIXP gi_putchw_rotate( GWCHAR ch, SGFIXP x, SGFIXP y )
   {
   PGSYMBOL psymbol;
   GMODE mode;
   /* Get symbol for character */
   psymbol = gi_getsymbol( ch, gcurvp->pfont, gcurvp->codepagep);
   if (psymbol != NULL)
      {
      /* Prepare symbol for pixel color fetch */
      mode = (GMODE) ((((unsigned int) gcurvp->mode) & (GSYMCLR|GINVERSE|GTRANSPERANT))|(GALIGN_BOTTOM|GALIGN_LEFT));
      if (gi_getsym_open( psymbol, (SGUINT)(gi_fsymsize(gcurvp->pfont)/gcurvp->fsize.y), mode)==0)
         return 0;

      /* Output symbol using rotated mode, with anker in lower left symbol corner */
      gi_symrotate(x, y, 0,0, mode, gsymw(psymbol), gsymh(psymbol));

      /* return width */
      #ifdef GNOTXTSPACE
      return SGU_TO_FIXP(gsymw(psymbol));
      #else
      return SGU_TO_FIXP(gsymw(psymbol) + gcurvp->chln.x);
      #endif
      }
   return (SGFIXP) 0;
   }
#endif

void gputsymrot( SGINT x, SGINT y, float angle, PGSYMBOL psymbol,
                 SGINT xanchor, SGINT yanchor, GMODE symflag)
   {
   /* Prepare symbol for pixel color fetch */
   if (gi_getsym_open( psymbol, 0, symflag)==0)
      return;

   gi_setfpsincos( angle ); /* Prepare for fixed point calculations */
   gi_symrotate(SGI_TO_FIXP(x+_GTOI(gcurvp->lt.x)),
                SGI_TO_FIXP(y+_GTOI(gcurvp->lt.y)),
                SGU_TO_FIXP(xanchor),SGU_TO_FIXP(yanchor),
                symflag, gsymw(psymbol), gsymh(psymbol));
   ghw_updatehw();
   }

#ifdef GFUNC_VP

void gputsymrot_vp( SGUCHAR vp, SGINT x, SGINT y, float angle, PGSYMBOL psymbol,
                    SGINT xanchor, SGINT yanchor, GMODE symflag)
   {
   GSETFUNCVP(vp, gputsymrot(x,y,angle, psymbol, xanchor,yanchor, symflag) );
   }

#endif

#ifdef GSOFT_FONTS
void gputfsymrot( SGINT x, SGINT y, float angle, SGUINT index, PGFONT pfont,
                 SGINT xanchor, SGINT yanchor, GMODE symflag)
   {
   PGSYMBOL psymbol;
   /* Prepare pointer to symbol data for index, use code page lookup if the font contains a codepage */
   psymbol = gi_getsymbol((GWCHAR)index,pfont,gi_fpcodepage(pfont));
   if (psymbol == NULL)
      return; /* Font symbol lookup failed */

   /* Prepare symbol for pixel color fetch */
   if (gi_getsym_open( psymbol, (SGUINT)(gi_fsymsize(pfont)/gi_fsymh(pfont)), symflag)==0)
      return; /* Format error */

   gi_setfpsincos( angle ); /* Prepare for fixed point calculations */
   gi_symrotate(SGI_TO_FIXP(x+_GTOI(gcurvp->lt.x)),
                SGI_TO_FIXP(y+_GTOI(gcurvp->lt.y)),
                SGU_TO_FIXP(xanchor),SGU_TO_FIXP(yanchor),
                symflag, gsymw(psymbol), gsymh(psymbol));
   ghw_updatehw();
   }

#ifdef GFUNC_VP

void gputfsymrot_vp( SGUCHAR vp, SGINT x, SGINT y, float angle, SGUINT index, PGFONT pfont,
                    SGINT xanchor, SGINT yanchor, GMODE symflag)
   {
   GSETFUNCVP(vp, gputfsymrot(x,y,angle,index,pfont,xanchor,yanchor,symflag) );
   }

#endif

#endif /* GSOFT_FONTS */

#endif /* GSOFT_SYMBOLS */

