/**
  ******************************************************************************
  * @file    spiffs_config_platform.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    17.02.2016
  *
  ******************************************************************************
 */

#ifndef SPIFFS_CONFIG_PLATFORM_H_
#define SPIFFS_CONFIG_PLATFORM_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>
typedef uint8_t u8_t;
typedef int8_t s8_t;
typedef uint16_t u16_t;
typedef int16_t s16_t;
typedef uint32_t u32_t;
typedef int32_t s32_t;
// alignment
#define SPIFFS_ALIGNED_OBJECT_INDEX_TABLES	1

#endif /* SPIFFS_CONFIG_PLATFORM_H_ */
