/**
  ******************************************************************************
  * @file    system_hardware_config.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    19.05.2016
  * @brief   Системная аппаратная конфигурация
  *
  ******************************************************************************
  */

#ifndef SYSTEM_HARDWARE_CONFIG_H_
#define SYSTEM_HARDWARE_CONFIG_H_

/*! Приоритет прерываний NVIC для обработки запросов аппаратной периферии */
#define SYS_IRQ_CHANNEL_PREEMPTION_PRIORITY	1
/*! Макс. задержка в обработке прерываний с приоритетом SYS_IRQ_CHANNEL_PREEMPTION_PRIORITY (в мс) */
#define SYS_MAX_IRQ_LATENCY_MS	1

#endif /* SYSTEM_HARDWARE_CONFIG_H_ */
