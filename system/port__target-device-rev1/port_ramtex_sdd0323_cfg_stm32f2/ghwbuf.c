/************************** ghwbuf.c *****************************

   Low-level driver functions for the SSD0323 OLED display buffer
   handling. Constraints low-level functions for optimized buffer
   handling in buffered mode (GBUFFER defined) when softfonts and
   graphic is used.

   NOTE: These functions are only called by the GDISP high-level
   functions. They should not be used directly from user programs.

   The SSD0323 controller is assumed to be used with a OLED module.
   The following OLED module characteristics MUST be correctly
   defined in GDISPCFG.H:

      GDISPW  Display width in pixels
      GDISPH  Display height in pixels
      GBUFFER If defined most of the functions operates on
              a memory buffer instead of the OLED hardware.
              The memory buffer content is compied to the OLED
              display with ghw_updatehw().
              (Equal to an implementation of delayed write)

   Revision date:    030804
   Revision Purpose: Module is now only needed when GBUFFER is defined
   Revision date:    27-02-2015
   Revision Purpose: Support for use of vertical storage units
                     and 90-(270) degree rotation added
   Revision date:
   Revision Purpose:

   Version number: 1.10
   Copyright (c) RAMTEX Engineering Aps 2004-2015

*********************************************************************/
#include <ssd0323.h>   /* ssd0323 controller specific definements */
#ifdef IOTESTER_USB
#include <iotester.h>
#endif
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
   GHWCOLOR *adr; /* fast pointer */
   register GXT xcnt;

   if (ghw_upddelay)
      return;

   GBUF_CHECK();
   glcd_err = 0;

   /* update invalid rect */
   if (( irby >= ilty ) && ( irbx >= iltx ))
      {
      if( irby >= GDISPH ) irby = GDISPH-1;
      if( irbx >= GDISPW ) irbx = GDISPW-1;
      #ifdef  GHW_SINGLE_CHIP
      simcsset(); /* Activate chip select in advance (if needed) */
      #endif

      /* Set both x,y ranges in advance and take advantage of
         the controllers auto wrap features */
      ghw_set_yrange(ilty,irby);
      ghw_set_xrange(iltx,irbx);

      #ifdef GHW_USING_VBYTE
      irbx -= iltx;         /* Number of x bytes */
          ilty = GPIXMSK(ilty); /* Align with whole physical storage unit */

      /* 1 buffer storage unit = 1 hardware storage units */

      for (; ilty <= irby; ilty+= GHWPIXPSU)
         {
         adr = &gbuf[GINDEX(iltx,ilty)];
         /* Loop columns*/
         xcnt = irbx;
         do
            {
            ghw_auto_wr(*adr++);
            }
         while(xcnt-- != 0);
         }

      #else  /*GHW_USING_VBYTE*/

      /* Horizontal video bytes */
          iltx = GPIXMSK(iltx); /* Align with whole physical storage unit */
          irbx = GPIXMSK(irbx);

          /* Loop rows */
      for (; ilty <= irby; ilty++)
         {
         adr = &gbuf[ GINDEX(iltx,ilty)];
         /* 1 buffer storage unit = 1 hardware storage units */
         for (xcnt = iltx;;)
            {
            ghw_auto_wr(*adr);
            if (xcnt >= irbx)
               break;
            xcnt += GHWPIXPSU;
            adr++;
            }
         }

      #endif /*GHW_USING_VBYTE*/

      #ifdef  GHW_SINGLE_CHIP
      simcsclr(); /* Deactivate chip select after flush (if needed) */
      #endif
      /* Invalidate dirty area range */
      iltx = 1;
      ilty = 1;
      irbx = 0;
      irby = 0;
      }
   #if (defined(_WIN32) && defined(GHW_PCSIM))
   GSimFlush();
   #endif
   #ifdef IOTESTER_USB
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


