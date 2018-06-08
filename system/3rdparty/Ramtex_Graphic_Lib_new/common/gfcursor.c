/************************* gfcursor.c ******************************

   Handles the soft font cursor

   Creation date: 980223

   Revision date:     02-01-27
   Revision Purpose:  Vertical softfort cursor support.
                      Cursor handling changed from background-save/restore
                      to background inverse toggling. This saves RAM
                      memory used for storing data under cursor.
   Revision date:     02-03-08
   Revision Purpose:  Cursor on last pixel line corrected.
   Revision date:     14-04-05
   Revision Purpose:  GHW_NO_LCD_READ_SUPPORT optimization switch added
   Revision date:     16-11-07
   Revision Purpose:  Vfont adaption
   Revision date:     16-09-15
   Revision Purpose:  Cursor blink corrected.

   Version number: 2.5
   Copyright (c) RAMTEX International Aps 1998-2015
   Web site, support and upgrade: www.ramtex.dk

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

/********************************************************************/

/* GNOCURSOR is automatically defined if GBASIC_TEXT is undefined */
#ifndef GNOCURSOR

/*
   Return width of cursor in current viewport
*/
GXT gcursor_width( void )
   {
   GXT w;
   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
      w = GDISPCW;
   else
   #endif
   if (GDATA_CURSOR & GCURBLOCK)
      w = gcurvp->fsize.x-1; /* cursor width  = font width */
   else
   if (GDATA_CURSOR & GCURVERTICAL)
      w = (GDATA_CURSOR&0x0F);        /* cursor width  = setting */
   else
      {
      w = gcurvp->fsize.x;   /* cursor width = font width */
      if (w > 5) w -= 2;            /* Adjust look */
      }
   return w;
   }

/*
   Return height of cursor in current viewport
*/
GYT gcursor_height( void )
   {
   GYT h;
   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
      h = GDISPCH;
   else
   #endif
   if (GDATA_CURSOR & GCURBLOCK)
      h = gcurvp->fsize.y-1; /* cursor height = font height */
   else
   if (GDATA_CURSOR & GCURVERTICAL)
      {
      h = gcurvp->fsize.y; /* cursor height = font height */
      if (h > 6) h -= 2;            /* Adjust look */
      }
   else
      h = (GDATA_CURSOR&0x0F)+1;    /* cursor height  = setting */
   return h;
   }

/********************************************************************
internal function
*/
void gi_cursor( SGBOOL on )
   {
   #if (defined( GSOFT_FONTS ) && !defined( GVP_NO_READ_SUPPORT ))
   GXT x;
   GYT y;
   GXT GFAST w;
   GYT GFAST h;
   #endif

   if (on == 0) glcd_err = 0;     /* Reset HW error flag at first call */
   if( (GDATA_CURSOR & GCURON) == 0 ) /* always kill cursor */
      on = 0;

   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
      {
      /* Change hardware cursor temporary */
      GCURSOR c = GDATA_CURSOR;
      ghw_setcursor( (GCURSOR) (on ? (c | GCURON) : (c & ~GCURON)));
      GDATA_CURSOR = c;
      return;
      }
   #endif

   #if (defined( GSOFT_FONTS ) && !defined( GVP_NO_READ_SUPPORT ))
   if( on != GDATA_CURSORON )  /* alter cursor */
      {
      GDATA_CURSORON = on;     /* Invert cursor area */
      if ((gcurvp->cpos.x > gcurvp->rb.x) || (gcurvp->cpos.y > gcurvp->rb.y))
         {
         /* Cursor is temporary out of viewport area (because of GLINECUT or GNOSCROLL) */
         return;
         }
      x = gcurvp->cpos.x;
      y = gcurvp->cpos.y;
      w = gcursor_width();
      h = gcursor_height();
      if (y < h)
         y = 0;
      else
         y -= h;
      if( x+w >= gcurvp->rb.x )
         w = gcurvp->rb.x-x; /* truncate at VP */
      if( y+h > gcurvp->rb.y )
         h = gcurvp->rb.y-y;   /* truncate at VP */

      /* Invert bits under cursor, the rest remain unchanged */
      ghw_invert(x, y, (GXT)(x+w), (GYT)(y+h));
      ghw_updatehw();
      }
   #endif
   }

/********************************************************************
   Segment: View port
   Level: Fonts
   Cursor is set on/off by a call to gcursorblink()
   from the application (typically called from a timer function)
   If SYSFONT the HW support for cursor is used.
   The cursor is Ored on the graphics currently on the LCD
*/
#ifdef GSOFT_FONTS
void gcursorblink(void)
   {
   gi_datacheck(); /* check internal data for errors */
   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
      return; /* let HW do the job */
   #endif

   glcd_err = 0;
   if( (GDATA_CURSOR & GCURBLINK) == 0 )
      gi_cursor( 1 ); /* always on */
   else
      gi_cursor( (SGBOOL)(GDATA_CURSORON == 0) );
   }
#endif /* GSOFT_FONTS */


/********************************************************************
   Segment: Basic text
   Level: Fonts
   Sets the cursor type as a combination of GCURSOR
   Returns the old cursor type
*/
GCURSOR gi_setcursor( GCURSOR type )
   {
   GCURSOR c;
   gi_datacheck(); /* check internal data for errors */
   gi_cursor( 0 ); /* set cursor off */

   #ifndef GHW_NO_HDW_FONT
   if( gishwfont() )
      {
      c = GDATA_CURSOR;
      ghw_setcursor( type );
      }
   else
   #endif
      {
      c = GDATA_CURSOR;
      ghw_setcursor( (GCURSOR)(type & ~GCURON) ); /* set off HW cursor */
      #ifdef GSOFT_FONTS
      GDATA_CURSOR = type;
      gi_cursor((SGBOOL)((type&GCURON) ? 1 : 0)); /* set cursor on/off */
      #endif
      }

   return c;
   }

#endif  /* GNOCURSOR */


