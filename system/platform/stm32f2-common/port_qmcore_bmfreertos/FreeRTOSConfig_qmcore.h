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

/* Software timers */
#if !defined(configUSE_TIMERS) || (configUSE_TIMERS != 1)
#ifdef configUSE_TIMERS
#undef configUSE_TIMERS
#endif
#define configUSE_TIMERS				1 // used by QmTimer implementation
#endif
#if !defined(configTIMER_QUEUE_LENGTH)
#ifdef configTIMER_QUEUE_LENGTH
#undef configTIMER_QUEUE_LENGTH
#endif
#define configTIMER_QUEUE_LENGTH	(halinternal_freertos_timer_queue_length + 1) // timer queue used by HAL + pure software timers used by Qm
#endif

/* Miscellaneous */
#if !defined(configUSE_NEWLIB_REENTRANT) || (configUSE_NEWLIB_REENTRANT != 1)
#ifdef configUSE_NEWLIB_REENTRANT
#undef configUSE_NEWLIB_REENTRANT
#endif
#define configUSE_NEWLIB_REENTRANT		1
#endif

/* Qm task priorities scheme */
#define configMAX_PRIORITIES		( 5 )
#define configTIMER_TASK_PRIORITY	qmconfigSYSTEM_PRIORITY // timer expirations handled at system priority
#define qmconfigSYSTEM_PRIORITY		(tskIDLE_PRIORITY + 4) // system/core thread
#define qmconfigAPP_HIGH_PRIORITY	(tskIDLE_PRIORITY + 3) // application threads (high)
#define qmconfigAPP_NORMAL_PRIORITY	(tskIDLE_PRIORITY + 2) // application threads (default)
#define qmconfigAPP_LOW_PRIORITY	(tskIDLE_PRIORITY + 1) // application threads (low)

/* Qm system/core specifics */
#define qmconfigSYSTEM_STACK_SIZE	( 2048 )

#endif /* FREERTOSCONFIG_QM_H_ */
