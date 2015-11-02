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

/* Low-level memory controller */
void stm32f2_ext_mem_init(void);
char stm32f2_ext_sram_test(void);
void stm32f2_LCD_init(void);

/* Platform hardware resources */
void stm32f2_ext_pins_init(int platform_hw_resource);
void stm32f2_ext_pins_deinit(int platform_hw_resource);

#endif /* SYSTEM_HW_IO_H_ */
