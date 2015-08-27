/************************* Gscreen.c *******************************

   Creation date: 30-03-02

   Revision date:
   Revision Purpose:

   Revision date:     03-08-02
   Revision Purpose:  gscinit(PGSCREENS * scp) changed to gscinit(PGSCREENS scp)
                      gscisowner(PGSCREENS scp) added

   Revision date:     01-07-10
   Revision Purpose:  New gcurvpnum used as primary save / restore parameter to optimize
                      viewport handling.

   Version number: 1.2
   Copyright (c) RAMTEX Engineering Aps 2002-2010

*********************************************************************/

#include <gi_disp.h> /* gLCD prototypes */

#ifdef GSCREENS
#include <string.h>  /* memcpy */
/* <stdlib.h> is included via gdisphw.h */

/* Structure for screen save */
typedef struct
   {
   GCOMMON_DATA common_data;
   GCURSOR cursor;
   SGUCHAR curvpnum;
   } _GCOMMON_STATE;

typedef union
   {
   _GCOMMON_STATE state;
   char buf[1];
   } GCOMMON_STATE, *PGCOMMON_STATE;

/*
  Check if screen is the current owner of the screen resources.
  Return 1 if owner
  Return 0 if not owner (or pointer error)
*/
SGUCHAR gscisowner( PGSCREENS screen )
   {
   SGUINT i;
   if (screen == NULL)
      return 0;
   i = sizeof(GCOMMON_STATE);
   return ghw_is_owner( (SGUCHAR *) &(((PGCOMMON_STATE) screen)->buf[i]) );
   }

/*
   Update save state for the current screen
*/
void gscsave( PGSCREENS screen )
   {
   PGCOMMON_STATE p;
   SGUCHAR *pbuf;
   SGUINT i;
   if (screen == NULL)
      return;
   p = (PGCOMMON_STATE) screen;
   i = sizeof(GCOMMON_STATE);
   pbuf = (SGUCHAR *) &(p->buf[i]);
   /* Take snap shot of low-level driver states */
   if (ghw_save_state(pbuf) != pbuf)
      {
      G_WARNING("gscsave() : parameter was not the current screen");
      return;
      }
   /* Take snap shot of the library and viewport states */
   #ifndef GNOCURSOR
   p->state.cursor = gsetcursor((GCURSOR)0);
   #else
   p->state.cursor = ((GCURSOR)0);
   #endif
   p->state.curvpnum = gcurvpnum; /* Save */
   memcpy(&(p->state.common_data),&gdata,sizeof(GCOMMON_DATA));
   }

/*
   Set a screen so it become the active screen
*/
void gscrestore(PGSCREENS screen)
   {
   PGCOMMON_STATE p;
   SGUINT i;
   if (screen == NULL)
      return;
   glcd_err = 0;
   p = (PGCOMMON_STATE) screen;
   memcpy(&gdata,&(p->state.common_data),sizeof(GCOMMON_DATA));
   gcurvpnum = p->state.curvpnum;
   gcurvp = &gdata.viewports[ gcurvpnum ];
   i = sizeof(GCOMMON_STATE);
   ghw_set_state((SGUCHAR *)&p->buf[i],0); /* Make visual swap to new screen */
   gsetcursor(p->state.cursor);
   }

/*
   Return size of the buffer needed to hold the screen
*/
GBUFINT gscsize(void)
   {
   return ((GBUFINT) sizeof(GCOMMON_STATE) + ghw_gbufsize());
   }

/*
   Initialize a new screen.
*/
SGUCHAR gscinit(PGSCREENS scp)
   {
   PGCOMMON_STATE p;
   SGUCHAR *pbuf;
   if (scp != NULL)
      {
      memset(scp,0,gscsize());  /* Clear memory area for screen */
      p = (PGCOMMON_STATE) scp;
      pbuf = (SGUCHAR *)&p->buf[0];
      ghw_set_state(&pbuf[sizeof(GCOMMON_STATE)],1); /* Initialize buffers */
      glcd_err = 0;
      ginit();             /* Initialize basic library and hardware states */
      if (glcd_err)        /* Some low level error, remove gbuffer */
         {
         ghw_set_state(NULL,1);
         return 1;   /* Error */
         }
      #ifndef GNOCURSOR
      p->state.cursor = gsetcursor((GCURSOR)0);
      #else
      p->state.cursor = ((GCURSOR)0);
      #endif
      gcurvpnum = p->state.curvpnum;
      gcurvp = &gdata.viewports[ gcurvpnum ];
      memcpy(&(p->state.common_data), &gdata, sizeof(GCOMMON_DATA));
      }
   else
      {
      ghw_set_state(NULL,1);
      }
   return 0;
   }

#endif

