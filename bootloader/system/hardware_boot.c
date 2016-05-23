/**
  ******************************************************************************
  * @file    hardware_boot.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    19.05.2016
  *
  ******************************************************************************
  */

#include <stdbool.h>
#include "stm32f2xx.h"
#include "system_hw_io.h"
#include "hal_rcc.h"
#include "device.h"

#include "hardware_boot.h"

static bool test_hse_clock(void);

static void preinit_hardware_io(void) {
	/* Set CS_PLL pin (PH15) to high level (prevent spurious activation of sky72310 spi interface) */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
	GPIOH->MODER |= GPIO_MODER_MODER15_0;
	GPIOH->ODR |= GPIO_ODR_ODR_15;
	/* Set RESET_DSP pin (PH13) to low level (prevent DSP from early startup) */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
	GPIOH->MODER |= GPIO_MODER_MODER13_0;
}

/*! Обработчик системного сброса (точка входа в загрузчик)
 *
 * Выполняется при сбросе/включении устройства и ответственен за запуск инициализации устройства.
 * Внимание: память на данном этапе не инициализирована, в данном контексте нельзя обращаться к глобальным переменным !
 */
void Reset_Handler(void) {
	preinit_hardware_io();
	stm32f2_ext_mem_init();
	__asm volatile("b System_Startup_Entry");
}

hwboot_test_result_t hwboot_test_board(void) {
	if (!stm32f2_ext_sram_test())
		return hwboottestErrorExtSram;
	if (!test_hse_clock())
		return hwboottestErrorHseClock;
	return hwboottestOk;
}

bool hwboot_check_firmware(void) {
#ifdef NDEBUG
	if (*((uint32_t *)(FLASH_FIRMWARE_PROGRAM_START_ADDRESS + FLASH_FIRMWARE_PROGRAM_MAGIC_OFFSET)) != FLASH_FIRMWARE_PROGRAM_MAGIC_VALUE)
		return false;
#else
	if (*((uint16_t *)(FLASH_FIRMWARE_PROGRAM_START_ADDRESS + FLASH_FIRMWARE_PROGRAM_ENTRY_OFFSET)) == 0xFFFF)
		return false;
#endif
	return true;
}

void hwboot_jump_firmware(void) {
	/* Interrupts must be disabled to safely proceed
	 * (before firmware switches to its isr vectors table and initializes interrupts).
	 */
	__disable_irq();
	/* Jump to firmware entry address */
	uint32_t entry_addr = FLASH_FIRMWARE_PROGRAM_START_ADDRESS + FLASH_FIRMWARE_PROGRAM_ENTRY_OFFSET;
	__asm volatile("bx %0" :: "r" (entry_addr | 1));
}

static bool test_hse_clock(void) {
	bool result = hal_rcc_enable_hse(false, ONBOARD_HSE_CLOCK_STARTUP_TIME_MS);
	hal_rcc_disable_hse();
	return result;
}
