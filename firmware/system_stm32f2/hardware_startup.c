/**
  ******************************************************************************
  * @file    hardware_startup.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.08.2015
  * @brief   Файл низкоуровневого аппаратного запуска программы на целевой системе
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"
#include "system_stm32f2xx.h"
#include "system_hw_memory.h"

/*! Обработчик системного сброса (точка входа в загрузчик)
 *
 * Выполняется при сбросе/включении устройства и ответственен за запуск инициализации устройства.
 * Внимание: память на данном этапе не инициализирована, в данном контексте нельзя обращаться к глобальным переменным !
 */
void Reset_Handler(void) {
	SystemInit(); // перемещено из System_Startup_Entry
	stm32f2_ext_mem_init();
	if (!stm32f2_ext_sram_test())
		while(1); // memory test failed, cannot safely continue !
	__asm volatile("b System_Startup_Entry");
}
