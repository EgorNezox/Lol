/**
  ******************************************************************************
  * @file    sys_internal_freertos_timers.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    13.11.2015
  *
  ******************************************************************************
 */

#ifndef SYS_INTERNAL_FREERTOS_TIMERS_H_
#define SYS_INTERNAL_FREERTOS_TIMERS_H_

/*! After hal_system_init() this variable contains minimum queue length required for freertos timers to function properly */
extern unsigned int halinternal_freertos_timer_queue_length;

#endif /* SYS_INTERNAL_FREERTOS_TIMERS_H_ */
