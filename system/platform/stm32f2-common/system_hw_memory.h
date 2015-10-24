/**
  ******************************************************************************
  * @file    system_hw_memory.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    18.08.2015
  * @brief   Интерфейс аппаратного управления памятью системы
  *
  ******************************************************************************
  */

#ifndef SYSTEM_HW_MEMORY_H_
#define SYSTEM_HW_MEMORY_H_

void stm32f2_ext_mem_init(void);
char stm32f2_ext_sram_test(void);
void stm32f2_LCD_init(void);

#endif /* SYSTEM_HW_MEMORY_H_ */
