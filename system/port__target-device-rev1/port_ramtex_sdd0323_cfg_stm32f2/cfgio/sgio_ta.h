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

#define LCD_IO_BASE_ADDRESS	0x60000000

/* Used only in GHW_BUS8 mode */
#define GHWWR  (* (SGUCHAR volatile *) ( LCD_IO_BASE_ADDRESS | 0x1 ))
#define GHWRD  (* (SGUCHAR volatile *) ( LCD_IO_BASE_ADDRESS | 0x1 ))
#define GHWSTA (* (SGUCHAR volatile *) ( LCD_IO_BASE_ADDRESS ))
#define GHWCMD (* (SGUCHAR volatile *) ( LCD_IO_BASE_ADDRESS ))

/* Used only in GHW_BUS16 mode */
#define GHWWRW  (* (SGUINT volatile *) ( LCD_IO_BASE_ADDRESS | 0x2 ))
#define GHWRDW  (* (SGUINT volatile *) ( LCD_IO_BASE_ADDRESS | 0x2 ))
#define GHWSTAW (* (SGUINT volatile *) ( LCD_IO_BASE_ADDRESS ))
#define GHWCMDW (* (SGUINT volatile *) ( LCD_IO_BASE_ADDRESS ))

/* Used only in GHW_BUS32 mode */
#define GHWWRDW  (* (SGULONG volatile *) ( LCD_IO_BASE_ADDRESS | 0x4 ))
#define GHWRDDW  (* (SGULONG volatile *) ( LCD_IO_BASE_ADDRESS | 0x4 ))
#define GHWSTADW (* (SGULONG volatile *) ( LCD_IO_BASE_ADDRESS ))
#define GHWCMDDW (* (SGULONG volatile *) ( LCD_IO_BASE_ADDRESS ))
