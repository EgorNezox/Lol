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

static int fr_band_commander[46] =
{
		1622,	 1974,
		1975,	 2158,
		2206,    2483,
		2517,    2610,
		2665,    2835,
		3170,    3385,
		3515,	 3560,
		3561,	 3885,
		4015,	 4334,
		4335,	 4635,
		4765,	 4980,
		5075,    5275,
		5276,    5465,
		5745,    5885,
		6215,    6420,
		6421,    6510,
		6780,    7185,
		7465,    7815,
		7816,    8800,
		9055,    9385,
		9915,    9980,
		10115,   11160,
		11415,   11585

};

int  getInterval(uint8_t min,uint8_t sec);
int CalcShiftCommandFreq(uint8_t RN_KEY, uint8_t SEC, uint8_t DAY, uint8_t HRS, uint8_t MIN,int interval);
int getCommanderFreq(uint8_t RN_KEY, uint8_t SEC, uint8_t DAY, uint8_t HRS, uint8_t MIN);
uint8_t IsStart(uint8_t sec);

#ifdef __cplusplus
}
#endif


#endif /* FIRMWARE_APP_SYNCHRO_VIRTUAL_TIMER_H_ */
