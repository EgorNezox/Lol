/***********************************************************************
 *
 * This file contains I/O port definition for the target system and is
 * included by sgio.h
 *
 * This file is normally generated automatically by the SGSETUP tool using
 * the sgio_pc.h header file as a template. The definitions here correspond
 * to I/O port definitions in sgio_pc.h but are here using the I/O register
 * definition syntax for the target compiler.
 *
 * The example definitions here assume that the LCD I/O registers are
 * memory mapped at fixed addresses, and that the register SELECT line on
 * the display is connected to address bus bit 0 signal for 8 bit bus and.
 * to address bus bit 1 signal for a 16 bit bus.
 *
 * Modify these definitions so they fit the actual target system and the
 * I/O register definition syntax used by the actual target compiler.
 *
 * Copyright (c) RAMTEX International 2006
 * Version 1.0
 *
 **********************************************************************/

#if defined(PORT__TARGET_DEVICE)
/* Used only in GHW_BUS8 mode */
	#define GHWWR  (* (SGUCHAR volatile *) ( 0x60000001 ))
	#define GHWRD  (* (SGUCHAR volatile *) ( 0x60000001 ))
	#define GHWSTA (* (SGUCHAR volatile *) ( 0x60000000 ))
	#define GHWCMD (* (SGUCHAR volatile *) ( 0x60000000 ))
#elif defined(PORT_DEVICE_MODEL)
	#define GHWWR  (* (SGUCHAR volatile *) ( 0x68000001 ))
	#define GHWRD  (* (SGUCHAR volatile *) ( 0x68000001 ))
	#define GHWSTA (* (SGUCHAR volatile *) ( 0x68000000 ))
	#define GHWCMD (* (SGUCHAR volatile *) ( 0x68000000 ))
#endif /* PORT__* */


/* Used only in GHW_BUS16 mode */
#define GHWWRW  (* (SGUINT volatile *) ( 0x68000002 ))
#define GHWRDW  (* (SGUINT volatile *) ( 0x68000002 ))
#define GHWSTAW (* (SGUINT volatile *) ( 0x68000000 ))
#define GHWCMDW (* (SGUINT volatile *) ( 0x68000000 ))

/* Used only in GHW_BUS32 mode */
#define GHWWRDW  (* (SGULONG volatile *) ( 0x68000004 ))
#define GHWRDDW  (* (SGULONG volatile *) ( 0x68000004 ))
#define GHWSTADW (* (SGULONG volatile *) ( 0x68000000 ))
#define GHWCMDDW (* (SGULONG volatile *) ( 0x68000000 ))


