/**
  ******************************************************************************
  * @file    device.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    20.05.2016
  *
  ******************************************************************************
 */

#ifndef DEVICE_H_
#define DEVICE_H_

/* Время запуска/стабилизации внешнего клока HSE (в мс) */
#define ONBOARD_HSE_CLOCK_STARTUP_TIME_MS	10

/* Следующие определения должны соответствовать определениям в stm32_memory.ld
 */
#define MEMORY_B1_SRAM2_START_ADDRESS			0x64000000
#define MEMORY_B1_SRAM2_LENGTH					(2*1024*1024)
#define FLASH_FIRMWARE_PROGRAM_START_ADDRESS	0x08040000	// FLASH_firmware ORIGIN
#define FLASH_FIRMWARE_PROGRAM_LENGTH			786432		// FLASH_firmware LENGTH
#define FLASH_FIRMWARE_PROGRAM_MAGIC_OFFSET		0x00000000	// offset from start to firmware magic word
#define FLASH_FIRMWARE_PROGRAM_MAGIC_VALUE		0xBAD4DEAD	// valid value of firmware magic word
#define FLASH_FIRMWARE_PROGRAM_ENTRY_OFFSET		0x00000004	// offset from start to firmware startup entry

#endif /* DEVICE_H_ */
