/**
  ******************************************************************************
  * @file    system.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.08.2015
  * @brief   Реализация общей системы аппаратной абстракции
  *
  ******************************************************************************
 */

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f2xx.h"

#include "sys_internal.h"
#include "sys_internal_freertos_timers.h"

#define MPU_REGION_SIZE(n)	((n-1) << MPU_RASR_SIZE_Pos) // size = 2^n

unsigned int halinternal_freertos_timer_queue_length = 0;

/*! Автоматическая инициализация средствами стандартной библиотеки (до входа в main) */
void  __attribute__((constructor)) hal_system_init(void) {
	/* Настройка MPU для детектирования обращений по адресу NULL (+ 32KB) */
	MPU->CTRL = 0;
	MPU->RBAR = 0x00000000 | MPU_RBAR_VALID_Msk | 0;
	MPU->RASR = MPU_REGION_SIZE(15) | MPU_RASR_ENABLE_Msk | MPU_RASR_XN_Msk;
	MPU->CTRL |= MPU_CTRL_ENABLE_Msk | MPU_CTRL_HFNMIENA_Msk | MPU_CTRL_PRIVDEFENA_Msk;
	/* Включение процессорных исключений Usage Fault, Bus Fault, MMU Fault */
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;
	/* Настройка значений приоритетов исключений/прерываний как pre-emption (без sub-priority) */
	NVIC_SetPriorityGrouping(max(7 - __NVIC_PRIO_BITS, 0));
	/* Инициализация поддержки отладочного вывода SWO в МК */
	DBGMCU->CR |= DBGMCU_CR_TRACE_IOEN;
	/* Инициализация всех субмодулей */
	halinternal_gpio_init();
	halinternal_exti_init();
	halinternal_timer_init();
	halinternal_uart_init();
	halinternal_i2c_init();
}

void halinternal_set_nvic_priority(IRQn_Type irqn) {
	NVIC_SetPriority(irqn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), SYS_IRQ_CHANNEL_PREEMPTION_PRIORITY, 0));
}

void halinternal_system_fault_handler(void) {
#ifndef NDEBUG
	__BKPT(0); // отладка: выйти из этой функции (step return) и см. причину выше по стеку вызовов
#else
	// TODO: обработать сбой регистрируемой функцией приложения
	NVIC_SystemReset();
#endif
}

#ifndef __APCS_32__
#error "Exception handling functions assume AAPCS-compliant compiler"
#endif
#define ASM_HANDLE_EXCEPTION(exc_type)	\
		__asm volatile					\
		(								\
			" mov r0, %[exc]											\n"	\
			" tst lr, #4												\n"	\
			" ite eq													\n"	\
			" mrseq r1, msp												\n"	\
			" mrsne r1, psp												\n"	\
			" add r2, r1, #32											\n"	\
			" push {lr}													\n"	\
			" push {r4-r11}												\n" \
			" mov r3, sp												\n"	\
			" ldr r4, " #exc_type "_addr_const							\n"	\
			" blx r4													\n"	\
			" pop {r4-r11}												\n" \
			" pop {pc}													\n"	\
			" " #exc_type "_addr_const: .word CPUExceptionHandler	\n"	\
			:: [exc]"i" (exc_type)	\
		);
typedef enum {excMemManage, excBusFault, excUsageFault, excHardFault} exc_type;
typedef struct {
	unsigned int r0, r1, r2, r3, r12, lr, pc, psr;
} exc_stacked_regs_t;
typedef struct {
	unsigned int r4, r5, r6, r7, r8, r9, r10, r11;
} exc_other_regs_t;
void __attribute__((naked)) MemManage_Handler(void) {
	ASM_HANDLE_EXCEPTION(excMemManage)
}
void __attribute__((naked)) BusFault_Handler(void) {
	ASM_HANDLE_EXCEPTION(excBusFault)
}
void __attribute__((naked)) UsageFault_Handler(void) {
	ASM_HANDLE_EXCEPTION(excUsageFault)
}
void __attribute__((naked)) HardFault_Handler(void) {
	ASM_HANDLE_EXCEPTION(excHardFault)
}

static __attribute__ ((used)) void CPUExceptionHandler(exc_type exception, exc_stacked_regs_t *exc_stacked_regs, void *exc_sp, exc_other_regs_t *exc_other_regs) {
	/* Системное исключение процессора (см. аргументы функции и документацию на архитектуру ARMv7)
	 * Возможные причины:
	 *  - запрещенное обращение к памяти (множество вариаций, например, выход за границы массива, обращение по невалидному указателю...);
	 *  - ошибка при обработке прерываний и/или конфигурации FreeRTOS;
	 *  - переполнение стека в задаче FreeRTOS (имя задачи находится в pxCurrentTCB->pcTaskName);
	 *  - другие трудно-выявляемые ошибки на системном уровне.
	 */
	halinternal_system_fault_handler(); // здесь можно выполнить команду rollback_exception в консоли gdb (при поддержке .gdbinit скрипта)
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
	/* Переполнение стека в задаче pcTaskName (FreeRTOS).
	 * (Размер стека задач потоков приложения определяется макросом qmconfigAPP_STACK_SIZE конфигурации FreeRTOS.)
	 */
	halinternal_system_fault_handler();
}

void vApplicationMallocFailedHook(void) {
	/* Сбой динамического выделения памяти (malloc()/calloc()/realloc() возвращает NULL)
	 * Возможные причины:
	 *  - закончилось свободное место в куче (размер определяется макросом configTOTAL_HEAP_SIZE в конфигурации FreeRTOS);
	 *  - фрагментация памяти;
	 *  - ошибка при вызове функции выделения памяти (запрашиваемый размер больше размера кучи, ...).
	 */
	halinternal_system_fault_handler();
}
