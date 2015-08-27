/**
  ******************************************************************************
  * @file    system_hw_ext_devices.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    18.08.2015
  * @brief   Файл общей конфигурации внешних устройств и интерфейсов микроконтроллера STM32F2
  *
  * Содержит функцию инициализации GPIO и внешней SRAM памяти.
  * Глобальные переменные здесь нельзя использоваться, т.к. инициализация памяти может быть
  * еще не выполнена, а если данные размещаются во внешней SRAM, то доступ к ней еще не сконфигурирован.
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"
#include "system_hw_map.h"

#define _STR(arg) #arg
#define STR(arg) _STR(arg)

/*! Первичная иницициализация внешней памяти.
 *
 *  External SRAM:	Bank1_SRAM2		(A[19:0], D[15:0], NBL[0], NBL[1], NOE, NWE, NE2)
 *  LCD:			Bank1_NORSRAM3	(A[0], D[15:0], NOE, NWE, NE3)
 *
 *  Код функции предполагает, что GPIO-регистры содержат значения по умолчанию (сразу после сброса МК) !
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

/*-- FSMC Configuration for external SRAM ------------------------------------------------------*/
	  /* Enable the FSMC interface clock */
	  RCC->AHB3ENR        |= 0x00000001;

	  /* Configure and enable Bank1_SRAM2 */
	  FSMC_Bank1->BTCR[2]  = 0x00001015;
	  FSMC_Bank1->BTCR[3]  = 0x00010400;
	  FSMC_Bank1E->BWTR[2] = 0x0fffffff;

	  /*
	  Bank1_SRAM2 is configured as follow:

	  p.FSMC_AddressSetupTime = 0;
	  p.FSMC_AddressHoldTime = 0;
	  p.FSMC_DataSetupTime = 4;
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


void stm32f2_LCD_init(void) {
	FSMC_NORSRAMInitTypeDef  FSMC_NORSRAMInitStructure;
	FSMC_NORSRAMTimingInitTypeDef  p;

	/* Enable FSMC clock */
	RCC_AHB3PeriphClockCmd(RCC_AHB3Periph_FSMC, ENABLE);

	/*-- FSMC Configuration ------------------------------------------------------*/
	/*----------------------- SRAM Bank 3 ----------------------------------------*/
	/* FSMC_Bank1_NORSRAM3 configuration */
	p.FSMC_AddressSetupTime = 2;
	p.FSMC_AddressHoldTime = 0;
	p.FSMC_DataSetupTime = 9;
	p.FSMC_BusTurnAroundDuration = 0;
	p.FSMC_CLKDivision = 0;
	p.FSMC_DataLatency = 0;
	p.FSMC_AccessMode = FSMC_AccessMode_A;
	/* Color LCD configuration ------------------------------------
	     LCD configured as follow:
	        - Data/Address MUX = Disable
	        - Memory Type = SRAM
	        - Data Width = 16bit
	        - Write Operation = Enable
	        - Extended Mode = Enable
	        - Asynchronous Wait = Disable */

	FSMC_NORSRAMInitStructure.FSMC_Bank = FSMC_Bank1_NORSRAM3;
	FSMC_NORSRAMInitStructure.FSMC_DataAddressMux = FSMC_DataAddressMux_Disable;
	FSMC_NORSRAMInitStructure.FSMC_MemoryType = FSMC_MemoryType_SRAM;
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

	FSMC_NORSRAMInit(&FSMC_NORSRAMInitStructure);

	/* Enable FSMC NOR/SRAM Bank3 */
	FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM3, ENABLE);
}


char stm32f2_ext_sram_test(void) {
	return 1; /* not implemented */
}
