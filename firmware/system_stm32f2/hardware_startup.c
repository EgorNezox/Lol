/**
  ******************************************************************************
  * @file    hardware_startup.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.08.2015
  * @brief   Файл низкоуровневого аппаратного запуска программы на целевой системе
  *
  ******************************************************************************
  */

#include <stdint.h>
#include "stm32f2xx.h"
#include "system_stm32f2xx.h"
#include "system_hw_io.h"

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
	SystemInit(); // перемещено из System_Startup_Entry
	stm32f2_ext_mem_init();
	if (!stm32f2_ext_sram_test())
		while(1); // memory test failed, cannot safely continue !
	__asm volatile("b System_Startup_Entry");
}
