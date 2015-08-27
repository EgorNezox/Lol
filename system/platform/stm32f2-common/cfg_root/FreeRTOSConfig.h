/*
    FreeRTOS V8.2.2 - Copyright (C) 2015 Real Time Engineers Ltd.
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f2xx.h"

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *----------------------------------------------------------*/

#define configUSE_IDLE_HOOK			0
#define configUSE_TICK_HOOK			0
#define configCPU_CLOCK_HZ			( SystemCoreClock )
#define configTICK_RATE_HZ			( ( TickType_t ) 1000 )
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 256 )
#define configMAX_TASK_NAME_LEN		( 64 )
#define configIDLE_SHOULD_YIELD		0

/* Software timers */
#define configTIMER_TASK_STACK_DEPTH	(configMINIMAL_STACK_SIZE)

#define configQUEUE_REGISTRY_SIZE		50
#define configGENERATE_RUN_TIME_STATS	0
#define configUSE_MALLOC_FAILED_HOOK	1
#define configCHECK_FOR_STACK_OVERFLOW  2

/* The lowest priority. */
#define configKERNEL_INTERRUPT_PRIORITY 	255
/* Priorities 1-15 available for interrupts making system calls.
 * In other words, there are no special hard real-time ISRs in the application.
 */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY 	(1 << (8 - __NVIC_PRIO_BITS))

#ifdef ZEON_DEBUG_ENABLE
#define configASSERT(x)	if (!(x)) __asm volatile("bkpt");
#endif

/* Activate gdb debug helpers hook (see port files and .gdbinit scripts) */
#define xPortPendSVHandler	xPortPendSVHandler_native

#include "FreeRTOSConfig_qmcore.h"
#include "FreeRTOSConfig_app.h"

#endif /* FREERTOS_CONFIG_H */
