/**
  ******************************************************************************
  * @file    timer.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    03.09.2015
  * @brief   Реализация аппаратной абстракции доступа к таймеру на STM32F2xxx средствами FreeRTOS и процессорного таймера SysTick (Cortex-M)
  *
  * Ограничения:
  * - после запуска FreeRTOS нельзя использовать функции таймера в критических секциях и при остановленном планировщике FreeRTOS;
  * - не поддерживается одновременное использование функций таймера из основного контекста и контекста прерываний;
  * Таймер автоматически выбирает средство отсчета из контекста вызова,
  * предполагая сценарий работы системы, состоящий из двух последовательных фаз:
  * - SysTick: работа до запуска планировщика FreeRTOS, один контекст вызова, SysTick нигде в системе не используется;
  * - FreeRTOS ticks: работа после запуска планировщика FreeRTOS, обратного возврата нет.
  * Максимальное время отсчета SysTick ограничено частотой системного клока
  * (для макс. частоты микроконтроллера в 120 МГЦ величина составляет чуть более 1000 мс).
  * Максимальное время отсчета FreeRTOS ticks ограничено величиной счетчика тиков FreeRTOS.
  * Точность отсчета: для SysTick равна 1/(SysClock/8), для FreeRTOS ticks равна 1/configTICK_RATE_HZ.
  * SysTick тактируется от внешнего клока (HCLK/8), предоставляемого микроконтроллером.
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"

#include "sys_internal.h"
#include "hal_timer.h"

static uint32_t timer_get_maximum_systick_delay_ms(void) {
	return ((float)SysTick_LOAD_RELOAD_Msk/(SystemCoreClock/8))*1000;
}

static void timer_start_systick(unsigned int ms) {
	SysTick->CTRL = 0;
	SysTick->LOAD = ((float)ms/1000)*(SystemCoreClock/8);
	SysTick->VAL = 0;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

static char timer_check_systick_expired(unsigned int *elapsed_ms) {
	if (elapsed_ms) *elapsed_ms = ((float)(SysTick->LOAD - SysTick->VAL)/(SystemCoreClock/8))*1000;
	return ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0);
}

static void timer_stop_systick(void) {
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

void halinternal_timer_init(void) {
	SYS_ASSERT((SysTick->CTRL & SysTick_CTRL_CLKSOURCE_Msk) == 0);
	SYS_ASSERT(configTICK_RATE_HZ <= 1000);
}

void hal_timer_delay(unsigned int ms) {
	SYS_ASSERT(ms > 0);
	if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
		uint32_t max_interval = timer_get_maximum_systick_delay_ms();
		while (ms > 0) {
			if (ms < max_interval) {
				timer_start_systick(ms);
				ms = 0;
			} else {
				timer_start_systick(max_interval);
				ms -= max_interval;
			}
			while (!timer_check_systick_expired(NULL));
			timer_stop_systick();
		}
	} else {
		vTaskDelay(ms/portTICK_PERIOD_MS);
	}
}

hal_timer_t hal_timer_start(unsigned int ms) {
	hal_timer_t tmr;
	if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
		if (ms > 0) {
			SYS_ASSERT(ms <= timer_get_maximum_systick_delay_ms());
		} else {
			ms = timer_get_maximum_systick_delay_ms();
		}
		tmr.type = htimerSysTick;
		timer_start_systick(ms);
	} else {
		if (ms == 0) ms = portMAX_DELAY - 1;
		tmr.type = htimerFreeRTOS;
		tmr.timestamp_start = xTaskGetTickCount();
		tmr.timestamp_end = tmr.timestamp_start + ms/portTICK_PERIOD_MS;
		tmr.timestamp_ovf = (tmr.timestamp_start > tmr.timestamp_end);
	}
	return tmr;
}

char hal_timer_check_expired(hal_timer_t timer, unsigned int *elapsed_ms) {
	char expired;
	if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
		SYS_ASSERT(timer.type == htimerSysTick);
		expired = timer_check_systick_expired(elapsed_ms);
		if (expired) timer_stop_systick();
	} else {
		TickType_t current_timestamp = xTaskGetTickCount();
		SYS_ASSERT(timer.type == htimerFreeRTOS);
		if (elapsed_ms) *elapsed_ms = (current_timestamp - timer.timestamp_start)*portTICK_PERIOD_MS;
		if (!timer.timestamp_ovf) {
			expired = !((timer.timestamp_start <= current_timestamp) && (current_timestamp < timer.timestamp_end));
		} else {
			expired = ((timer.timestamp_end <= current_timestamp) && (current_timestamp < timer.timestamp_start));
		}
	}
	return expired;
}
