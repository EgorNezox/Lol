/**
  ******************************************************************************
  * @file    hal_rcc.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    20.05.2016
  * @brief   Интерфейс аппаратной абстракции доступа управлению тактированием и сбросом системы
  *
  ******************************************************************************
  */

#ifndef HAL_RCC_H_
#define HAL_RCC_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t sysclk_frequency;	/*!< SYSCLK clock frequency expressed in Hz */
	uint32_t hclk_frequency;	/*!< HCLK clock frequency expressed in Hz */
	uint32_t pclk1_frequency;	/*!< PCLK1 clock frequency expressed in Hz */
	uint32_t pclk2_frequency;	/*!< PCLK2 clock frequency expressed in Hz */
} hal_rcc_clocks_t;

void hal_rcc_get_clocks(hal_rcc_clocks_t* clocks);
bool hal_rcc_enable_hse(bool bypass_oscillator, unsigned int startup_timeout_ms);
void hal_rcc_disable_hse();

#ifdef __cplusplus
}
#endif

#endif /* HAL_RCC_H_ */
