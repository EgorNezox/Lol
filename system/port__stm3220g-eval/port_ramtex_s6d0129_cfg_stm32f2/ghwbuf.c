/*********************** ghwbuf.c *****************************

   Low-level driver functions for the S6D0129 LCD display buffer
   handling. Constraints low-level functions for optimized buffer
   handling in buffered mode (GBUFFER defined) when softfonts and
   graphic is used.

   NOTE: These functions are only called by the GDISP high-level
   functions. They should not be used directly from user programs.

   The S6D0129 controller is assumed to be used with a LCD module.
   The following LCD module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the LCD hardware.
              The memory buffer content is compied to the LCD
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)


   Creation date:
   Revision date:     10-09-10
   Revision Purpose:  IOTester-USB auto sync support added.

   Revision date:
   Revision Purpose:

   Version number: 1.00
   Copyright (c) RAMTEX Engineering Aps 2007

*********************************************************************/
#include <s6d0129.h>    /* controller specific definements */
#ifdef GHW_SINGLE_CHIP
#include <bussim.h>
#endif

#ifdef GBASIC_INIT_ERR
/****************************************************************
 ** functions for internal implementation
 ****************************************************************/

/*
   Update HW with buffer content if buffered driver else nothing
*/
#ifdef GBUFFER

void ghw_updatehw(void)
   {
   GCOLOR *cp; /* fast pointer */
   GXT x;

   if (ghw_upddelay)
      return;

   GBUF_CHECK();
   glcd_err = 0;

   /* update invalid rect */
   if (( irby >= ilty ) && ( irbx >= iltx ))
      {
      if( irby >= GDISPH ) irby = GDISPH-1;
      if( irbx >= GDISPW ) irbx = GDISPW-1;

      /* Set both x,y ranges in advance and take advantage of
         the controllers auto wrap features */
      ghw_set_xyrange(iltx,ilty,irbx,irby);

      /* Loop rows */
      for (;ilty <= irby; ilty++)
         {
         cp = &gbuf[ GINDEX(iltx,ilty) ];
         /* Loop columns*/
         for (x = iltx; x <= irbx; x++, cp++)
            ghw_auto_wr(*cp);
         }

      /* Invalidate dirty area range */
      iltx = 1;
      ilty = 1;
      irbx = 0;
      irby = 0;
      }
   #if (defined(_WIN32) && defined(GHW_PCSIM))
   GSimFlush();
   #endif
   #if (defined(_WIN32) && defined(IOTESTER_USB))
   iot_sync(IOT_SYNC);
   #endif
   #if (defined(_WIN32) && defined(IOTESTER_USB))
   iot_sync(IOT_SYNC);
   #endif
   }

/*
   Set updatehw to instant update or delayed update
      1 = Normal screen update from buffer
      0 = Update from buffer stopped until normal update is selected again

   Activated from gsetupdate(on);
*/
GUPDATE ghw_setupdate( GUPDATE update )
   {
   GUPDATE old_update;
   old_update = (GUPDATE) (ghw_upddelay == 0);
   if ((update != 0) && (ghw_upddelay != 0))
      {
      /* Update is re-activated, make a screen update. */
      ghw_upddelay = 0;  /* Flag = 0 -> no delayed update */
      ghw_updatehw();
      }
   else
      ghw_upddelay = (update == 0);
   return old_update;
   }
#endif  /* GBUFFER */

#endif  /* GBASIC_INIT_ERR */


