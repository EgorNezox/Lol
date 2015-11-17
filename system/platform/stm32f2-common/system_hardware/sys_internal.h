/**
  ******************************************************************************
  * @file    sys_internal.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.08.2015
  * @brief   Заголовочный файл реализации аппаратной абстракции
  *
  ******************************************************************************
 */

#ifndef SYS_INTERNAL_H_
#define SYS_INTERNAL_H_

#include <stdint.h>
#include "system_hardware_config.h"

#ifndef NDEBUG
#define SYS_ASSERT(e)	do { if (!(e)) __asm volatile("bkpt"); } while (0)
#else
#define SYS_ASSERT(e)	((void)(e))
#endif

#define MAX_DMA_TRANSFER_SIZE	0xFFFF // see STM32F2 reference manual

#define BB_MAP_REG_BIT(reg_offset, bit)	(*(__IO uint32_t *)(PERIPH_BB_BASE + (reg_offset * 32) + (bit * 4)))

#ifndef max
#define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)	(((a) < (b)) ? (a) : (b))
#endif

void halinternal_system_fault_handler(void);
void halinternal_gpio_init(void);
void halinternal_exti_init(void);
void halinternal_timer_init(void);
void halinternal_uart_init(void);

#endif /* SYS_INTERNAL_H_ */
