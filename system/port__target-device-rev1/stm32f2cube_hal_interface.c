/**
  ******************************************************************************
  * @file    stm32f2cube_hal_interface.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    12.01.2016
  *
  ******************************************************************************
 */

#include "stm32f2xx_hal.h"
#include "stm32f2xx.h"

extern char stm32f2cube_hal_active;

void init_stm32f2cube_hal() {
	stm32f2cube_hal_active = 1;
	HAL_Init();
	__enable_irq();
}

void deinit_stm32f2cube_hal() {
	HAL_SuspendTick(); // не вызывать HAL_DeInit(), т.к. она делает полный сброс всей периферии (тварь такая)
	stm32f2cube_hal_active = 0;
}
