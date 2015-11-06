/**
  ******************************************************************************
  * @file    FreeRTOSConfig_app.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.08.2015
  * @brief   Пользовательская конфигурация FreeRTOS
  *
  ******************************************************************************
  */

#ifndef FREERTOSCONFIG_APP_H_
#define FREERTOSCONFIG_APP_H_

#define configTOTAL_HEAP_SIZE	( ( size_t ) ( 800 * 1024 ) )
#define usertaskSTACK_SIZE		8192

#endif /* FREERTOSCONFIG_APP_H_ */
