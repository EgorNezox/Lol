/*
 * virtual_timer.h
 *
 *  Created on: 28 но€б. 2016 г.
 *      Author: Pankov_D
 */

#ifndef FIRMWARE_APP_SYNCHRO_VIRTUAL_TIMER_H_
#define FIRMWARE_APP_SYNCHRO_VIRTUAL_TIMER_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

int  getInterval(uint8_t min,uint8_t sec);
int CalcShiftCommandFreq(int RN_KEY, uint8_t SEC, uint8_t DAY, uint8_t HRS, uint8_t MIN,int interval);
int getCommanderFreq(int RN_KEY, uint8_t SEC, uint8_t DAY, uint8_t HRS, uint8_t MIN);
uint8_t IsStart(uint8_t sec);

#ifdef __cplusplus
}
#endif


#endif /* FIRMWARE_APP_SYNCHRO_VIRTUAL_TIMER_H_ */
