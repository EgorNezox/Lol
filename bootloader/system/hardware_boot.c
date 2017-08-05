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
#include "system.h"
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
	if (*((uint16_t *)(FLASH_FIRMWARE_PROGRAM_START_ADDRESS + FLASH_FIRMWARE_PROGRAM_ENTRY_OFFSET)) == 0xFFFF)
		return false;
	return true;
}

bool hwboot_check_usbcdc(void) {
	if (*((uint16_t *)(FLASH_USBFLASHER_PROGRAM_START_ADDRESS)) == 0xFFFF)
		return false;
	return true;
}

#if NEW_BOOTLOADER
	void hwboot_jump_cdc(void)
#else
	void hwboot_jump_usbflasher(void)
#endif
{
	/* Interrupts must be disabled to safely proceed
	 * (before firmware switches to its isr vectors table and initializes interrupts).
	 */
	__disable_irq();

	/* Firstly, disable and clear pending interrupts from all mcu peripherals */
	for (int i = 0; i <= 80; i++) {
		NVIC_DisableIRQ((IRQn_Type)i);
		NVIC_ClearPendingIRQ((IRQn_Type)i);
	}
	/* Set system clock from HSI oscillator (and reset prescalers) */
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1  | RCC_CFGR_PPRE2);
	while ((RCC->CFGR & RCC_CFGR_SWS ) != RCC_CFGR_SWS_HSI);
	/* Reset access latency of flash interface to zero CPU cycles */
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	/* Reset HSE and main PLL */
	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_PLLON);
	/* Reset all peripheral clocks
	 * Warning: breaks access to external memory (if any) since here !
	 */
	RCC->AHB1ENR = 0;
	RCC->AHB2ENR = 0;
	RCC->AHB3ENR = 0;
	RCC->APB1ENR = 0;
	RCC->APB2ENR = 0;

	/* Jump to firmware entry address */
	uint32_t* entry_addr = (uint32_t*)(FLASH_USBFLASHER_PROGRAM_START_ADDRESS + 4);
	__asm volatile("bx %0" :: "r" (*entry_addr | 1));
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

void hwboot_jump_system_bootloader(void) {
	stm32f2_enter_bootloader();
}

static bool test_hse_clock(void) {
	bool result = hal_rcc_enable_hse(false, ONBOARD_HSE_CLOCK_STARTUP_TIME_MS);
	hal_rcc_disable_hse();
	return result;
}
