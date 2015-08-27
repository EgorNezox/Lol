#ifndef S1D013517_H
#define S1D013517_H
/*
   Define low-level driver settings and functions specific for S1D13517
   in addition to the normal settings in s6d0129.h
*/

#include <s6d0129.h>   /* Controller specific definements */

#ifdef __cplusplus
extern "C" {
#endif

/********** public functions in low-level drivers ********/

/*
   Select the frame buffer to work on and update
   (May be different from the frame buffer shown on screen)
*/
void ghw_wrbufsel(SGUCHAR bufnum);

/*
   Select the frame buffer to show on the screen
*/
void ghw_bufsel(SGUCHAR bufnum);

/********** used by low-level drivers only ********/
void ghw_linestart(void);
void ghw_lineend(void);

#if (GDISPPIXW == 16)
   #define G_TRANSCOLOR           (GCOLOR)0xffdf
   #define G_TRANSCOLOR_REPLACE   (GCOLOR)0xffff
#else /* (GDISPPIXW == 24) */
   #define G_TRANSCOLOR           (GCOLOR)0xfff7ff
   #define G_TRANSCOLOR_REPLACE   (GCOLOR)0xfffbff
#endif

#define GHW_COLORCHECK(color) { if (color == G_TRANSCOLOR) color = G_TRANSCOLOR_REPLACE; }

#ifdef __cplusplus
}
#endif

#endif /* S1D013517_H */

