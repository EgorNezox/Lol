/**
  ******************************************************************************
  * @file    FreeRTOSConfig_qmcore.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    08.09.2015
  * @brief   Конфигурация FreeRTOS для Qm
  *
  ******************************************************************************
  */

#ifndef FREERTOSCONFIG_QM_H_
#define FREERTOSCONFIG_QM_H_

/* Primary kernel configuration */
#define configUSE_PREEMPTION		1
#define configUSE_CO_ROUTINES 		0
#define configUSE_16_BIT_TICKS		0
#define configUSE_TRACE_FACILITY	0
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 256 )

/* Software timers */
#define configUSE_TIMERS				1 // used by QmTimer implementation
#define configTIMER_QUEUE_LENGTH		1 // pure software timers, no *FromISR* calls allowed
#define configTIMER_TASK_STACK_DEPTH	configMINIMAL_STACK_SIZE // assumed only small QmTimerPrivate::callback() stack

/* Miscellaneous configuration */
#define configUSE_NEWLIB_REENTRANT		1
#define configUSE_MUTEXES				1
#define configUSE_COUNTING_SEMAPHORES 	1
#define configUSE_ALTERNATIVE_API 		0
#define configUSE_RECURSIVE_MUTEXES		1
#define INCLUDE_vTaskPrioritySet		1
#define INCLUDE_uxTaskPriorityGet		1
#define INCLUDE_vTaskDelete				1
#define INCLUDE_vTaskCleanUpResources	1
#define INCLUDE_vTaskSuspend			1
#define INCLUDE_vTaskDelayUntil			1
#define INCLUDE_vTaskDelay				1
#define INCLUDE_xTaskGetSchedulerState	1

/* Qm task priorities scheme */
#define configMAX_PRIORITIES		( 5 )
#define configTIMER_TASK_PRIORITY	qmconfigSYSTEM_PRIORITY // timer expirations handled at system priority
#define qmconfigSYSTEM_PRIORITY		(tskIDLE_PRIORITY + 4) // system/core thread
#define qmconfigAPP_HIGH_PRIORITY	(tskIDLE_PRIORITY + 3) // application threads (high)
#define qmconfigAPP_NORMAL_PRIORITY	(tskIDLE_PRIORITY + 2) // application threads (default)
#define qmconfigAPP_LOW_PRIORITY	(tskIDLE_PRIORITY + 1) // application threads (low)

/* Qm system/core specifics */
#define qmconfigSYSTEM_STACK_SIZE	( ( unsigned short ) 2048 )

#endif /* FREERTOSCONFIG_QM_H_ */
