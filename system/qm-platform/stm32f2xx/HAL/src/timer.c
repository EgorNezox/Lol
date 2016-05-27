/**
  ******************************************************************************
  * @file    timer.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    03.09.2015
  * @brief   Реализация аппаратной абстракции доступа к таймеру на STM32F2xxx средствами FreeRTOS и процессорного таймера SysTick (Cortex-M)
  *
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

#include <stdlib.h>
#include "stm32f2xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "dl_list.h"

#include "sys_internal.h"
#include "hal_timer.h"

#ifndef sysconfigTIMER_TASK_PRIORITY
#define sysconfigTIMER_TASK_PRIORITY	tskIDLE_PRIORITY
#endif
#ifndef sysconfigTIMER_TASK_STACK_SIZE
#define sysconfigTIMER_TASK_STACK_SIZE	configMINIMAL_STACK_SIZE
#endif

#define DEFINE_PCB_FROM_HANDLE(var_pcb, handle) \
	struct s_timer_pcb *var_pcb = (struct s_timer_pcb *)handle; \
	SYS_ASSERT(var_pcb != 0);

struct s_timer_pcb;
DLLIST_TYPEDEF_LIST(struct s_timer_pcb, active)
DLLIST_TYPEDEF_LIST(struct s_timer_pcb, pending)

struct s_timer_pcb {
	DLLIST_ELEMENT_FIELDS(struct s_timer_pcb, active)
	DLLIST_ELEMENT_FIELDS(struct s_timer_pcb, pending)
	enum {
		stateIdle,
		stateActiveSystick,
		stateActiveFreertos
	} state;
	TickType_t timestamp_start;
	TickType_t timestamp_end;
	bool timestamp_overflow;
	void *userid;
	void (*callbackTimeout)(hal_timer_handle_t handle);
};

static void timer_reset(struct s_timer_pcb *timer, BaseType_t scheduler_state);
static portTASK_FUNCTION(timer_task_function, pvParameters);
static void timer_remove_from_lists(struct s_timer_pcb *timer);
static void timer_process_pending_list(void);
static TickType_t timer_process_expired(void);
static bool timer_timeout_is_later(struct s_timer_pcb *timer1, struct s_timer_pcb *timer2);
static void timer_update_current_pending(struct s_timer_pcb *timer);
static void timer_update_active_list_iterator(struct s_timer_pcb *timer);
static uint32_t timer_systick_get_maximum_delay_ms(void);
static void timer_systick_start(unsigned int ms);
static void timer_systick_stop(void);
static bool timer_systick_check_expired(void);
static void timer_freertos_start(struct s_timer_pcb *timer, TickType_t timestamp_start, unsigned int ms);
static void timer_freertos_sync_task(signed portBASE_TYPE *pxHigherPriorityTaskWoken);
static bool timer_freertos_check_expired(struct s_timer_pcb *timer, TickType_t current_timestamp);

static DLLIST_LIST_TYPE(active) timer_active_list;
static DLLIST_LIST_TYPE(pending) timer_pending_list;
static SemaphoreHandle_t timer_task_semphr = 0;
struct s_timer_pcb *timer_current_pending = 0;
struct s_timer_pcb *timer_current_active = 0;

void halinternal_timer_init(void) {
	SysTick->CTRL = 0;
	SYS_ASSERT((SysTick->CTRL & SysTick_CTRL_CLKSOURCE_Msk) == 0);
	SYS_ASSERT(configTICK_RATE_HZ <= 1000);
	xTaskCreate(timer_task_function, "hal_timer", sysconfigTIMER_TASK_STACK_SIZE, 0, sysconfigTIMER_TASK_PRIORITY, 0);
	timer_task_semphr = xSemaphoreCreateBinary();
	DLLIST_INIT_LIST(&timer_active_list);
	DLLIST_INIT_LIST(&timer_pending_list);
}

void hal_timer_delay(unsigned int ms) {
	if (ms == 0)
		return;
	if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
		uint32_t max_interval = timer_systick_get_maximum_delay_ms();
		while (ms > 0) {
			uint32_t interval = min(ms, max_interval);
			timer_systick_start(interval);
			while (!timer_systick_check_expired());
			timer_systick_stop();
			ms -= interval;
		}
	} else {
		unsigned int total_ticks = ms/portTICK_PERIOD_MS;
		while (total_ticks > 0) {
			unsigned int ticks = min(total_ticks, (portMAX_DELAY - 1));
			vTaskDelay((portTickType)ticks);
			total_ticks -= ticks;
		}
	}
}

hal_timer_handle_t hal_timer_create(hal_timer_params_t *params) {
	struct s_timer_pcb *timer = malloc(sizeof(struct s_timer_pcb));
	DLLIST_INIT_ELEMENT(active, timer);
	DLLIST_INIT_ELEMENT(pending, timer);
	timer->state = stateIdle;
	timer->timestamp_start = 0;
	timer->timestamp_end = 0;
	timer->timestamp_overflow = false;
	if (params) {
		timer->userid = params->userid;
		timer->callbackTimeout = params->callbackTimeout;
	} else {
		timer->userid = 0;
		timer->callbackTimeout = 0;
	}
	return (hal_timer_handle_t)timer;
}

void hal_timer_delete(hal_timer_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(timer, handle)
	BaseType_t scheduler_state = xTaskGetSchedulerState();
	portDISABLE_INTERRUPTS();
	timer_reset(timer, scheduler_state);
	portENABLE_INTERRUPTS();
	free(timer);
}

void hal_timer_start(hal_timer_handle_t handle, unsigned int ms, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	DEFINE_PCB_FROM_HANDLE(timer, handle)
	BaseType_t scheduler_state = xTaskGetSchedulerState();
	TickType_t current_timestamp = xTaskGetTickCount();
	portDISABLE_INTERRUPTS();
	timer_reset(timer, scheduler_state);
	if (scheduler_state == taskSCHEDULER_NOT_STARTED) {
		if (ms > 0) {
			SYS_ASSERT(ms <= timer_systick_get_maximum_delay_ms());
			timer->state = stateActiveSystick;
			timer_systick_start(ms);
		}
	} else {
		timer_freertos_start(timer, current_timestamp, ms);
	}
	portENABLE_INTERRUPTS();
	if (scheduler_state != taskSCHEDULER_NOT_STARTED)
		timer_freertos_sync_task(pxHigherPriorityTaskWoken);
}

void hal_timer_start_from(hal_timer_handle_t handle, TickType_t timestamp, unsigned int ms, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	DEFINE_PCB_FROM_HANDLE(timer, handle)
	BaseType_t scheduler_state = xTaskGetSchedulerState();
	SYS_ASSERT(scheduler_state != taskSCHEDULER_NOT_STARTED);
	portDISABLE_INTERRUPTS();
	timer_reset(timer, scheduler_state);
	timer_freertos_start(timer, timestamp, ms);
	portENABLE_INTERRUPTS();
	timer_freertos_sync_task(pxHigherPriorityTaskWoken);
}

void hal_timer_stop(hal_timer_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(timer, handle)
	BaseType_t scheduler_state = xTaskGetSchedulerState();
	portDISABLE_INTERRUPTS();
	timer_reset(timer, scheduler_state);
	portENABLE_INTERRUPTS();
}

bool hal_timer_check_timeout(hal_timer_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(timer, handle)
	BaseType_t scheduler_state = xTaskGetSchedulerState();
	TickType_t current_timestamp = xTaskGetTickCount();
	bool expired;
	portDISABLE_INTERRUPTS();
	switch (timer->state) {
	case stateIdle: {
		expired = true;
		break;
	}
	case stateActiveSystick: {
		SYS_ASSERT(scheduler_state == taskSCHEDULER_NOT_STARTED);
		expired = timer_systick_check_expired();
		if (expired) {
			timer->state = stateIdle;
			timer_systick_stop();
		}
		break;
	}
	case stateActiveFreertos: {
		SYS_ASSERT(scheduler_state != taskSCHEDULER_NOT_STARTED);
		expired = timer_freertos_check_expired(timer, current_timestamp);
		break;
	}
	}
	portENABLE_INTERRUPTS();
	return expired;
}

void* hal_timer_get_userid(hal_timer_handle_t handle) {
	DEFINE_PCB_FROM_HANDLE(timer, handle)
	return timer->userid;
}

static void timer_reset(struct s_timer_pcb *timer, BaseType_t scheduler_state) {
	switch (timer->state) {
	case stateIdle:
		break;
	case stateActiveSystick: {
		SYS_ASSERT(scheduler_state == taskSCHEDULER_NOT_STARTED);
		timer_systick_stop();
		break;
	}
	case stateActiveFreertos: {
		SYS_ASSERT(scheduler_state != taskSCHEDULER_NOT_STARTED);
		break;
	}
	}
	timer->state = stateIdle;
	timer_remove_from_lists(timer);
}

static portTASK_FUNCTION(timer_task_function, pvParameters) {
	TickType_t ticks_to_wait = portMAX_DELAY;
	while (1) {
		xSemaphoreTake(timer_task_semphr, ticks_to_wait);
		timer_process_pending_list();
		ticks_to_wait = timer_process_expired();
	}
}

static void timer_remove_from_lists(struct s_timer_pcb *timer) {
	timer_update_current_pending(timer);
	if (DLLIST_IS_IN_LIST(pending, &timer_pending_list, timer))
		DLLIST_REMOVE_FROM_LIST(pending, &timer_pending_list, timer);
	if (DLLIST_IS_IN_LIST(active, &timer_active_list, timer)) {
		timer_update_active_list_iterator(timer);
		DLLIST_REMOVE_FROM_LIST(active, &timer_active_list, timer);
	}
}

static void timer_process_pending_list(void) {
	bool pending_list_not_empty;
	do {
		portDISABLE_INTERRUPTS();
		pending_list_not_empty = !DLLIST_IS_LIST_EMPTY(&timer_pending_list);
		if (pending_list_not_empty) {
			timer_current_pending = DLLIST_GET_LIST_FRONT(&timer_pending_list);
			DLLIST_REMOVE_FROM_LIST(pending, &timer_pending_list, timer_current_pending);
			timer_current_active = DLLIST_GET_LIST_BACK(&timer_active_list);
			while (timer_current_pending) {
				if (timer_current_active == 0) {
					DLLIST_ADD_TO_LIST_FRONT(active, &timer_active_list, timer_current_pending);
					break;
				}
				if (!timer_timeout_is_later(timer_current_active, timer_current_pending)) {
					DLLIST_INSERT_TO_LIST_AFTER(active, timer_current_active, timer_current_pending);
					break;
				}
				timer_update_active_list_iterator(timer_current_active);
				portENABLE_INTERRUPTS();
				portDISABLE_INTERRUPTS();
			}
			timer_update_current_pending(timer_current_pending);
		}
		portENABLE_INTERRUPTS();
	} while (pending_list_not_empty);
}

static TickType_t timer_process_expired(void) {
	TickType_t ticks_to_wait = portMAX_DELAY;
	bool active_list_not_empty;
	do {
		TickType_t current_timestamp = xTaskGetTickCount();
		portDISABLE_INTERRUPTS();
		active_list_not_empty = !DLLIST_IS_LIST_EMPTY(&timer_active_list);
		if (active_list_not_empty) {
			struct s_timer_pcb *timer = DLLIST_GET_LIST_FRONT(&timer_active_list);
			void (*timeout_callback)(hal_timer_handle_t handle);
			if ((timer->state == stateActiveFreertos) && !timer_freertos_check_expired(timer, current_timestamp)) {
				ticks_to_wait = timer->timestamp_end - current_timestamp;
				portENABLE_INTERRUPTS();
				break;
			}
			DLLIST_REMOVE_FROM_LIST(active, &timer_active_list, timer);
			timer->state = stateIdle;
			timeout_callback = timer->callbackTimeout;
			if (timeout_callback) {
				portENABLE_INTERRUPTS();
				timeout_callback((hal_timer_handle_t)timer);
				portDISABLE_INTERRUPTS();
			}
		}
		portENABLE_INTERRUPTS();
	} while (active_list_not_empty);
	return ticks_to_wait;
}

static bool timer_timeout_is_later(struct s_timer_pcb *timer1, struct s_timer_pcb *timer2) {
	bool result;
	SYS_ASSERT((timer1->state == stateActiveFreertos) && (timer2->state == stateActiveFreertos));
	if (timer1->timestamp_overflow == timer2->timestamp_overflow)
		result = (timer1->timestamp_end > timer2->timestamp_end);
	else
		result = (timer1->timestamp_end < timer2->timestamp_end);
	return result;
}

static void timer_update_current_pending(struct s_timer_pcb *timer) {
	if (timer == timer_current_pending)
		timer_current_pending = 0;
}

static void timer_update_active_list_iterator(struct s_timer_pcb *timer) {
	if (timer == timer_current_active)
		timer_current_active = DLLIST_GET_PREV(active, timer_current_active);
}

static uint32_t timer_systick_get_maximum_delay_ms(void) {
	return ((float)SysTick_LOAD_RELOAD_Msk/(SystemCoreClock/8))*1000;
}

static void timer_systick_start(unsigned int ms) {
	SysTick->CTRL = 0;
	SysTick->LOAD = ((float)ms/1000)*(SystemCoreClock/8);
	SysTick->VAL = 0;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

static void timer_systick_stop(void) {
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

static bool timer_systick_check_expired(void) {
	return ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) != 0);
}

static void timer_freertos_start(struct s_timer_pcb *timer, TickType_t timestamp_start, unsigned int ms) {
	SYS_ASSERT(ms/portTICK_PERIOD_MS < portMAX_DELAY);
	timer->state = stateActiveFreertos;
	timer->timestamp_start = timestamp_start;
	timer->timestamp_end = timer->timestamp_start + ms/portTICK_PERIOD_MS;
	timer->timestamp_overflow = (timer->timestamp_start > timer->timestamp_end);
	DLLIST_ADD_TO_LIST_BACK(pending, &timer_pending_list, timer);
}

static void timer_freertos_sync_task(signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	SYS_ASSERT(timer_task_semphr);
	if (halinternal_is_isr_active()) {
		SYS_ASSERT(pxHigherPriorityTaskWoken != 0);
		xSemaphoreGiveFromISR(timer_task_semphr, pxHigherPriorityTaskWoken);
	} else {
		SYS_ASSERT(pxHigherPriorityTaskWoken == 0);
		xSemaphoreGive(timer_task_semphr);
	}
}

static bool timer_freertos_check_expired(struct s_timer_pcb *timer, TickType_t current_timestamp) {
	bool expired;
	if (!timer->timestamp_overflow) {
		expired = !((timer->timestamp_start <= current_timestamp) && (current_timestamp < timer->timestamp_end));
	} else {
		expired = ((timer->timestamp_end <= current_timestamp) && (current_timestamp < timer->timestamp_start));
	}
	return expired;
}
