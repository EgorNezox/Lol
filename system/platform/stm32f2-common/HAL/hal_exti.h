/**
  ******************************************************************************
  * @file    hal_exti.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    05.11.2015
  * @brief   Интерфейс аппаратной абстракции доступа к линиям внешних прерываний
  *
  ******************************************************************************
  */

#ifndef HAL_EXTI_H_
#define HAL_EXTI_H_

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void* hal_exti_handle_t;

typedef enum {
	hextiMode_Rising,
	hextiMode_Falling,
	hextiMode_Rising_Falling
} hal_exti_mode_t;

typedef struct {
	hal_exti_mode_t mode;
	void *userid;
	void (*isrcallbackTrigger)(hal_exti_handle_t handle, void *userid, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
} hal_exti_params_t;

hal_exti_handle_t hal_exti_open(int line, hal_exti_params_t *params);
void hal_exti_close(hal_exti_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* HAL_EXTI_H_ */
