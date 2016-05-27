/**
  ******************************************************************************
  * @file    hal_timer.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    03.09.2015
  * @brief   Интерфейс аппаратной абстракции доступа к таймеру
  *
  ******************************************************************************
  */

#ifndef HAL_TIMER_H_
#define HAL_TIMER_H_

#include <stdbool.h>
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* hal_timer_handle_t;

typedef struct {
	void *userid;		/*!< (опциональный) пользовательский идентификатор для callback'ов */
	/*! callback, вызываемый из таймерной задачи, индицирующий истечение таймаута */
	void (*callbackTimeout)(hal_timer_handle_t handle);
} hal_timer_params_t;

/*! Выдерживает задержку в мс */
void hal_timer_delay(unsigned int ms);

hal_timer_handle_t hal_timer_create(hal_timer_params_t *params);
void hal_timer_delete(hal_timer_handle_t handle);
void hal_timer_start(hal_timer_handle_t handle, unsigned int ms, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
void hal_timer_start_from(hal_timer_handle_t handle, TickType_t timestamp, unsigned int ms, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
void hal_timer_stop(hal_timer_handle_t handle);
bool hal_timer_check_timeout(hal_timer_handle_t handle);
void* hal_timer_get_userid(hal_timer_handle_t handle);

#ifdef __cplusplus
}
#endif
#endif /* HAL_TIMER_H_ */
