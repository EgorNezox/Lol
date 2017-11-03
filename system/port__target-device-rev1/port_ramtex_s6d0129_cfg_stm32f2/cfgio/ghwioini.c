/************************* GHWIOINI.C ******************************

   Inittialize and deinitialize target specific LCD resources.

   Creation date: 011111

   Revision date:
   Revision Purpose:

   Version number: 2.00
   Copyright (c) RAMTEX Engineering Aps 2001

*********************************************************************/

#ifndef GHW_NOHDW
#ifdef PORT__TARGET_DEVICE_REV1
#include "system_hw_io.h"	/* !!! target-specific !!! */
#endif
#endif
#include <gdisphw.h>

#ifdef GHW_SINGLE_CHIP
void sim_reset( void );
#endif

#ifdef GBASIC_INIT_ERR
/*
   ghw_io_init()

   This function is called once by ginit() via ghw_init() before any LCD
   controller registers is addressed. Any target system specific
   initialization like I/O port initialization can be placed here.

*/
void ghw_io_init(void)
   {
   #ifndef GHW_NOHDW
   #ifdef GHW_SINGLE_CHIP
   sim_reset();  /* Initiate LCD bus simulation ports */
   #endif
   #ifdef PORT__TARGET_DEVICE_REV1
   stm32f2_LCD_init(); /* !!! target-specific !!! */
    #endif
   #endif
   }

/*
  This function is called once by gexit() via ghw_exit() as the last operation
  after all LCD controller operations has stopped.
  Any target system specific de-initialization, like I/O port deallocation
  can be placed here. In most embedded systems this function can be empty.
*/
void ghw_io_exit(void)
   {
   #ifndef GHW_NOHDW
	/* !!! target-specific !!! */
   #endif
   }

#endif /* GBASIC_INIT_ERR */


