/**
  ******************************************************************************
  * @file    sys_internal.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.08.2015
  * @brief   Заголовочный файл реализации аппаратной абстракции
  *
  ******************************************************************************
 */

#ifndef SYSTEM_INTERNAL_H_
#define SYSTEM_INTERNAL_H_

#include <stdint.h>
#include "system_hardware_config.h"

#ifndef NDEBUG
#define SYS_ASSERT(e)	do { if (!(e)) __asm volatile("bkpt"); } while (0)
#else
#define SYS_ASSERT(e)	((void)(e))
#endif

void halinternal_system_fault_handler(void);
void halinternal_timer_init(void);
void halinternal_uart_init(void);

#endif /* SYSTEM_INTERNAL_H_ */
