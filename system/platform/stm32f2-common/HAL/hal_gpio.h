/**
  ******************************************************************************
  * @file    hal_gpio.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    24.10.2015
  * @brief   Интерфейс аппаратной абстракции доступа к пинам ввода/вывода общего назначения
  *
  ******************************************************************************
  */

#ifndef HAL_GPIO_H_
#define HAL_GPIO_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	hgpioPA = 0,
	hgpioPB,
	hgpioPC,
	hgpioPD,
	hgpioPE,
	hgpioPF,
	hgpioPG,
	hgpioPH,
	hgpioPI
} hal_gpio_port_t;

typedef struct {
	hal_gpio_port_t port;
	uint8_t number;
} hal_gpio_pin_t;

typedef enum {
	hgpioMode_In,
	hgpioMode_Out,
	hgpioMode_AF,
	hgpioMode_Analog
} hal_gpio_mode_t;

typedef enum {
	hgpioSpeed_2MHz,
	hgpioSpeed_25MHz,
	hgpioSpeed_50MHz,
	hgpioSpeed_100MHz
} hal_gpio_speed_t;

typedef enum {
	hgpioType_PP,
	hgpioType_PPUp,
	hgpioType_PPDown,
	hgpioType_OD
} hal_gpio_type_t;

typedef enum {
	hgpioAF_SYS = 0,
	hgpioAF_TIM_1_2,
	hgpioAF_TIM_3_4_5,
	hgpioAF_TIM_8_9_10_11,
	hgpioAF_I2C_1_2_3,
	hgpioAF_SPI_1_2_I2S_2,
	hgpioAF_SPI_3_I2S_3,
	hgpioAF_USART_1_2_3,
	hgpioAF_UART_4_5_USART_6,
	hgpioAF_CAN_1_2_TIM_12_13_14,
	hgpioAF_OTG_FS_OTG_HS,
	hgpioAF_ETH,
	hgpioAF_FSMC_SDIO_OTG_HS,
	hgpioAF_DCMI,
	hgpioAF_14,
	hgpioAF_EVENTOUT
} hal_gpio_af_t;

typedef struct {
	hal_gpio_mode_t mode;
	hal_gpio_speed_t speed;
	hal_gpio_type_t type;
	hal_gpio_af_t af;
} hal_gpio_params_t;

typedef enum {
	hgpioLow = 0,
	hgpioHigh,
} hal_gpio_level_t;

void hal_gpio_init(hal_gpio_pin_t pin, hal_gpio_params_t *params);
void hal_gpio_deinit(hal_gpio_pin_t pin);
hal_gpio_level_t hal_gpio_get_input(hal_gpio_pin_t pin);
void hal_gpio_set_output(hal_gpio_pin_t pin, hal_gpio_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* HAL_GPIO_H_ */
