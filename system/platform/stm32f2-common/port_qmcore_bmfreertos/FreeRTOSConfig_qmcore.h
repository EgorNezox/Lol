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
#define configUSE_16_BIT_TICKS		0

/* Miscellaneous */
#if !defined(configUSE_NEWLIB_REENTRANT) || (configUSE_NEWLIB_REENTRANT != 1)
#ifdef configUSE_NEWLIB_REENTRANT
#undef configUSE_NEWLIB_REENTRANT
#endif
#define configUSE_NEWLIB_REENTRANT		1
#endif

/* Task priorities scheme */
#define configMAX_PRIORITIES			( 6 )
#define sysconfigTIMER_TASK_PRIORITY	(tskIDLE_PRIORITY + 5) // system/HAL timers thread (must be highest to prevent races)
#define qmconfigSYSTEM_PRIORITY			(tskIDLE_PRIORITY + 4) // system/core thread
#define qmconfigAPP_HIGH_PRIORITY		(tskIDLE_PRIORITY + 3) // application threads (high)
#define qmconfigAPP_NORMAL_PRIORITY		(tskIDLE_PRIORITY + 2) // application threads (default)
#define qmconfigAPP_LOW_PRIORITY		(tskIDLE_PRIORITY + 1) // application threads (low)

/* System/core specifics */
#define qmconfigSYSTEM_STACK_SIZE		( 2048 )
#define sysconfigTIMER_TASK_STACK_SIZE	qmconfigSYSTEM_STACK_SIZE

#endif /* FREERTOSCONFIG_QM_H_ */
