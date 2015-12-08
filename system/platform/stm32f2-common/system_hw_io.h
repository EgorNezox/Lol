/**
  ******************************************************************************
  * @file    system_hw_io.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    02.11.2015
  * @brief   Интерфейс аппаратного управления устройствами ввода/вывода системы
  *
  ******************************************************************************
  */

#ifndef SYSTEM_HW_IO_H_
#define SYSTEM_HW_IO_H_

#include "hal_gpio.h"
#include "hal_i2c.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Low-level memory controller */
void stm32f2_ext_mem_init(void);
char stm32f2_ext_sram_test(void);
void stm32f2_LCD_init(void);

/* Generic/common initialization (being called before entering main() !) */
void stm32f2_hardware_io_init(void);

/* Platform hardware resources */
void stm32f2_ext_pins_init(int platform_hw_resource);
void stm32f2_ext_pins_deinit(int platform_hw_resource);
hal_gpio_pin_t stm32f2_get_gpio_pin(int platform_hw_resource);
int stm32f2_get_exti_line(int platform_hw_resource);
int stm32f2_get_uart_instance(int platform_hw_resource);
int stm32f2_get_i2c_bus_instance(int platform_hw_resource);
hal_i2c_device_t stm32f2_get_i2c_device_descriptor(int platform_hw_resource);

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_HW_IO_H_ */
