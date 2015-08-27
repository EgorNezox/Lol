/**
  ******************************************************************************
  * @file    irq_handlers_extended.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    24.02.2015
  * @brief   Расширенные обработчики прерываний STM32F2
  *
  * Некоторые источники прерываний микроконтроллера объединены в одно прерывание.
  * Данное решение программно демультиплексирует совмещенные обработчики в отдельные обработчики,
  * которые могут быть использованы в проекте так же как и обычные обработчики.
  * Решение максимально близко эмулирует модель прерываний ARMv7-M.
  *
  * EXTI9_5_IRQHandler: EXTI<x>_IRQHandler (x = 5..9)
  * EXTI15_10_IRQHandler: EXTI<x>_IRQHandler (x = 10..15)
  *
  ******************************************************************************
 */

#include <stdbool.h>
#include "stm32f2xx.h"

#define WEAK_SYMBOL	__attribute__ ((weak))

typedef struct {
	uint32_t line;
	void (*handler)(void);
	bool mask;
	bool pending;
} irq_exti_sub_descr_t;

void WEAK_SYMBOL EXTI5_IRQHandler(void) {}
void WEAK_SYMBOL EXTI6_IRQHandler(void) {}
void WEAK_SYMBOL EXTI7_IRQHandler(void) {}
void WEAK_SYMBOL EXTI8_IRQHandler(void) {}
void WEAK_SYMBOL EXTI9_IRQHandler(void) {}
void WEAK_SYMBOL EXTI10_IRQHandler(void) {}
void WEAK_SYMBOL EXTI11_IRQHandler(void) {}
void WEAK_SYMBOL EXTI12_IRQHandler(void) {}
void WEAK_SYMBOL EXTI13_IRQHandler(void) {}
void WEAK_SYMBOL EXTI14_IRQHandler(void) {}
void WEAK_SYMBOL EXTI15_IRQHandler(void) {}

void irq_exti_process_sub_interrupts(irq_exti_sub_descr_t descriptors[], int count) {
	/* Assign pending statuses (detection) */
	__disable_irq();
	for (int i = 0; i < count; i++) {
		descriptors[i].mask = EXTI->IMR & descriptors[i].line;
		descriptors[i].pending = (descriptors[i].mask && (EXTI->PR & descriptors[i].line));
	}
	__enable_irq();
	/* Enter active states */
	for (int i = 0; i < count; i++)
		if (descriptors[i].pending)
			descriptors[i].handler();
	/* Check if interrupt masks was disabled, and clear pending bits
	 * (otherwise interrupt will remaing pending and detection will ignore it, causing processor being stuck in this handler !)
	 */
	for (int i = 0; i < count; i++)
		if (descriptors[i].mask && !(EXTI->IMR & descriptors[i].line))
			EXTI->PR = descriptors[i].line;
	// End with NVIC hardware behavior...
}

void EXTI9_5_IRQHandler(void) {
	static irq_exti_sub_descr_t descriptors[] = {
			{0x00020, EXTI5_IRQHandler, false, false},
			{0x00040, EXTI6_IRQHandler, false, false},
			{0x00080, EXTI7_IRQHandler, false, false},
			{0x00100, EXTI8_IRQHandler, false, false},
			{0x00200, EXTI9_IRQHandler, false, false},
	};
	irq_exti_process_sub_interrupts(descriptors, sizeof(descriptors)/sizeof(descriptors[0]));
}
void EXTI15_10_IRQHandler(void) {
	static irq_exti_sub_descr_t descriptors[] = {
			{0x00400, EXTI10_IRQHandler, false, false},
			{0x00800, EXTI11_IRQHandler, false, false},
			{0x01000, EXTI12_IRQHandler, false, false},
			{0x02000, EXTI13_IRQHandler, false, false},
			{0x04000, EXTI14_IRQHandler, false, false},
			{0x08000, EXTI15_IRQHandler, false, false},
	};
	irq_exti_process_sub_interrupts(descriptors, sizeof(descriptors)/sizeof(descriptors[0]));
}
