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

#include "sys_internal.h"
#include "hal_gpio.h"

#define GPIO_PORTS_COUNT		9
#define GPIO_PORT_PINS_COUNT	16

static uint32_t const GPIO_RCC_AHB1_PERIPH[] = {
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

static GPIO_TypeDef* const GPIO_PORT[] = {
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

static uint32_t const GPIO_BITBAND_PORT_OFFSET[] = {
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

static int gpio_exti_source_assignment[GPIO_PORT_PINS_COUNT];

void halinternal_gpio_init(void) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
	for (int i = 0; i < sizeof(gpio_exti_source_assignment)/sizeof(gpio_exti_source_assignment[0]); i++)
		gpio_exti_source_assignment[i] = -1;
}

void hal_gpio_set_default_params(hal_gpio_params_t *params) {
	SYS_ASSERT(params);
	params->mode = hgpioMode_In;
	params->speed = hgpioSpeed_2MHz;
	params->type = hgpioType_PP;
	params->af = hgpioAF_SYS;
	params->exti_source = false;
}

void hal_gpio_init(hal_gpio_pin_t pin, hal_gpio_params_t *params) {
	SYS_ASSERT((0 <= pin.port) && (pin.port < GPIO_PORTS_COUNT));
	SYS_ASSERT((0 <= pin.number) && (pin.number < GPIO_PORT_PINS_COUNT));
	SYS_ASSERT(params);
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
	if (params->exti_source) {
		SYS_ASSERT(gpio_exti_source_assignment[pin.number] == -1);
		SYSCFG_EXTILineConfig(pin.port, pin.number);
		gpio_exti_source_assignment[pin.number] = pin.port;
	}
	RCC_AHB1PeriphClockCmd(GPIO_RCC_AHB1_PERIPH[pin.port], ENABLE);
	GPIO_PinAFConfig(GPIO_PORT[pin.port], pin.number, params->af);
	GPIO_Init(GPIO_PORT[pin.port], &init_struct);
	portEXIT_CRITICAL();
}

void hal_gpio_deinit(hal_gpio_pin_t pin) {
	SYS_ASSERT((0 <= pin.port) && (pin.port < GPIO_PORTS_COUNT));
	SYS_ASSERT((0 <= pin.number) && (pin.number < GPIO_PORT_PINS_COUNT));
	GPIO_InitTypeDef init_struct;
	GPIO_StructInit(&init_struct);
	init_struct.GPIO_Pin = 1 << pin.number;
	portENTER_CRITICAL();
	if (gpio_exti_source_assignment[pin.number] == pin.port)
		gpio_exti_source_assignment[pin.number] = -1;
	GPIO_Init(GPIO_PORT[pin.port], &init_struct);
	portEXIT_CRITICAL();
}

hal_gpio_level_t hal_gpio_get_input(hal_gpio_pin_t pin) {
	SYS_ASSERT((0 <= pin.port) && (pin.port < GPIO_PORTS_COUNT));
	SYS_ASSERT((0 <= pin.number) && (pin.number < GPIO_PORT_PINS_COUNT));
	return (BB_MAP_REG_BIT((GPIO_BITBAND_PORT_OFFSET[pin.port] + 0x10), pin.number) ? hgpioHigh : hgpioLow);
}

void hal_gpio_set_output(hal_gpio_pin_t pin, hal_gpio_level_t level) {
	SYS_ASSERT((0 <= pin.port) && (pin.port < GPIO_PORTS_COUNT));
	SYS_ASSERT((0 <= pin.number) && (pin.number < GPIO_PORT_PINS_COUNT));
	BB_MAP_REG_BIT((GPIO_BITBAND_PORT_OFFSET[pin.port] + 0x14), pin.number) = level;
}
