/**
  ******************************************************************************
  * @file    gpio.c
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    24.10.2015
  * @brief   Реализация аппаратной абстракции доступа к пинам ввода/вывода общего назначения на STM32F2xx
  *
  ******************************************************************************
  */

#include "stm32f2xx.h"
#include "FreeRTOS.h"

#include "hal_gpio.h"

#define BB_MAP_REG_BIT(reg_offset, bit)	(*(__IO uint32_t *)(PERIPH_BB_BASE + (reg_offset * 32) + (bit * 4)))

static uint32_t const STD_RCC_AHB1_PERIPH[] = {
		RCC_AHB1Periph_GPIOA,
		RCC_AHB1Periph_GPIOB,
		RCC_AHB1Periph_GPIOC,
		RCC_AHB1Periph_GPIOD,
		RCC_AHB1Periph_GPIOE,
		RCC_AHB1Periph_GPIOF,
		RCC_AHB1Periph_GPIOG,
		RCC_AHB1Periph_GPIOH,
		RCC_AHB1Periph_GPIOI
};

static GPIO_TypeDef* const STD_PORT[] = {
		GPIOA,
		GPIOB,
		GPIOC,
		GPIOD,
		GPIOE,
		GPIOF,
		GPIOG,
		GPIOH,
		GPIOI
};

static uint32_t const BITBAND_PORT_OFFSET[] = {
		(GPIOA_BASE - PERIPH_BASE),
		(GPIOB_BASE - PERIPH_BASE),
		(GPIOC_BASE - PERIPH_BASE),
		(GPIOD_BASE - PERIPH_BASE),
		(GPIOE_BASE - PERIPH_BASE),
		(GPIOF_BASE - PERIPH_BASE),
		(GPIOG_BASE - PERIPH_BASE),
		(GPIOH_BASE - PERIPH_BASE),
		(GPIOI_BASE - PERIPH_BASE)
};

void hal_gpio_init(hal_gpio_pin_t pin, hal_gpio_params_t *params) {
	GPIO_InitTypeDef init_struct;
	GPIO_StructInit(&init_struct);
	init_struct.GPIO_Pin = 1 << pin.number;
	switch (params->mode) {
	case hgpioMode_In: init_struct.GPIO_Mode = GPIO_Mode_IN; break;
	case hgpioMode_Out: init_struct.GPIO_Mode = GPIO_Mode_OUT; break;
	case hgpioMode_AF: init_struct.GPIO_Mode = GPIO_Mode_AF; break;
	case hgpioMode_Analog: init_struct.GPIO_Mode = GPIO_Mode_AN; break;
	}
	switch (params->speed) {
	case hgpioSpeed_2MHz: init_struct.GPIO_Speed = GPIO_Speed_2MHz; break;
	case hgpioSpeed_25MHz: init_struct.GPIO_Speed = GPIO_Speed_25MHz; break;
	case hgpioSpeed_50MHz: init_struct.GPIO_Speed = GPIO_Speed_50MHz; break;
	case hgpioSpeed_100MHz: init_struct.GPIO_Speed = GPIO_Speed_100MHz; break;
	}
	switch (params->type) {
	case hgpioType_PP: init_struct.GPIO_PuPd = GPIO_PuPd_NOPULL; break;
	case hgpioType_PPUp: init_struct.GPIO_PuPd = GPIO_PuPd_UP; break;
	case hgpioType_PPDown: init_struct.GPIO_PuPd = GPIO_PuPd_DOWN; break;
	case hgpioType_OD: init_struct.GPIO_OType = GPIO_OType_OD; break;
	}
	portENTER_CRITICAL();
	RCC_AHB1PeriphClockCmd(STD_RCC_AHB1_PERIPH[pin.port], ENABLE);
	GPIO_PinAFConfig(STD_PORT[pin.port], pin.number, params->af);
	GPIO_Init(STD_PORT[pin.port], &init_struct);
	portEXIT_CRITICAL();
}

void hal_gpio_deinit(hal_gpio_pin_t pin) {
	GPIO_InitTypeDef init_struct;
	GPIO_StructInit(&init_struct);
	init_struct.GPIO_Pin = 1 << pin.number;
	portENTER_CRITICAL();
	GPIO_Init(STD_PORT[pin.port], &init_struct);
	portEXIT_CRITICAL();
}

hal_gpio_level_t hal_gpio_get_input(hal_gpio_pin_t pin) {
	return (BB_MAP_REG_BIT((BITBAND_PORT_OFFSET[pin.port] + 0x10), pin.number) ? hgpioHigh : hgpioLow);
}

void hal_gpio_set_output(hal_gpio_pin_t pin, hal_gpio_level_t level) {
	BB_MAP_REG_BIT((BITBAND_PORT_OFFSET[pin.port] + 0x14), pin.number) = level;
}
