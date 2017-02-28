/*
 * virtual_timer.c
 *
 *  Created on: 28 ����. 2016 �.
 *      Author: Pankov_D
 */

#include "virtual_timer.h"

int fr_band_commanders[][6] =
{
		{1622,	 1974},
		{1975,	 2158, 2206,    2483},
		{2517,   2610, 2665,    2835},
		{3170,   3385, 3515,	3560},
		{3561,	 3885, 4015,	4334},
		{4335,	 4635, 4765,	4980,  5075,    5275},
		{5276,   5465, 5745,    5885,  6215,    6420},
		{6421,   6510, 6780,    7185,  7465,    7815},
		{7816,   8800, 9055,    9385},
		{9915,   9980, 10115,   11160, 11415,   11585}

};

int  getInterval(uint8_t min,uint8_t sec)
{
	uint8_t unchet = min % 2;
	uint8_t res = (60*unchet + sec)/12;
	res += 1;
	return res;
}

uint8_t IsStart(uint8_t sec)
{
	if (sec % 12 == 0) return 1;
	else return 0;
}

int getCommanderFreq(int RN_KEY, uint8_t SEC, uint8_t DAY, uint8_t HRS,uint8_t MIN)
{
    uint8_t interval = getInterval(MIN,SEC);
    uint8_t len_int[10] = {1,2,2,2,2,3,3,3,2,3};

    int fr_sh = CalcShiftCommandFreq(RN_KEY,SEC,DAY,HRS,MIN,interval);

    for(int i = 0; i< len_int[interval-1];i++)
    {
    	if (fr_sh + fr_band_commanders[interval-1][2*i] <  fr_band_commanders[interval-1][2*i+1])
    		return 1000 * (fr_sh + fr_band_commanders[interval-1][2*i]);
    	else
    		fr_sh = fr_sh - (fr_band_commanders[interval-1][2*i+1] - fr_band_commanders[interval-1][2*i]);
    }

    return 1000 * fr_band_commanders[interval-1][0];

}

int CalcShiftCommandFreq(int RN_KEY, uint8_t SEC, uint8_t DAY, uint8_t HRS, uint8_t MIN,int interval)
{
	uint32_t tot_width_int[10] = {352,460,263,260,643,715,534,844,1314,1280};

	int FR_SH = (RN_KEY + 47*SEC + 22*MIN + 57*HRS + 43*DAY) % tot_width_int[interval-1];
	return FR_SH;
}
