/**
  ******************************************************************************
  * @file    cm3_itm_debug.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @version V2.0
  * @date    14-March-2012
  * @brief   Cortex-M3 ITM debug support header file.
  ******************************************************************************
  */

#ifndef CM3_ITM_DEBUG_H_
#define CM3_ITM_DEBUG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void CM3_ITM_SendByte(uint8_t channel_num, uint8_t data);
void CM3_ITM_SendHalfWord(uint8_t channel_num, uint16_t data);
void CM3_ITM_SendWord(uint8_t channel_num, uint32_t data);

#ifdef __cplusplus
}
#endif

#endif /* CM3_ITM_DEBUG_H_ */
