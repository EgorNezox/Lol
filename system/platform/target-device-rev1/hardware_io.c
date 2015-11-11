/**
  ******************************************************************************
  * @file    hardware_io.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    18.08.2015
  * @brief   Реализация управления вводом/выводом микроконтроллера STM32F2 для target-device-rev1
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"
#include "FreeRTOS.h"

#include "system_hw_io.h"
#include "../platform_hw_map.h"
#include "hal_gpio.h"
#include "hal_timer.h"

#define _STR(arg) #arg
#define STR(arg) _STR(arg)

/* Следующие определения должны соответствовать определениям в stm32_memory.ld
 */
#define MEMORY_B1_SRAM2_START_ADDRESS			0x64000000
#define MEMORY_B1_SRAM2_SIZE_WORDS				(1*1024*1024) // 1M x 16
#define MEMORY_B1_SRAM2_LENGTH					(2*MEMORY_B1_SRAM2_SIZE_WORDS) // bytes

char stm32f2_ext_sram_test(void) __attribute__((optimize("-O0")));

/*! Первичная иницициализация внешней памяти.
 *
 *  External SRAM:	Bank1_SRAM2		(A[19:0], D[15:0], NBL[0], NBL[1], NOE, NWE, NE2)
 *  LCD:			Bank1_NORSRAM1	(A[0], D[7:0], NOE, NWE, NE1)
 *
 *  Код функции предполагает, что GPIO-регистры содержат значения по умолчанию (сразу после сброса МК) !
 *  Глобальные переменные здесь нельзя использовать, данные могут быть размещены в еще неинициализированной памяти.
 */
void stm32f2_ext_mem_init(void) {
	/*-- GPIOs Configuration -------------------------------------------------------*/
	/*
	 +-------------------+--------------------+------------------+-------------------+
	 +                       FSMC pins assignment                                    +
	 +-------------------+--------------------+------------------+-------------------+
	 | PD0  <-> FSMC_D2  | PE0  <-> FSMC_NBL0 | PF0  <-> FSMC_A0 | PG0  <-> FSMC_A10 |
	 | PD1  <-> FSMC_D3  | PE1  <-> FSMC_NBL1 | PF1  <-> FSMC_A1 | PG1  <-> FSMC_A11 |
	 |                   |                    | PF2  <-> FSMC_A2 | PG2  <-> FSMC_A12 |
	 |                   | PE3  <-> FSMC_A19  | PF3  <-> FSMC_A3 | PG3  <-> FSMC_A13 |
	 | PD4  <-> FSMC_NOE |                    | PF4  <-> FSMC_A4 | PG4  <-> FSMC_A14 |
	 | PD5  <-> FSMC_NWE |                    | PF5  <-> FSMC_A5 | PG5  <-> FSMC_A15 |
	 |                   |                    |                  |                   |
	 | PD7  <-> FSMC_NE1 | PE7  <-> FSMC_D4   |                  |                   |
	 | PD8  <-> FSMC_D13 | PE8  <-> FSMC_D5   |                  |                   |
	 | PD9  <-> FSMC_D14 | PE9  <-> FSMC_D6   |                  | PG9  <-> FSMC_NE2 |
	 | PD10 <-> FSMC_D15 | PE10 <-> FSMC_D7   |                  |                   |
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
	  GPIOD->AFR[0]  |= 0xc0cc00cc;
	  GPIOD->AFR[1]  |= 0xcccccccc;
	  /* Configure PDx pins in Alternate function mode */
	  GPIOD->MODER   |= 0xaaaa8a0a;
	  /* Configure PDx pins speed to 100 MHz */
	  GPIOD->OSPEEDR |= 0xffffcf0f;
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
	  GPIOG->AFR[1]  |= 0x000000c0;
	  /* Configure PGx pins in Alternate function mode */
	  GPIOG->MODER   |= 0x00080aaa;
	  /* Configure PGx pins speed to 100 MHz */
	  GPIOG->OSPEEDR |= 0x000c0fff;
	  /* Configure PGx pins Output type to push-pull */
	  GPIOG->OTYPER  |= 0x00000000;
	  /* No pull-up, pull-down for PGx pins */
	  GPIOG->PUPDR   |= 0x00000000;

/*-- FSMC Configuration for external SRAM ------------------------------------------------------*/
	  /* Enable the FSMC interface clock */
	  RCC->AHB3ENR        |= 0x00000001;

	  /* Configure and enable Bank1_SRAM2 */
	  FSMC_Bank1->BTCR[2]  = 0x00001015;
	  FSMC_Bank1->BTCR[3]  = 0x00010800;
	  FSMC_Bank1E->BWTR[2] = 0x0fffffff;

	  /*
	  Bank1_SRAM2 is configured as follow:

	  p.FSMC_AddressSetupTime = 0;
	  p.FSMC_AddressHoldTime = 0;
	  p.FSMC_DataSetupTime = 8;
	  p.FSMC_BusTurnAroundDuration = 1;
	  p.FSMC_CLKDivision = 0;
	  p.FSMC_DataLatency = 0;
	  p.FSMC_AccessMode = FSMC_AccessMode_A;

	  FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM2;
	  FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_PSRAM;
	  FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_16b;
	  FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	  FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	  FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	  FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	  FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
	  FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;
	  */
}

/* Тестирование аппаратной исправности внешней памяти
 * Глобальные переменные здесь нельзя использовать, данные могут быть размещены в тестируемой памяти.
 */
char stm32f2_ext_sram_test(void) {
	  char result = 0;
	  volatile uint8_t *ext_mem8 = (uint8_t *)MEMORY_B1_SRAM2_START_ADDRESS;
	  volatile uint16_t *ext_mem16 = (uint16_t *)ext_mem8;

	  for (volatile int i = 0; i < 16; i++) {
		  uint16_t data_val = (1 << i);
		  ext_mem16[0] = data_val;
		  if (ext_mem16[0] != data_val) goto test_end;
	  }

	  for (volatile int i = 0; i < MEMORY_B1_SRAM2_SIZE_WORDS; i++) {
		  ext_mem8[2*i] = 0xAA;
		  ext_mem8[2*i+1] = 0x55;
	  }
	  for (volatile int32_t Ai = -1; Ai < 19; Ai++) {
		  char scan_result = 0;
		  uint32_t cell = 1 << (uint32_t)Ai;
		  ext_mem16[cell] = 0;

#define ASM_INLINE_EXTMEM_SCAN_PROCEDURE(n) \
		"	movw r2, #21930					\n" /* r2 <- 0x55AA */ \
		"	mov %[result_val], #0			\n" /* scan_result = 0 */ \
		"	b test_extmem_"STR(n)"			\n" \
		"test_extmem_loop_"STR(n)":			\n" /* <loop> {... */ \
		"	ldrh r3, [r0], #2				\n" /* ext_mem16[i++] */ \
		"	cmp r3, r2						\n" /* ext_mem16[i] == 0x55aa ? */ \
		"	bne test_extmem_end_"STR(n)"	\n" /* false -> goto end */ \
		"test_extmem_"STR(n)":				\n" \
		"	cmp r0, r1						\n" /* i < cell ? */ \
		"	bcc test_extmem_loop_"STR(n)"	\n" /* ...} */ \
		"	mov %[result_val], #1			\n" /* scan_result = 1 */ \
		"test_extmem_end_"STR(n)":			\n"

		  /*
		   * for (volatile int i = 0; i < cell; i++)
		   * 	if (ext_mem16[i] != 0x55AA) goto test_end;
		  */
		  __asm volatile (
				  "	push {r0, r1, r2, r3}			\n"
				  "	mov r0, %[mem_addr]				\n" // r0 <- (ext_mem + 0)
				  "	mov r1, r0						\n" // r1 <- (ext_mem + 2*cell)
				  "	add r1, r1, %[cell_i], lsl #1	\n"
				  ASM_INLINE_EXTMEM_SCAN_PROCEDURE(1)
				  "	pop {r0, r1, r2, r3}			\n"
				  :[result_val]"=r"(scan_result)
				   :[cell_i]"r"(cell), [mem_addr]"r"(ext_mem16)
				    :"r0", "r1", "r2", "r3", "cc", "memory"
		  );
		  if (!scan_result) goto test_end;

		  /*
		   * for (volatile int i = (cell+1); i < MEMORY_B1_SRAM2_SIZE_WORDS; i++)
	 	   * 	if (ext_mem16[i] != 0x55AA) goto test_end;
		  */
		  __asm volatile (
				  "	push {r0, r1, r2, r3}			\n"
				  "	mov r0, %[mem_addr]				\n" // r0 <- (ext_mem + 2*(cell+1))
				  "	add r0, r0, %[cell_i], lsl #1	\n"
				  "	add r0, r0, #2					\n"
				  "	mov r1, %[mem_addr]				\n" // r1 <- (ext_mem + MEMORY_B1_SRAM2_SIZE_WORDS)
				  "	add r1, r1, #" STR(MEMORY_B1_SRAM2_SIZE_WORDS) "\n"
				  ASM_INLINE_EXTMEM_SCAN_PROCEDURE(2)
				  "	pop {r0, r1, r2, r3}			\n"
				  :[result_val]"=r"(scan_result)
				   :[cell_i]"r"(cell), [mem_addr]"r"(ext_mem16)
				    :"r0", "r1", "r2", "r3", "cc", "memory"
		  );
		  if (!scan_result) goto test_end;

		  if (ext_mem16[cell] != 0) goto test_end;

		  ext_mem16[cell] = 0x55AA;
	  }

	  result = 1;
test_end:
	  return result;
}

void stm32f2_LCD_init(void) {
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef  p;

	/*-- FSMC Configuration ------------------------------------------------------*/
	/*----------------------- SRAM Bank 1 ----------------------------------------*/
	/* FSMC_Bank1_NORSRAM1 configuration */
	p.FSMC_AddressSetupTime = 1;
	p.FSMC_AddressHoldTime = 0;
	p.FSMC_DataSetupTime = 6;
	p.FSMC_BusTurnAroundDuration = 1;
	p.FSMC_CLKDivision = 0;
	p.FSMC_DataLatency = 0;
	p.FSMC_AccessMode = FSMC_AccessMode_A;
	/* Color LCD configuration ------------------------------------
	     LCD configured as follow:
	        - Data/Address MUX = Disable
	        - Memory Type = SRAM
	        - Data Width = 8bit
	        - Write Operation = Enable
	        - Extended Mode = Disable
	        - Asynchronous Wait = Disable */

	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM1;
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
	FSMC_NORSRAMInitStructure.FSMC_MemoryDataWidth = FSMC_MemoryDataWidth_8b;
	FSMC_NORSRAMInitStructure.FSMC_BurstAccessMode = FSMC_BurstAccessMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_AsynchronousWait = FSMC_AsynchronousWait_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalPolarity = FSMC_WaitSignalPolarity_Low;
	FSMC_NORSRAMInitStructure.FSMC_WrapMode = FSMC_WrapMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignalActive = FSMC_WaitSignalActive_BeforeWaitState;
	FSMC_NORSRAMInitStructure.FSMC_WriteOperation = FSMC_WriteOperation_Enable;
	FSMC_NORSRAMInitStructure.FSMC_WaitSignal = FSMC_WaitSignal_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ExtendedMode = FSMC_ExtendedMode_Disable;
	FSMC_NORSRAMInitStructure.FSMC_WriteBurst = FSMC_WriteBurst_Disable;
	FSMC_NORSRAMInitStructure.FSMC_ReadWriteTimingStruct = &p;
	FSMC_NORSRAMInitStructure.FSMC_WriteTimingStruct = &p;

	portENTER_CRITICAL();
	/* Enable FSMC clock */
	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE);
	/* Configure FSMC NOR/SRAM */
	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);
	/* Enable FSMC NOR/SRAM Bank1 */
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1, ENABLE);
	portEXIT_CRITICAL();

	/* Configure LCD controller reset line and do reset */
	hal_gpio_pin_t lcd_reset_pin = {hgpioPB, 1};
	hal_gpio_params_t lcd_reset_params;
	hal_gpio_set_default_params(&lcd_reset_params);
	lcd_reset_params.mode = hgpioMode_Out;
	hal_gpio_init(lcd_reset_pin, &lcd_reset_params);
	hal_gpio_set_output(lcd_reset_pin, hgpioLow);
	hal_timer_delay(100);
	hal_gpio_set_output(lcd_reset_pin, hgpioHigh);
	hal_timer_delay(10);
}

void stm32f2_ext_pins_init(int platform_hw_resource) {
	hal_gpio_params_t params;
	hal_gpio_set_default_params(&params);
	switch (platform_hw_resource) {
	case platformhwHeadsetUart:
		params.mode = hgpioMode_AF;
		params.af = hgpioAF_USART_1_2_3;
		hal_gpio_init((hal_gpio_pin_t){hgpioPB, 10}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPB, 11}, &params);
		break;
	case platformhwHeadsetPttIopin:
		params.mode = hgpioMode_In;
		hal_gpio_init((hal_gpio_pin_t){hgpioPF, 8}, &params);
		break;
	case platformhwDataFlashSpi:
		params.mode = hgpioMode_AF;
		params.af = hgpioAF_SPI_3_I2S_3;
		hal_gpio_init((hal_gpio_pin_t){hgpioPC, 10}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPC, 11}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPC, 12}, &params);
		params.mode = hgpioMode_Out;
		params.af = hgpioAF_SYS;
		hal_gpio_init((hal_gpio_pin_t){hgpioPD, 2}, &params);
		break;
	case platformhwMatrixKeyboard:
		params.mode = hgpioMode_In;
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 2}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 3}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 4}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 5}, &params);
		params.mode = hgpioMode_Out;
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 6}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 7}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 8}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 9}, &params);
		break;
	case platformhwKeyboardButt1Iopin:
		params.mode = hgpioMode_In;
		params.exti_source = true;
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 11}, &params);
		break;
	case platformhwKeyboardButt2Iopin:
		params.mode = hgpioMode_In;
		params.exti_source = true;
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 12}, &params);
		break;
	case platformhwKeyboardsLightIopin:
		params.mode = hgpioMode_Out;
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 10}, &params);
		break;
	case platformhwEnRxRs232Iopin:
		params.mode = hgpioMode_Out;
		hal_gpio_init((hal_gpio_pin_t){hgpioPB, 13}, &params);
		break;
	case platformhwEnTxRs232Iopin:
		params.mode = hgpioMode_Out;
		hal_gpio_init((hal_gpio_pin_t){hgpioPB, 12}, &params);
		break;
	case platformhwDspUart:
		params.mode = hgpioMode_AF;
		params.af = hgpioAF_USART_1_2_3;
		hal_gpio_init((hal_gpio_pin_t){hgpioPB, 6}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPB, 7}, &params);
		break;
	case platformhwDspResetIopin:
		params.mode = hgpioMode_Out;
		hal_gpio_init((hal_gpio_pin_t){hgpioPH, 13}, &params);
		break;
	case platformhwAtuUart:
		params.mode = hgpioMode_AF;
		params.af = hgpioAF_USART_1_2_3;
		hal_gpio_init((hal_gpio_pin_t){hgpioPA, 2}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPA, 3}, &params);
		break;
	case platformhwBatterySmbusI2c:
		params.mode = hgpioMode_AF;
		params.af = hgpioAF_I2C_1_2_3;
		hal_gpio_init((hal_gpio_pin_t){hgpioPB, 8}, &params);
		hal_gpio_init((hal_gpio_pin_t){hgpioPB, 9}, &params);
		break;
	default: __asm volatile("bkpt"); // no such resource
	}
}

void stm32f2_ext_pins_deinit(int platform_hw_resource) {
	switch (platform_hw_resource) {
	case platformhwHeadsetUart:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPB, 10});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPB, 11});
		break;
	case platformhwHeadsetPttIopin:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPF, 8});
		break;
	case platformhwDataFlashSpi:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPC, 10});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPC, 11});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPC, 12});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPD, 2});
		break;
	case platformhwMatrixKeyboard:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 2});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 3});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 4});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 5});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 6});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 7});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 8});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 9});
		break;
	case platformhwKeyboardButt1Iopin:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 11});
		break;
	case platformhwKeyboardButt2Iopin:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 12});
		break;
	case platformhwKeyboardsLightIopin:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 10});
		break;
	case platformhwEnRxRs232Iopin:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPB, 13});
		break;
	case platformhwEnTxRs232Iopin:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPB, 12});
		break;
	case platformhwDspUart:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPB, 6});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPB, 7});
		break;
	case platformhwDspResetIopin:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPH, 13});
		break;
	case platformhwAtuUart:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPA, 2});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPA, 3});
		break;
	case platformhwBatterySmbusI2c:
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPB, 8});
		hal_gpio_deinit((hal_gpio_pin_t){hgpioPB, 9});
		break;
	default: __asm volatile("bkpt"); // no such resource
	}
}

hal_gpio_pin_t stm32f2_get_gpio_pin(int platform_hw_resource) {
	switch (platform_hw_resource) {
	case platformhwHeadsetPttIopin:
		return (hal_gpio_pin_t){hgpioPF, 8};
	case platformhwKeyboardButt1Iopin:
		return (hal_gpio_pin_t){hgpioPH, 11};
	case platformhwKeyboardButt2Iopin:
		return (hal_gpio_pin_t){hgpioPH, 12};
	case platformhwKeyboardsLightIopin:
		return (hal_gpio_pin_t){hgpioPH, 10};
	case platformhwEnRxRs232Iopin:
		return (hal_gpio_pin_t){hgpioPB, 13};
	case platformhwEnTxRs232Iopin:
		return (hal_gpio_pin_t){hgpioPB, 12};
	case platformhwDspResetIopin:
		return (hal_gpio_pin_t){hgpioPH, 13};
	default: __asm volatile("bkpt"); // no such resource
	}
	return (hal_gpio_pin_t){0, 0};
}

int stm32f2_get_exti_line(int platform_hw_resource) {
	switch (platform_hw_resource) {
	case platformhwHeadsetPttIopin:
		return 8;
	case platformhwKeyboardButt1Iopin:
		return 11;
	case platformhwKeyboardButt2Iopin:
		return 12;
	}
	return -1;
}
