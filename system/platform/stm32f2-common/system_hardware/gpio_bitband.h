/**
  ******************************************************************************
  * @file    gpio_bitband.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    29.05.2012
  * @brief   Определения макросов для bit-band доступа к GPIO-пинам
  *
  * Макросы принимают аргументы в формате:
  * - port, GPIO порт (Px)
  * - pin, пин (0..15)
  * Пример: GPIOx_ODR_pin_BB(PB, 12) = 1; // установить пин PB12 в единицу
  *
  ******************************************************************************
  */

#ifndef GPIO_BITBAND_H_
#define GPIO_BITBAND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f2xx.h"

#define BB_MAP_REG_BIT(reg_offset, bit)	(*(__IO uint32_t *)(PERIPH_BB_BASE + (reg_offset * 32) + (bit * 4)))

#define PA_OFFSET      (GPIOA_BASE - PERIPH_BASE)
#define PB_OFFSET      (GPIOB_BASE - PERIPH_BASE)
#define PC_OFFSET      (GPIOC_BASE - PERIPH_BASE)
#define PD_OFFSET      (GPIOD_BASE - PERIPH_BASE)
#define PE_OFFSET      (GPIOE_BASE - PERIPH_BASE)
#define PF_OFFSET      (GPIOF_BASE - PERIPH_BASE)
#define PG_OFFSET      (GPIOG_BASE - PERIPH_BASE)
#define PH_OFFSET      (GPIOH_BASE - PERIPH_BASE)
#define PI_OFFSET      (GPIOI_BASE - PERIPH_BASE)

#define GPIOx_IDR_OFFSET(port_offset)	(port_offset + 0x10)
#define GPIOx_ODR_OFFSET(port_offset)	(port_offset + 0x14)

#define GPIOx_IDR_pin_BB_(port, pin)	BB_MAP_REG_BIT(GPIOx_IDR_OFFSET(port##_OFFSET), pin)
#define GPIOx_ODR_pin_BB_(port, pin)	BB_MAP_REG_BIT(GPIOx_ODR_OFFSET(port##_OFFSET), pin)

/*! Макрос bit-band-доступа к регистру GPIOx_IDR (чтение пина) */
#define GPIOx_IDR_pin_BB(port, pin)		GPIOx_IDR_pin_BB_(port, pin)
/*! Макрос bit-band-доступа к регистру GPIOx_ODR (установка пина) */
#define GPIOx_ODR_pin_BB(port, pin)		GPIOx_ODR_pin_BB_(port, pin)

#ifdef __cplusplus
}
#endif
#endif /* GPIO_BITBAND_H_ */
