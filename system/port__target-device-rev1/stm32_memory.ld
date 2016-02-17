/**
  ******************************************************************************
  * @file    stm32_memory.ld
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    12.08.2015
  * @brief   Linker script with memory regions definitions for STM32F207IG Device
  *
  ******************************************************************************
 */

MEMORY
{
  /* Internal FLASH */
  FLASH_firmware (rx)		: ORIGIN = 0x08000000, LENGTH = (4*16K+64K+6*128K)	/* Sectors 0-10: firmware ISR vectors, program code and constants */
  																				/* RESERVED sector 11: hardware info (flashed by factory setup) */

  /* Internal SRAM */
  RAM (rwx)					: ORIGIN = 0x20000000, LENGTH = (128K-10K)			/* firmware program data */
  SWJDP_NEOSERVICE_LINK_SECTION	: ORIGIN = 0x2001D800, LENGTH = 10K				/* special non-initialized section for SWJ-DP NeoService link storage */

  /* External memories */
  MEMORY_B1_SRAM2 (rwx)		: ORIGIN = 0x64000000, LENGTH = 2M					/* firmware program data */
}
