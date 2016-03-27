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

#include "system.h"
#include "system_hw_io.h"
#include "sys_internal.h"

#define MPU_REGION_SIZE(n)	((n-1) << MPU_RASR_SIZE_Pos) // size = 2^n
#define SYSTEM_BOOT_ADDRESS	0x1fff0000

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
	halinternal_spi_init();
	/* Общая инициализация ввода/вывода аппаратной платформы системы */
	stm32f2_hardware_io_init();
}

void halinternal_set_nvic_priority(IRQn_Type irqn) {
	NVIC_SetPriority(irqn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), SYS_IRQ_CHANNEL_PREEMPTION_PRIORITY, 0));
}

bool halinternal_is_isr_active(void) {
	return (__get_IPSR() != 0);
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

void stm32f2_enter_bootloader() {
	/* Firstly, disable and clear pending interrupts from all mcu peripherals */
	for (int i = 0; i <= 80; i++) {
		NVIC_DisableIRQ((IRQn_Type)i);
		NVIC_ClearPendingIRQ((IRQn_Type)i);
	}
	/* Set system clock from HSI oscillator (and reset prescalers) */
	RCC->CFGR &= ~RCC_CFGR_SW;
	RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1  | RCC_CFGR_PPRE2);
	while ((RCC->CFGR & RCC_CFGR_SWS ) != RCC_CFGR_SWS_HSI);
	/* Reset access latency of flash interface to zero CPU cycles */
	FLASH->ACR &= ~FLASH_ACR_LATENCY;
	/* Reset HSE and main PLL */
	RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_PLLON);
	/* Switch execution context to internal SRAM
	 * It "detaches" stack from external memory, if any.
	 * Warning: no local variables can be used since here !
	 */
	__asm volatile("mov sp, %0\n" : : "r" (0x20000000+256) : "sp");
	/* Reset all peripheral clocks
	 * Warning: breaks access to external memory (if any) since here !
	 */
	RCC->AHB1ENR = 0;
	RCC->AHB2ENR = 0;
	RCC->AHB3ENR = 0;
	RCC->APB1ENR = 0;
	RCC->APB2ENR = 0;
	/* Prepare system boot state and jump to bootloader code */
	SYSCFG->MEMRMP = SYSCFG_MEMRMP_MEM_MODE_0;
	SCB->VTOR = SYSTEM_BOOT_ADDRESS;
	__asm volatile("mov sp, %0\n" : : "r" (*(uint32_t *)SYSTEM_BOOT_ADDRESS) : "sp");
	__asm volatile("bx %0" :: "r" (*(uint32_t *)(SYSTEM_BOOT_ADDRESS + 4)));
}
