/*************************  gsymrotate.c  **************************

   Draw a symbol rotated optionally using a rotation ancker different
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

   Version number: 1.3
   Copyright (c) RAMTEX Engineering Aps 2009-2012

*********************************************************************/
#include <math.h>  /* Sinus and cosinus functions */
#include <gi_disp.h>

#ifdef GSOFT_SYMBOLS
SGLONG gi_fp_sin;  /* sin values as a fractional (16:16) number 65536==1.0 */
SGLONG gi_fp_cos;  /* cosine values as a fractional (16:16) number 65536==1.0 */
static SGLONG maxx, maxy;    /* Size of source bitmap as a fractional (16:16) number.   */
static SGINT w,h;
static SGBOOL transperant;
static SGBOOL fill;
#if (defined( GHW_USING_RGB ) || defined( GHW_USING_GREY_LEVEL))
static SGBOOL grey_mode;
static SGBOOL blending;
#endif
static GCOLOR colorback;

typedef struct    /* Structure to keep track of coordinates */
   {              /* in the source and destination rectangles */
   SGINT x;       /* Location on destination bitmap in integral coordinates */
   SGINT y;
   SGLONG xo;     /* Location on source bitmap in fractional coordinates      */
   SGLONG yo;
   } GSYMPLACE;

static void pos_cpy(GSYMPLACE *dest, GSYMPLACE *src)
   {
   dest->x = src->x;
   dest->y = src->y;
   dest->xo = src->xo;
   dest->yo = src->yo;
   }

/* Macros which control blending resolution */
#if (defined( GHW_USING_RGB ) || defined( GHW_USING_GREY_LEVEL))
 #if (GDISPPIXW >= 4)
  #define ALPHAMSK 0xf                 /* 0xf = 16 levels */
  /*#define ALPHAMSK  0x7*/            /* 0x7 = 8 levels */
 #elif (GDISPPIXW == 2)
  #define ALPHAMSK 0x3                 /* 0x3 = 4 levels */
 #else
  #define ALPHAMSK 0x1                 /* 0x1 = 2 levels */
 #endif
 #define ALPHAMAX (ALPHAMSK*ALPHAMSK)  /* 0xe1 = 225 */
 #define ALPHASCALE(calpha) (SGUINT)(calpha)
#endif

#ifdef GHW_USING_RGB
 #if (GDISPPIXW == 8)
   #define  GCOLORTMP SGUINT
 #else
   #define  GCOLORTMP SGULONG
 #endif
 #define  REDSCALE(c,  calpha) (((GCOLORTMP)((c) & G_RED_MSK  )*(calpha)))
 #define  GREENSCALE(c,calpha) (((GCOLORTMP)((c) & G_GREEN_MSK)*(calpha)))
 #define  BLUESCALE(c, calpha) (((GCOLORTMP)((c) & G_BLUE_MSK )*(calpha)))
#else
 #define GREYMAX ((1<<(GDISPPIXW))-1)
#endif /* GHW_USING_RGB */
#define  GREYSCALE(c, calpha) ((SGUINT)(c)*(calpha))


/*
   Process pixel.
   If blending == 0 then read 0 or 1 pixel from symbol (0 when upper
   or left edge) and write output
   If blending != 0 then do anliazing over 1-4 symbol pixel.
   At edges and when transperant then baground pixel are involved.

   The top row (y=0) and left column (x=0) are virtual and only used
   for blending.

   When grey-level symbols are used the color is treated as a logical value
   and define the blending between the forground color and either the background
   color or the screen background if transperance is used.
*/
static void process_pixel(GSYMPLACE *pos)
   {
   SGINT x,y;
   GCOLOR c;
   #ifdef GHW_USING_RGB
   GCOLORTMP r,g,b;
   #endif
   #if (defined( GHW_USING_RGB ) || defined( GHW_USING_GREY_LEVEL))
   SGUCHAR dx,dy,dx1,dy1;
   SGUCHAR calpha;
   GCOLOR bg;
   SGUINT grey = 0;
   SGUINT alpha = 0;
   #endif

   /* Check if output pixel is inside viewport */
   if (pos->x < 0)
      return;
   if (pos->y < 0)
      return;
   if (pos->x > (SGINT) (gcurvp->rb.x - gcurvp->lt.x))
      return;
   if (pos->y > (SGINT) (gcurvp->rb.y - gcurvp->lt.y))
      return;

   if (fill)
      {
      /* Process pixel fill */
      ghw_setpixel( (GXT) pos->x+gcurvp->lt.x, (GYT) pos->y+gcurvp->lt.y, colorback);
      return;
      }

   /* Process pixel color */
   x = (SGINT)((pos->xo) >> 16);
   y = (SGINT)((pos->yo) >> 16);

   #if (defined( GHW_USING_RGB ) || defined( GHW_USING_GREY_LEVEL))
   if (blending != 0)
      {
      #ifdef GHW_USING_RGB
      r=g=b=0;
      #endif

      #if (ALPHAMSK == 0xf)
      dx1 = (SGUCHAR)(pos->xo >> 12) & ALPHAMSK;
      dy1 = (SGUCHAR)(pos->yo >> 12) & ALPHAMSK;
      #elif (ALPHAMSK == 0x7)
      dx1 = (SGUCHAR)(pos->xo >> 13) & ALPHAMSK;
      dy1 = (SGUCHAR)(pos->yo >> 13) & ALPHAMSK;
      #elif (ALPHAMSK == 0x3)
      dx1 = (SGUCHAR)(pos->xo >> 14) & ALPHAMSK;
      dy1 = (SGUCHAR)(pos->yo >> 14) & ALPHAMSK;
      #else
      dx1 = (SGUCHAR)(pos->xo >> 15) & ALPHAMSK;
      dy1 = (SGUCHAR)(pos->yo >> 15) & ALPHAMSK;
      #endif
      if ((dx1 != 0) || (dy1 != 0))
         {
         /* Symbol pixel area crosses screen pixel area boundaries. Do pixel aliazing */
         dx = (ALPHAMSK-dx1);
         dy = (ALPHAMSK-dy1);
         /* x,y */
         if ((calpha = dx*dy) != 0)
            {
            if ((x==0)||(y==0))
               alpha = (SGUINT)(alpha + ALPHASCALE(calpha));
            else
               {
               c = gi_getsym_pixel(x-1,y-1);
               if (!grey_mode)
                  {
                  if (transperant && (c == ghw_def_background))
                     alpha = (SGUINT)(alpha+ALPHASCALE(calpha));
                  else
                     {
                     #ifdef GHW_USING_RGB
                     r += REDSCALE(  c,calpha);
                     g += GREENSCALE(c,calpha);
                     b += BLUESCALE( c,calpha);
                     #else
                     grey += GREYSCALE(c, calpha);
                     #endif
                     }
                  }
               else
                  grey += GREYSCALE(c, calpha);
               }
            }
         /* x+1,y */
         if ((calpha = (dx1*dy)) != 0)
            {
            if ((y==0) || ((x+1) >= w))
               alpha = (SGUINT)(alpha + ALPHASCALE(calpha));
            else
               {
               c = gi_getsym_pixel(x,y-1);
               if (!grey_mode)
                  {
                  if (transperant && (c == ghw_def_background))
                     alpha = (SGUINT)(alpha + ALPHASCALE(calpha));
                  else
                     {
                     #ifdef GHW_USING_RGB
                     r += REDSCALE(  c,calpha);
                     g += GREENSCALE(c,calpha);
                     b += BLUESCALE( c,calpha);
                     #else
                     grey += GREYSCALE(c, calpha);
                     #endif
                     }
                  }
               else
                  grey += GREYSCALE(c, calpha);
               }
            }
         /* x,y+1 */
         if ((calpha = dx*dy1) != 0)
            {
            if ((x==0) || ((y+1) >= h))
               alpha = (SGUINT)(alpha + ALPHASCALE(calpha));
            else
               {
               c = gi_getsym_pixel(x-1,y);
               if (!grey_mode)
                  {
                  if (transperant && (c == ghw_def_background))
                     alpha = (SGUINT)(alpha + ALPHASCALE(calpha));
                  else
                     {
                     #ifdef GHW_USING_RGB
                     r += REDSCALE(  c,calpha);
                     g += GREENSCALE(c,calpha);
                     b += BLUESCALE( c,calpha);
                     #else
                     grey += GREYSCALE(c, calpha);
                     #endif
                     }
                  }
               else
                  grey += GREYSCALE(c, calpha);
               }
            }
         /* x+1,y+1 */
         if ((calpha = dy1*dx1) != 0)
            {
            if (((x+1) >= w) || ((y+1) >= h))
               alpha = (SGUINT)(alpha+ALPHASCALE(calpha));
            else
               {
               c = gi_getsym_pixel(x,y);
               if (!grey_mode)
                  {
                  if (transperant && (c == ghw_def_background))
                     alpha = (SGUINT)(alpha+ALPHASCALE(calpha));
                  else
                     {
                     #ifdef GHW_USING_RGB
                     r += REDSCALE(  c,calpha);
                     g += GREENSCALE(c,calpha);
                     b += BLUESCALE( c,calpha);
                     #else
                     grey += GREYSCALE(c, calpha);
                     #endif
                     }
                  }
               else
                  grey += GREYSCALE(c, calpha);
               }
            }

         /* Blending caluclations completed (color values is multiplied by ALPHAMSK) */

         if (grey_mode)
            {
            grey_processing:
            /* The symbol "color" is an intensity level,
               Real color is a scaling between foreground and bacground color */
            #if (!defined( GHW_NO_LCD_READ_SUPPORT ) || defined(GBUFFER))
            if (transperant)
               bg = ghw_getpixel((GYT) pos->x+gcurvp->lt.x, (GYT) pos->y+gcurvp->lt.y);
            else
            #endif
               bg = ghw_def_background; /* Use viewport background color */

            #ifdef GHW_USING_RGB
            r = ((((GCOLORTMP)(ghw_def_foreground & G_RED_MSK))  *grey + ((GCOLORTMP)(bg & G_RED_MSK))  *((ALPHAMAX*0xff)-grey))/(ALPHAMAX*0xff));
            b = ((((GCOLORTMP)(ghw_def_foreground & G_BLUE_MSK)) *grey + ((GCOLORTMP)(bg & G_BLUE_MSK)) *((ALPHAMAX*0xff)-grey))/(ALPHAMAX*0xff));
            g = ((((GCOLORTMP)(ghw_def_foreground & G_GREEN_MSK))*grey + ((GCOLORTMP)(bg & G_GREEN_MSK))*((ALPHAMAX*0xff)-grey))/(ALPHAMAX*0xff));
            #else
            grey = (SGUINT)((((SGUINT)ghw_def_foreground)*grey + ((SGUINT)bg)*(ALPHAMAX*GREYMAX-grey))/(ALPHAMAX*GREYMAX));
            #endif
            /* Symbol grey level is converted to real color values */
            if (alpha != 0)
               {
               /* Do extra alpha blening of edge values */
               #ifdef GHW_USING_RGB
               r = (r*(ALPHAMAX-alpha) + ((GCOLORTMP)(bg & G_RED_MSK  ))*alpha)/ALPHAMAX;
               b = (b*(ALPHAMAX-alpha) + ((GCOLORTMP)(bg & G_BLUE_MSK ))*alpha)/ALPHAMAX;
               g = (g*(ALPHAMAX-alpha) + ((GCOLORTMP)(bg & G_GREEN_MSK))*alpha)/ALPHAMAX;
               #else
               grey = (grey*(ALPHAMAX-alpha) + (SGUINT)bg*alpha)/ALPHAMAX;
               #endif
               }
            }
         else
            {
            if (alpha != 0)
               {
               #if (!defined( GHW_NO_LCD_READ_SUPPORT ) || defined(GBUFFER))
               if (transperant)
                  bg = ghw_getpixel((GYT) pos->x+gcurvp->lt.x, (GYT) pos->y+gcurvp->lt.y);
               else
               #endif
                  bg = ghw_def_background; /* Use viewport background color */

               #ifdef GHW_USING_RGB
               r += REDSCALE(  bg,alpha);
               g += GREENSCALE(bg,alpha);
               b += BLUESCALE( bg,alpha);
               #else
               grey += GREYSCALE(bg, alpha);
               #endif
               }
            /* Reduce to color value after blend averanging */
            #ifdef GHW_USING_RGB
            r /= ALPHAMAX;
            g /= ALPHAMAX;
            b /= ALPHAMAX;
            #else
            grey /= ALPHAMAX;
            #endif
            }

         #ifdef GHW_USING_RGB
         /* Combine colors */
         c = (GCOLOR) ((r & G_RED_MSK) | (g & G_GREEN_MSK) | (b & G_BLUE_MSK));
         ghw_setpixel( (GXT) pos->x+gcurvp->lt.x, (GYT) pos->y+gcurvp->lt.y, c);
         #else
         ghw_setpixel( (GXT) pos->x+gcurvp->lt.x, (GYT) pos->y+gcurvp->lt.y, (GCOLOR) grey);
         #endif
         return;
         }
      }
   #endif /* GHW_USING_RGB || GHW_USING_GREY_LEVEL */

   if ((x!=0)&&(y!=0))
      {
      c = (GCOLOR) gi_getsym_pixel(x-1,y-1);
      #if (defined( GHW_USING_RGB ) || defined( GHW_USING_GREY_LEVEL))
      if (grey_mode)
         {
         /* The symbol "color" is an intensity level, compose color from foreground and background */
         grey = (SGUINT) c * ALPHAMAX;
         goto grey_processing;  /* Reuse a large code fragment */
         }
      #endif
      if (!transperant || (c != ghw_def_background))
         ghw_setpixel( (GXT) pos->x+gcurvp->lt.x, (GYT) pos->y+gcurvp->lt.y, c);
      }
   }

/*
  Fill line in destination rectangle with data from source symbol.
  The routine make a simplified flood fill to copy every pixel in the
  destination line from the source symbol. There are no guarantees that
  every pixel from the source is used, but every pixel in the destination
  is updated.
  cpypixline tries to go as far left in the destination as the source
  rectangle permits. While doing this, it copies the pixels from the source
  to the destination. The same thing is done from the center to the right.
*/
static void  cpypixline(GSYMPLACE *start)
   {
   GSYMPLACE pos;

   /* Copy to left */
   pos_cpy(&pos, start);

   /* Check source rectangle boundaries */
   while((pos.xo >= 0) && (pos.yo >= 0) &&
         (pos.xo < maxx) && (pos.yo < maxy))
      {
      process_pixel(&pos);
      pos.x++;         /* Move destination right */
      pos.xo += gi_fp_cos;  /* Move within source */
      pos.yo += gi_fp_sin;
      }

   /* Copy to right */
   pos_cpy(&pos, start);
   pos.x--;
   pos.xo -= gi_fp_cos;
   pos.yo -= gi_fp_sin;

   /* Check source rectangle boundaries */
   while((pos.xo >= 0) && (pos.yo >= 0) &&
         (pos.xo < maxx) && (pos.yo < maxy))
      {
      process_pixel(&pos);
      pos.x--;         /* Move destination left */
      pos.xo -= gi_fp_cos;  /* Move within source */
      pos.yo -= gi_fp_sin;
      }

   }


/*
   Prepare sin and cos values and convert to fix point numbers
   for faster processing (65536==1.0), i.e. for the rest of the symbol
   processing avoid floating point calculations and use fast integer
   calculations instead
*/
void gi_setfpsincos( float angle )
   {
   gi_fp_sin = (SGLONG)(sin(angle)*65536.0);
   gi_fp_cos = (SGLONG)(cos(angle)*65536.0);

   /* Snap to axis when angle is close to ortogonal */
   if ((gi_fp_sin < 63) && (gi_fp_sin > -63))
      {
      gi_fp_sin = 0;
      if (gi_fp_cos > 0)
         gi_fp_cos = 65536; /* 1.0 */
      else
         gi_fp_cos = -65536;/* -1.0 */
      }
   else
   if (gi_fp_sin > (65536-63))
      {
      gi_fp_sin = 65536;   /* 1.0 */
      gi_fp_cos = 0;
      }
   else
   if (gi_fp_sin < (-65536+63))
      {
      gi_fp_sin = -65536;  /* -1.0 */
      gi_fp_cos = 0;
      }
   }

/*
   void gi_symrotate( GXT x, GYT y,
                      float angle,
                      PGSYMBOL sym,
                      SGINT xanchor, SGINT yanchor,
                      GMODE symflag,
                      SGUINT symbytewidth
                      )
*/
void gi_symrotate( SGINT x, SGINT y, PGSYMBOL psymbol, SGINT xanchor, SGINT yanchor, GMODE symflag, SGUINT symbytewidth)
   {
   GSYMPLACE goup, godown;
   int  displacement, edgex;
   #ifdef GBUFFER
   GUPDATE update;
   #endif

   #if (defined( GHW_USING_RGB ) || defined( GHW_USING_GREY_LEVEL))
   if ((gi_fp_sin == 0)||(gi_fp_cos== 0))
      blending = 0; /* Alpha blending is not needed, Use exact pixel match */
   else
      blending = (symflag & GNOCOLORBLEND) ? 0 : 1; /* Blending is default */
   #endif

   /* Prepare symbol load */
   if (psymbol == NULL)
      return;

   /* Prepare drawing process flags */
   #if (defined( GHW_USING_RGB ) || defined( GHW_USING_GREY_LEVEL))
   grey_mode = ( gi_getsym_open(psymbol,symbytewidth) & GHW_GREYMODE) ? 1 : 0;
   #else
   gi_getsym_open(psymbol,symbytewidth); /* b&w mode */
   #endif
   fill  = (symflag & GSYMCLR) ? 1 : 0;
   colorback = (symflag & GINVERSE) ? ghw_def_foreground : ghw_def_background;
   transperant = (symflag & GTRANSPERANT) ? 1 : 0;

   /* Max size of source bitmap.*/

   /* Aliazing is done left to right, top to bottom.
      Add one virtual top pixel row and right pixel column to symbol so background
      optionally can be blended in during drawing (give a more clear left/top edge) */
   w=(SGINT)((SGUINT)gsymw(psymbol))+1;
   h=(SGINT)((SGUINT)gsymh(psymbol))+1;

   /* Convert source size into fixed point numbers (65536==1.0) */
   maxx = (((SGLONG)(w)) << 16);
   maxy = (((SGLONG)(h)) << 16);

   /* Add offsets to center caused by anchor connection point mode */
   switch (symflag & GALIGN_HCENTER)
      {
      case GALIGN_RIGHT:
         xanchor = (w-1)/2 - xanchor;  /* Specifed anker point is relative to right edge*/
         break;
      case GALIGN_LEFT:  /* Specifed anker point is relative to left edge*/
         xanchor = (1-((SGINT)w)/2) - xanchor;
         break;
      default: /* GALIGN_HCENTER (or default) */
         xanchor *= -1;
         break;
      }
   switch (symflag & GALIGN_VCENTER)
      {
      case GALIGN_BOTTOM:
         yanchor = (h-1)/2 - yanchor;   /* Specifed anker point is relative to bottom edge*/
         break;
      case GALIGN_TOP:
         yanchor = (1 - ((SGINT)h)/2) - yanchor;   /* Specifed anker point is relative to top edge */
         break;
      default: /* GALIGN_VCENTER (or default) */
         yanchor *= -1;
         break;
      }

   /* Output position (default = center) */
   goup.xo = (SGLONG) maxx/2;
   goup.yo = (SGLONG) maxy/2;
   goup.x = x;
   goup.y = y;
   if ((xanchor != 0) || (yanchor != 0))
      {
      /* Calculate center offsets caused by anchor rotation */
      goup.x = (SGINT)(goup.x-((gi_fp_cos*xanchor + gi_fp_sin*yanchor) >> 16));
      goup.y = (SGINT)(goup.y-((gi_fp_cos*yanchor - gi_fp_sin*xanchor) >> 16));
      }
   pos_cpy(&godown, &goup); /* copy center to "godown" */

   /*   Adjust starting position according to rectangle size and angle
        i.e. find end of fill by transforming a corner of the rectangle */
   if(gi_fp_cos*gi_fp_sin > 0)
      displacement = (-1*((SGLONG)w)*gi_fp_cos + ((SGLONG)h)*gi_fp_sin+65535) >> 17;
   else
      displacement = (-1*((SGLONG)w)*gi_fp_cos - ((SGLONG)h)*gi_fp_sin-65535) >> 17;

   if(gi_fp_sin > 0)
      displacement = - displacement;

   /* edgex is absolut value of displacement */
   edgex = (displacement > 0) ? displacement : - displacement;

   #ifdef GBUFFER
   /* Use drawing completion before flush to screen */
   update = gsetupdate(GUPDATE_OFF);
   #endif

   /* Process up until source rectangle bound is crossed */
   do
      {
      while(goup.xo >= 0 && goup.yo >= 0 &&
            goup.xo < maxx && goup.yo < maxy)
         {
         cpypixline(&goup);
         goup.y--;        /* Go up on screen */
         goup.xo += gi_fp_sin; /* Same move in source bitmap coordinates */
         goup.yo -= gi_fp_cos;
         }

      /* Stay inside symbol bounds as long as possible,
         by adjusting the fill location */
      if(displacement > 0)
         {
         goup.x++;
         goup.xo += gi_fp_cos;
         goup.yo += gi_fp_sin;
         }
      else
         {
         goup.x--;
         goup.xo -= gi_fp_cos;
         goup.yo -= gi_fp_sin;
         }
      }
   while(edgex-- > 0); /* Adjust only as long as it is useful.*/

   /* Reinit*/
   edgex = (displacement > 0) ? displacement : - displacement;

   /* Skip center line which is already drawn */
   godown.y++;
   godown.xo -= gi_fp_sin;
   godown.yo += gi_fp_cos;

   /* Process down until source rectangle bound is crossed */
   do
      {
      while((godown.xo >= 0)   && (godown.yo >= 0) &&
            (godown.xo < maxx) && (godown.yo < maxy))
         {
         cpypixline(&godown);
         godown.y++;          /* Go down on screen */
         godown.xo -= gi_fp_sin;   /* Same move in source bitmap coordinates */
         godown.yo += gi_fp_cos;
         }

      /* Stay inside symbol bounds as long as possible,
         by adjusting the fill location */
      if(displacement < 0)
         {
         godown.x++;
         godown.xo += gi_fp_cos;
         godown.yo += gi_fp_sin;
         }
      else
         {
         godown.x--;
         godown.xo -= gi_fp_cos;
         godown.yo -= gi_fp_sin;
         }
      }
   while(edgex-- > 0);  /* Adjust only as long as it is useful.*/

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
   The current font and viewport is used

   Called by

   Returns width of symbol
*/
SGINT gi_putchw_rotate( GWCHAR ch, SGINT x, SGINT y )
   {
   GMODE mode;
   PGSYMBOL psymbol;
   /* Output symbol using rotated mode, with anker in lower left symbol corner */
   mode = (GMODE) ((((unsigned int) gcurvp->mode) & (GSYMCLR|GNOCOLORBLEND|GINVERSE|GTRANSPERANT))|(GALIGN_BOTTOM|GALIGN_LEFT));
   /* Get symbol for character */
   psymbol = gi_getsymbol( ch, gcurvp->pfont, gcurvp->codepagep);
   if (psymbol != NULL)
      {
      gi_symrotate(x, y, psymbol,0,0,mode, (SGUINT)(gi_fsymsize(gcurvp->pfont)/gi_fsymh(gcurvp->pfont)));
      x = (SGINT)((SGUINT)gsymw(psymbol));
      #ifndef GNOTXTSPACE
      x+=(SGINT)((SGUINT)gcurvp->chln.x);
      #endif
      return x;
      }
   return 0;
   }

#endif

void gputsymrot( SGINT x, SGINT y, float angle, PGSYMBOL psymbol,
                 SGINT xanchor, SGINT yanchor, GMODE symflag)
   {
   gi_setfpsincos(angle); /* Prepare for fixed point calculations */
   gi_symrotate(x,y,psymbol,xanchor,yanchor,symflag,0);
   ghw_updatehw();
   }

#ifdef GFUNC_VP

void gputsymrot_vp( SGUCHAR vp, SGINT x, SGINT y, float angle, PGSYMBOL psymbol,
                    SGINT xanchor, SGINT yanchor, GMODE symflag)
   {
   gi_setfpsincos(angle); /* Prepare for fixed point calculations */
   GSETFUNCVP(vp, gi_symrotate(x,y,psymbol,xanchor,yanchor,symflag,0));
   ghw_updatehw();
   }

#endif

#endif /* GSOFT_SYMBOLS */

