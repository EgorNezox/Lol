/**
  ******************************************************************************
  * @file    FreeRTOSConfig.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    13.11.2015
  * @brief   Корневой конфигурационный файл FreeRTOS
  *
  ******************************************************************************
  */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f2xx.h"
#include "../system_hardware/sys_internal_freertos_timers.h"

/* Primary kernel configuration */
#define configUSE_IDLE_HOOK			0
#define configUSE_TICK_HOOK			0
#define configCPU_CLOCK_HZ			( SystemCoreClock ) // CMSIS
#define configTICK_RATE_HZ			( 1000 )
#define configMAX_TASK_NAME_LEN		( 64 )
#define configUSE_NEWLIB_REENTRANT	1 // HAL modules use stdlib and designed to be reentrant
#define configMINIMAL_STACK_SIZE	( 256 )

/* Debugging */
#ifndef NDEBUG
#define configASSERT(x)	if (!(x)) __asm volatile("bkpt");
#endif
#define configUSE_TRACE_FACILITY		1
#define configUSE_MALLOC_FAILED_HOOK	1
#define configCHECK_FOR_STACK_OVERFLOW  2
#define xPortPendSVHandler	xPortPendSVHandler_native // activates gdb debug helpers hook (see port files and scripts)

/* Software timers */
#define configUSE_TIMERS				1 // used by HAL
#define configTIMER_TASK_STACK_DEPTH	configMINIMAL_STACK_SIZE

/* Processor/port specific */
/* The lowest priority. */
#define configKERNEL_INTERRUPT_PRIORITY 	255
/* Priorities 1-15 available for interrupts making system calls.
 * In other words, there are no special hard real-time ISRs in the application.
 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	(1 << (8 - __NVIC_PRIO_BITS))

/* Miscellaneous */
#define configUSE_MUTEXES				1
#define configUSE_COUNTING_SEMAPHORES 	1
#define configUSE_RECURSIVE_MUTEXES		1
#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	1
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1
#define INCLUDE_xTaskGetSchedulerState	1

#include "FreeRTOSConfig_qmcore.h"
#include "FreeRTOSConfig_app.h"

#endif /* FREERTOS_CONFIG_H */
