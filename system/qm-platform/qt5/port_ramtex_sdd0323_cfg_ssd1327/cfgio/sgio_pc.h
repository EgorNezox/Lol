/***********************************************************************
 *
 * STIMGATE I/O port definition for the target system.
 *
 *   Target processor CPU family : IOTESTER
 *   Target processor device     : IOTESTER
 *   Target compiler             : PCMODE
 *
 * This file is included by SGIO.H to preprocess the sgxxx() port functions
 * when the C source file is compiled for execution in the PC.
 *
 * The file can be generated with the SGSETUP program.
 * SGSETUP use this file for back-annotation and to generate a corresponding
 * file: SGIO_TA.H for the target C-compiler.
 * NOTE : Do not modify this file directly, unless you are absolutely sure
 * of what you are doing.
 *
 * V3.03  STIMGATE Copyright (c) RAMTEX International 1997
 * Web site, support and upgrade: www.ramtex.dk
 *
 **********************************************************************/

 SGPORTELEM( WR, MM, BY, 0x0001, 0, GHWWR ), /*Graphic LCD data write*/
 SGPORTELEM( RD, MM, BY, 0x0001, 0, GHWRD ), /*Graphic LCD data read*/
 SGPORTELEM( RD, MM, BY, 0x0000, 0, GHWSTA ), /*Graphic LCD status*/
 SGPORTELEM( WR, MM, BY, 0x0000, 0, GHWCMD ), /*Graphic LCD command*/

 SGPORTELEM( WR, MM, WO, 0x0001, 0, GHWWRW ), /*Graphic LCD data write*/
 SGPORTELEM( RD, MM, WO, 0x0001, 0, GHWRDW ), /*Graphic LCD data read*/
 SGPORTELEM( RD, MM, WO, 0x0000, 0, GHWSTAW ), /*Graphic LCD status*/
 SGPORTELEM( WR, MM, WO, 0x0000, 0, GHWCMDW ), /*Graphic LCD command*/
