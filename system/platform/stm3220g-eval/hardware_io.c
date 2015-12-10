/**
  ******************************************************************************
  * @file    hardware_io.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    18.08.2015
  * @brief   Реализация управления вводом/выводом микроконтроллера STM32F2 для stm3220g-eval
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"
#include "FreeRTOS.h"

#include "system_hw_io.h"
#include "../platform_hw_map.h"

#define FSMC_BANK_REG_OFFSET(number)	(((number)-1)*2)

/*! Первичная иницициализация внешней памяти.
 *
 *  External SRAM:	Bank1_SRAM2		(A[19:0], D[15:0], NBL[0], NBL[1], NOE, NWE, NE2)
 *  LCD:			Bank1_NORSRAM3	(A[0], D[15:0], NOE, NWE, NE3)
 *
 *  Код функции предполагает, что GPIO-регистры содержат значения по умолчанию (сразу после сброса МК) !
 *  Глобальные переменные здесь нельзя использовать, данные могут быть размещены в еще неинициализированной памяти.
 */
void stm32f2_ext_mem_init(void) {
	/*-- GPIOs Configuration ------------------------------------------------------*/
	/*
	 +-------------------+------------------ -+------------------+-------------------+
	 +                       FSMC pins assignment                                    +
	 +-------------------+------------------- +------------------+-------------------+
	 | PD0  <-> FSMC_D2  | PE0  <-> FSMC_NBL0 | PF0  <-> FSMC_A0 | PG0  <-> FSMC_A10 |
	 | PD1  <-> FSMC_D3  | PE1  <-> FSMC_NBL1 | PF1  <-> FSMC_A1 | PG1  <-> FSMC_A11 |
	 |                   |                    | PF2  <-> FSMC_A2 | PG2  <-> FSMC_A12 |
	 |                   | PE3  <-> FSMC_A19  | PF3  <-> FSMC_A3 | PG3  <-> FSMC_A13 |
	 | PD4  <-> FSMC_NOE |                    | PF4  <-> FSMC_A4 | PG4  <-> FSMC_A14 |
	 | PD5  <-> FSMC_NWE |                    | PF5  <-> FSMC_A5 | PG5  <-> FSMC_A15 |
	 |                   |                    |                  |                   |
	 |                   | PE7  <-> FSMC_D4   |                  |                   |
	 | PD8  <-> FSMC_D13 | PE8  <-> FSMC_D5   |                  |                   |
	 | PD9  <-> FSMC_D14 | PE9  <-> FSMC_D6   |                  | PG9  <-> FSMC_NE2 |
	 | PD10 <-> FSMC_D15 | PE10 <-> FSMC_D7   |                  | PG10 <-> FSMC_NE3 |
	 | PD11 <-> FSMC_A16 | PE11 <-> FSMC_D8   |                  |                   |
	 | PD12 <-> FSMC_A17 | PE12 <-> FSMC_D9   | PF12 <-> FSMC_A6 |                   |
	 | PD13 <-> FSMC_A18 | PE13 <-> FSMC_D10  | PF13 <-> FSMC_A7 |                   |
	 | PD14 <-> FSMC_D0  | PE14 <-> FSMC_D11  | PF14 <-> FSMC_A8 |                   |
	 | PD15 <-> FSMC_D1  | PE15 <-> FSMC_D12  | PF15 <-> FSMC_A9 |                   |
	 +-------------------+--------------------+--------------------------------------+
	*/

	  /* Enable GPIOD, GPIOE, GPIOF and GPIOG interface clock */
	  RCC->AHB1ENR  |= 0x00000078;

	  /* Connect PDx pins to FSMC Alternate function */
	  GPIOD->AFR[0]  |= 0x00cc00cc;
	  GPIOD->AFR[1]  |= 0xcccccccc;
	  /* Configure PDx pins in Alternate function mode */
	  GPIOD->MODER   |= 0xaaaa0a0a;
	  /* Configure PDx pins speed to 100 MHz */
	  GPIOD->OSPEEDR |= 0xffff0f0f;
	  /* Configure PDx pins Output type to push-pull */
	  GPIOD->OTYPER  |= 0x00000000;
	  /* No pull-up, pull-down for PDx pins */
	  GPIOD->PUPDR   |= 0x00000000;

	  /* Connect PEx pins to FSMC Alternate function */
	  GPIOE->AFR[0]  |= 0xc000c0cc;
	  GPIOE->AFR[1]  |= 0xcccccccc;
	  /* Configure PEx pins in Alternate function mode */
	  GPIOE->MODER   |= 0xaaaa808a;
	  /* Configure PEx pins speed to 100 MHz */
	  GPIOE->OSPEEDR |= 0xffffc0cf;
	  /* Configure PEx pins Output type to push-pull */
	  GPIOE->OTYPER  |= 0x00000000;
	  /* No pull-up, pull-down for PEx pins */
	  GPIOE->PUPDR   |= 0x00000000;

	  /* Connect PFx pins to FSMC Alternate function */
	  GPIOF->AFR[0]  |= 0x00cccccc;
	  GPIOF->AFR[1]  |= 0xcccc0000;
	  /* Configure PFx pins in Alternate function mode */
	  GPIOF->MODER   |= 0xaa000aaa;
	  /* Configure PFx pins speed to 100 MHz */
	  GPIOF->OSPEEDR |= 0xff000fff;
	  /* Configure PFx pins Output type to push-pull */
	  GPIOF->OTYPER  |= 0x00000000;
	  /* No pull-up, pull-down for PFx pins */
	  GPIOF->PUPDR   |= 0x00000000;

	  /* Connect PGx pins to FSMC Alternate function */
	  GPIOG->AFR[0]  |= 0x00cccccc;
	  GPIOG->AFR[1]  |= 0x00000cc0;
	  /* Configure PGx pins in Alternate function mode */
	  GPIOG->MODER   |= 0x00280aaa;
	  /* Configure PGx pins speed to 100 MHz */
	  GPIOG->OSPEEDR |= 0x003c0fff;
	  /* Configure PGx pins Output type to push-pull */
	  GPIOG->OTYPER  |= 0x00000000;
	  /* No pull-up, pull-down for PGx pins */
	  GPIOG->PUPDR   |= 0x00000000;

	  /*
	   * FSMC Configuration for external SRAM
	   *
	   * Bank1_SRAM2 is configured as follow:
	   *
	   * FSMC_NORSRAMTimingInitTypeDef::FSMC_AddressSetupTime = 0;
	   * FSMC_NORSRAMTimingInitTypeDef::FSMC_AddressHoldTime = 0;
	   * FSMC_NORSRAMTimingInitTypeDef::FSMC_DataSetupTime = 4;
	   * FSMC_NORSRAMTimingInitTypeDef::FSMC_BusTurnAroundDuration = 1;
	   * FSMC_NORSRAMTimingInitTypeDef::FSMC_CLKDivision = 0;
	   * FSMC_NORSRAMTimingInitTypeDef::FSMC_DataLatency = 0;
	   * FSMC_NORSRAMTimingInitTypeDef::FSMC_AccessMode = FSMC_AccessMode_A;
	   *
	   * FSMC_NORSRAMInitTypeDef::FSMC_Bank = FSMC_Bank1_NORSRAM2;
	   * FSMC_NORSRAMInitTypeDef::FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	   * FSMC_NORSRAMInitTypeDef::FSMC_MemoryType = FSMC_MemoryType_PSRAM;
	   * FSMC_NORSRAMInitTypeDef::FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	   * FSMC_NORSRAMInitTypeDef::FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	   * FSMC_NORSRAMInitTypeDef::FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	   * FSMC_NORSRAMInitTypeDef::FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	   * FSMC_NORSRAMInitTypeDef::FSMC_WrapMode = FSMC_WrapMode_Disable;
	   * FSMC_NORSRAMInitTypeDef::FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	   * FSMC_NORSRAMInitTypeDef::FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	   * FSMC_NORSRAMInitTypeDef::FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	   * FSMC_NORSRAMInitTypeDef::FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	   * FSMC_NORSRAMInitTypeDef::FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	   */
	  RCC->AHB3ENR        |= 0x00000001;
	  FSMC_Bank1->BTCR[2]  = 0x00001015;
	  FSMC_Bank1->BTCR[3]  = 0x00010400;
	  FSMC_Bank1E->BWTR[2] = 0x0fffffff;
}

char stm32f2_ext_sram_test(void) {
	return 1; /* not implemented */
}

void stm32f2_LCD_init(void) {
	/* FSMC Configuration for i80-system 16-bit interface
	 *
	 * Bank1_NORSRAM3 is configured as follow:
	 *
	 * - Access mode = A
	 * - Extended Mode = Disable
	 * - Memory Type = SRAM
	 * - Data/Address MUX = Disable
	 * - Data Width = 16-bit
	 * - Write Operation = Enable
	 * - Asynchronous Wait = Disable
	 * - read/write timings: address setup = 2 HCLKs, address hold = 0, data setup = 9 HCLKs, bus turnaround = 0
	 */
	portDISABLE_INTERRUPTS();
	/* Enable FSMC clock */
	RCC->AHB3ENR |= RCC_AHB3ENR_FSMCEN;
	/* Configure FSMC NOR/SRAM Bank3 */
	FSMC_Bank1->BTCR[FSMC_BANK_REG_OFFSET(3)+0] = FSMC_BCR1_MWID_0 | FSMC_BCR3_WREN;
	FSMC_Bank1->BTCR[FSMC_BANK_REG_OFFSET(3)+1] = 0
			| (2 << POSITION_VAL(FSMC_BTR1_ADDSET))
			| (0 << POSITION_VAL(FSMC_BTR1_ADDHLD))
			| (9 << POSITION_VAL(FSMC_BTR1_DATAST))
			| (0 << POSITION_VAL(FSMC_BTR1_BUSTURN))
			| (0 << POSITION_VAL(FSMC_BTR1_CLKDIV))
			| (0 << POSITION_VAL(FSMC_BTR1_DATLAT))
			| (0 << POSITION_VAL(FSMC_BTR1_ACCMOD));
	FSMC_Bank1E->BWTR[FSMC_BANK_REG_OFFSET(3)+0] = 0x0FFFFFFF;
	/* Enable FSMC NOR/SRAM Bank3 */
	FSMC_Bank1->BTCR[FSMC_BANK_REG_OFFSET(3)+0] |= FSMC_BCR3_MBKEN;
	portENABLE_INTERRUPTS();
}

void stm32f2_ext_pins_init(int platform_hw_resource) {
	switch (platform_hw_resource) {
	case platformhwHeadsetUart:
	case platformhwHeadsetPttIopin:
	case platformhwDataFlashSpi:
	case platformhwMatrixKeyboard:
	case platformhwKeyboardButt1Iopin:
	case platformhwKeyboardButt2Iopin:
	case platformhwKeyboardsLightIopin:
	case platformhwEnRxRs232Iopin:
	case platformhwEnTxRs232Iopin:
	case platformhwDspUart:
	case platformhwDspResetIopin:
	case platformhwAtuUart:
	case platformhwBatterySmbusI2c:
		//TODO: stm32f2_ext_pins_init()
		break;
	default: configASSERT(0); // no such resource
	}
}

void stm32f2_ext_pins_deinit(int platform_hw_resource) {
	switch (platform_hw_resource) {
	case platformhwHeadsetUart:
	case platformhwHeadsetPttIopin:
	case platformhwDataFlashSpi:
	case platformhwMatrixKeyboard:
	case platformhwKeyboardButt1Iopin:
	case platformhwKeyboardButt2Iopin:
	case platformhwKeyboardsLightIopin:
	case platformhwEnRxRs232Iopin:
	case platformhwEnTxRs232Iopin:
	case platformhwDspUart:
	case platformhwDspResetIopin:
	case platformhwAtuUart:
	case platformhwBatterySmbusI2c:
		//TODO: stm32f2_ext_pins_deinit()
		break;
	default: configASSERT(0); // no such resource
	}
}

hal_gpio_pin_t stm32f2_get_gpio_pin(int platform_hw_resource) {
	switch (platform_hw_resource) {
	case platformhwHeadsetPttIopin:
	case platformhwKeyboardButt1Iopin:
	case platformhwKeyboardButt2Iopin:
	case platformhwKeyboardsLightIopin:
	case platformhwEnRxRs232Iopin:
	case platformhwEnTxRs232Iopin:
	case platformhwDspResetIopin:
		//TODO: stm32f2_get_gpio_pin()
		break;
	default: configASSERT(0); // no such resource
	}
	return (hal_gpio_pin_t){0, 0};
}

int stm32f2_get_exti_line(int platform_hw_resource) {
	(void)platform_hw_resource;
	//TODO: stm32f2_get_exti_line()
	return -1;
}

int stm32f2_get_uart_instance(int platform_hw_resource) {
	switch (platform_hw_resource) {
	case platformhwHeadsetUart:
	case platformhwDspUart:
	case platformhwAtuUart:
		//TODO: stm32f2_get_uart_instance()
		break;
	default: configASSERT(0); // no such resource
	}
	return -1;
}
