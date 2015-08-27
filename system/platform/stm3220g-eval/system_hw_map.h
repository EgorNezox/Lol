/**
  ******************************************************************************
  * @file    system_hw_map.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    18.08.2015
  * @brief   Интерфейс и определения назначения ресурсов микроконтроллера STM32F2
  *
  ******************************************************************************
  */

#ifndef SYSTEM_HW_MAP_H_
#define SYSTEM_HW_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Следующие определения должны соответствовать определениям в stm32_memory.ld
 */
#define MEMORY_B1_SRAM2_START_ADDRESS			0x64000000
#define MEMORY_B1_SRAM2_SIZE_WORDS				(1*1024*1024) // 1M x 16
#define MEMORY_B1_SRAM2_LENGTH					(2*MEMORY_B1_SRAM2_SIZE_WORDS) // bytes
#define FLASH_HARDWARE_INFO_WORD_ADDR			0x08004000 // flash sector 1, word offset 0x0
#define _HARDWARE_REV		((uint8_t)((*((uint32_t *)FLASH_HARDWARE_INFO_WORD_ADDR) >> 24) & 0xFF))
#define _HARDWARE_SERIAL	((uint16_t)(*((uint32_t *)FLASH_HARDWARE_INFO_WORD_ADDR) & 0xFFFF))

/* Время запуска/стабилизации внешнего клока HSE (в мс) */
#define	ONBOARD_HSE_CLOCK_STARTUP_TIME_MS	10

void stm32f2_ext_mem_init(void);
char stm32f2_ext_sram_test(void);
void stm32f2_LCD_init(void);

#ifdef __cplusplus
}
#endif
#endif /* SYSTEM_HW_MAP_H_ */
