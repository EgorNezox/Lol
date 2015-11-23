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
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	RCC->AHB1ENR |= 0
			| RCC_AHB1ENR_GPIOAEN
			| RCC_AHB1ENR_GPIOBEN
			| RCC_AHB1ENR_GPIOCEN
			| RCC_AHB1ENR_GPIODEN
			| RCC_AHB1ENR_GPIOEEN
			| RCC_AHB1ENR_GPIOFEN
			| RCC_AHB1ENR_GPIOGEN
			| RCC_AHB1ENR_GPIOHEN
			| RCC_AHB1ENR_GPIOIEN;
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
	struct {
	  uint32_t MODER0;
	  uint32_t OTYPER0;
	  uint32_t OSPEEDR0;
	  uint32_t PUPDR0;
	} init_struct = {0x00, 0x00, 0x00, 0x00};
	switch (params->mode) {
	case hgpioMode_In:
		init_struct.MODER0 = 0;
		break;
	case hgpioMode_Out:
		init_struct.MODER0 = GPIO_MODER_MODER0_0;
		break;
	case hgpioMode_AF:
		init_struct.MODER0 = GPIO_MODER_MODER0_1;
		break;
	case hgpioMode_Analog:
		init_struct.MODER0 = GPIO_MODER_MODER0_0 | GPIO_MODER_MODER0_1;
		break;
	default: SYS_ASSERT(0); break;
	}
	switch (params->speed) {
	case hgpioSpeed_2MHz:
		init_struct.OSPEEDR0 = 0;
		break;
	case hgpioSpeed_25MHz:
		init_struct.OSPEEDR0 = GPIO_OSPEEDER_OSPEEDR0_0;
		break;
	case hgpioSpeed_50MHz:
		init_struct.OSPEEDR0 = GPIO_OSPEEDER_OSPEEDR0_1;
		break;
	case hgpioSpeed_100MHz:
		init_struct.OSPEEDR0 = GPIO_OSPEEDER_OSPEEDR0_0 | GPIO_OSPEEDER_OSPEEDR0_1;
		break;
	default: SYS_ASSERT(0); break;
	}
	init_struct.OTYPER0 = 0;
	init_struct.PUPDR0 = 0;
	switch (params->type) {
	case hgpioType_PP:
		break;
	case hgpioType_PPUp:
		init_struct.PUPDR0 = GPIO_PUPDR_PUPDR0_0;
		break;
	case hgpioType_PPDown:
		init_struct.PUPDR0 = GPIO_PUPDR_PUPDR0_1;
		break;
	case hgpioType_OD:
		init_struct.OTYPER0 = GPIO_OTYPER_OT_0;
		break;
	default: SYS_ASSERT(0); break;
	}
	SYS_ASSERT((params->af & ~0x0F) == 0);
	portENTER_CRITICAL();
	if (params->exti_source) {
		SYS_ASSERT(gpio_exti_source_assignment[pin.number] == -1);
		gpio_exti_source_assignment[pin.number] = pin.port;
		uint32_t tmp = ((uint32_t)0x0F) << (0x04 * (pin.number & 0x03));
		SYSCFG->EXTICR[pin.number >> 0x02] &= ~tmp;
		SYSCFG->EXTICR[pin.number >> 0x02] |= (((uint32_t)pin.port) << (0x04 * (pin.number & 0x03)));
	}
	GPIO_PORT[pin.port]->AFR[pin.number >> 0x03] &= ~((uint32_t)0x0F << ((uint32_t)((uint32_t)pin.number & 0x07) * 4));
	GPIO_PORT[pin.port]->AFR[pin.number >> 0x03] |= ((uint32_t)(params->af) << ((uint32_t)((uint32_t)pin.number & 0x07) * 4));
	GPIO_PORT[pin.port]->MODER &= ~(GPIO_MODER_MODER0 << (pin.number * 2));
	GPIO_PORT[pin.port]->MODER |= (init_struct.MODER0 << (pin.number * 2));
	GPIO_PORT[pin.port]->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR0 << (pin.number * 2));
	GPIO_PORT[pin.port]->OSPEEDR |= (init_struct.OSPEEDR0 << (pin.number * 2));
	GPIO_PORT[pin.port]->OTYPER  &= ~(GPIO_OTYPER_OT_0 << ((uint16_t)pin.number));
	GPIO_PORT[pin.port]->OTYPER |= (uint16_t)(init_struct.OTYPER0 << ((uint16_t)pin.number));
	GPIO_PORT[pin.port]->PUPDR &= ~(GPIO_PUPDR_PUPDR0 << ((uint16_t)pin.number * 2));
	GPIO_PORT[pin.port]->PUPDR |= (init_struct.PUPDR0 << (pin.number * 2));
	portEXIT_CRITICAL();
}

void hal_gpio_deinit(hal_gpio_pin_t pin) {
	SYS_ASSERT((0 <= pin.port) && (pin.port < GPIO_PORTS_COUNT));
	SYS_ASSERT((0 <= pin.number) && (pin.number < GPIO_PORT_PINS_COUNT));
	portENTER_CRITICAL();
	if (gpio_exti_source_assignment[pin.number] == pin.port)
		gpio_exti_source_assignment[pin.number] = -1;
	GPIO_PORT[pin.port]->AFR[pin.number >> 0x03] &= ~((uint32_t)0x0F << ((uint32_t)((uint32_t)pin.number & 0x07) * 4));
	GPIO_PORT[pin.port]->MODER &= ~(GPIO_MODER_MODER0 << (pin.number * 2));
	if (pin.port == hgpioPA)
		GPIO_PORT[pin.port]->MODER |= 0xA8000000 & (GPIO_MODER_MODER0 << (pin.number * 2));
	else if (pin.port == hgpioPB)
		GPIO_PORT[pin.port]->MODER |= 0x00000280 & (GPIO_MODER_MODER0 << (pin.number * 2));
	GPIO_PORT[pin.port]->OSPEEDR &= ~(GPIO_OSPEEDER_OSPEEDR0 << (pin.number * 2));
	if (pin.port == hgpioPB)
		GPIO_PORT[pin.port]->OSPEEDR |= 0x000000C0 & (GPIO_OSPEEDER_OSPEEDR0 << (pin.number * 2));
	GPIO_PORT[pin.port]->OTYPER  &= ~(GPIO_OTYPER_OT_0 << ((uint16_t)pin.number));
	GPIO_PORT[pin.port]->PUPDR &= ~(GPIO_PUPDR_PUPDR0 << ((uint16_t)pin.number * 2));
	if (pin.port == hgpioPA)
		GPIO_PORT[pin.port]->PUPDR |= 0x64000000 & (GPIO_PUPDR_PUPDR0 << ((uint16_t)pin.number * 2));
	else if (pin.port == hgpioPB)
		GPIO_PORT[pin.port]->PUPDR |= 0x00000100 & (GPIO_PUPDR_PUPDR0 << ((uint16_t)pin.number * 2));
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
