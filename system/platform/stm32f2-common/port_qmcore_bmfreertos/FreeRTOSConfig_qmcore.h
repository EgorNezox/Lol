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

#define configUSE_PREEMPTION		1
#define configUSE_CO_ROUTINES 		0
#define configUSE_16_BIT_TICKS		0
#define configUSE_TRACE_FACILITY	0

#define configUSE_TIMERS	1
#define configTIMER_QUEUE_LENGTH	1

#define configUSE_MUTEXES				1
#define configUSE_COUNTING_SEMAPHORES 	1
#define configUSE_ALTERNATIVE_API 		0
#define configUSE_RECURSIVE_MUTEXES		1
#define INCLUDE_vTaskPrioritySet			1
#define INCLUDE_uxTaskPriorityGet			1
#define INCLUDE_vTaskDelete					1
#define INCLUDE_vTaskCleanUpResources		1
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay					1
#define INCLUDE_xTaskGetSchedulerState		1

#define configUSE_NEWLIB_REENTRANT	1

/* Task priorities scheme */
#define configMAX_PRIORITIES		( 6 )
#define configTIMER_TASK_PRIORITY	(tskIDLE_PRIORITY + 5)
#define usertaskSYSTEM_PRIORITY		(tskIDLE_PRIORITY + 4)
#define usertaskAPP_HIGH_PRIORITY	(tskIDLE_PRIORITY + 3)
#define usertaskAPP_NORMAL_PRIORITY	(tskIDLE_PRIORITY + 2)
#define usertaskAPP_LOW_PRIORITY	(tskIDLE_PRIORITY + 1)

#endif /* FREERTOSCONFIG_QM_H_ */
