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
	hextiLine_0 = 0,
	hextiLine_1,
	hextiLine_2,
	hextiLine_3,
	hextiLine_4,
	hextiLine_5,
	hextiLine_6,
	hextiLine_7,
	hextiLine_8,
	hextiLine_9,
	hextiLine_10,
	hextiLine_11,
	hextiLine_12,
	hextiLine_13,
	hextiLine_14,
	hextiLine_15,
	hextiLine_PVD,
	hextiLine_RTC_Alarm,
	hextiLine_USB_OTG_FS_Wakeup,
	hextiLine_Ethernet_Wakeup,
	hextiLine_USB_OTG_HS_Wakeup,
	hextiLine_RTC_Tamper_Timestamp,
	hextiLine_RTC_Wakeup,
} hal_exti_line_t;

typedef enum {
	hextiMode_Rising,
	hextiMode_Falling,
	hextiMode_Rising_Falling
} hal_exti_mode_t;

typedef struct {
	hal_exti_mode_t mode;
	void *userid;
	void (*isrcallbackTrigger)(hal_exti_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken);
} hal_exti_params_t;

void hal_exti_set_default_params(hal_exti_params_t *params);
hal_exti_handle_t hal_exti_open(int line, hal_exti_params_t *params);
void hal_exti_close(hal_exti_handle_t handle);
void* hal_exti_get_userid(hal_exti_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* HAL_EXTI_H_ */
