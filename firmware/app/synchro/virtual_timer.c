/*
 * virtual_timer.c
 *
 *  Created on: 28 но€б. 2016 г.
 *      Author: Pankov_D
 */

#include "virtual_timer.h"

uint8_t  getInterval(uint8_t min,uint8_t sec)
{
	uint8_t interval = 1;
	interval = (min  % 2 == 0) ? 1 : 6;

	uint8_t ost = sec / 12; // 0 .. 4
	interval =  interval + ost;

	return (interval * 12);
}


int getCommanderFreq(uint8_t RN_KEY, uint8_t SEC, uint8_t DAY, uint8_t HRS,uint8_t MIN)
{
	uint8_t interval = getInterval(MIN,SEC);

	int fr_sh = CalcShiftCommandFreq(RN_KEY,SEC,DAY,HRS,MIN,interval);

	fr_sh += 1622;
	fr_sh = fr_sh * 1000;

	switch(interval)
	{
	case 1: {
		// 0-1
		fr_sh = fr_band_commander[0] + fr_sh;
		break;
	}
	case 2: {
		// 2 - 5
		int delta = fr_band_commander[3] - fr_band_commander[2];
		if (fr_sh > delta)
			fr_sh  = fr_sh + fr_band_commander[4] - delta;
		else
			fr_sh = fr_sh  + fr_band_commander[2];
		break;
	}
	case 3: {
		// 6 - 9
		int delta = fr_band_commander[7] - fr_band_commander[6];
		if (fr_sh > delta)
			fr_sh  = fr_sh + fr_band_commander[8] - delta;
		else
			fr_sh = fr_sh +	fr_band_commander[6];
		break;
	}
	case 4: {
		// 10 - 13
		int delta = fr_band_commander[11] - fr_band_commander[10];
		if (fr_sh > delta)
			fr_sh  = fr_sh + fr_band_commander[12] - delta;
		else
			fr_sh  = fr_sh + fr_band_commander[10];
		break;
	}
	case 5: {
		// 14 - 17
		int delta = fr_band_commander[15] - fr_band_commander[14];
		if (fr_sh > delta)
			fr_sh  = fr_sh + fr_band_commander[16] - delta;
		else
			fr_sh  = fr_sh + fr_band_commander[14];
		break;
	}
	case 6: {
		// 18 - 23
		int delta[2] = {0,0};

	    delta[0] = fr_band_commander[19] - fr_band_commander[18];
	    delta[1] = fr_band_commander[21] - fr_band_commander[20];

	    int param = delta[0] + delta[1];

	    if (fr_sh < delta[0])
	    	fr_sh = fr_sh + fr_band_commander[18];
	    if (fr_sh > delta[0] && fr_sh < param)
	    	fr_sh = fr_sh + fr_band_commander[20] - delta[0];
	    if (fr_sh > param)
	    	fr_sh = fr_sh + fr_band_commander[22] - param;
		break;
	}
	case 7: {
		// 24 - 29
		int delta[2] = {0,0};

		delta[0] = fr_band_commander[25] - fr_band_commander[24];
		delta[1] = fr_band_commander[27] - fr_band_commander[26];

		int param = delta[0] + delta[1];

		if (fr_sh < delta[0])
			fr_sh = fr_sh + fr_band_commander[24];
		if (fr_sh > delta[0] && fr_sh < param)
			fr_sh = fr_sh + fr_band_commander[26] - delta[0];
		if (fr_sh > param)
			fr_sh = fr_sh + fr_band_commander[28] - param;
		break;
	}
	case 8: {
		// 30 - 35
		int delta[2] = {0,0};

		delta[0] = fr_band_commander[31] - fr_band_commander[30];
		delta[1] = fr_band_commander[33] - fr_band_commander[32];

		int param = delta[0] + delta[1];

		if (fr_sh < delta[0])
			fr_sh = fr_sh + fr_band_commander[30];
		if (fr_sh > delta[0] && fr_sh < param)
			fr_sh = fr_sh + fr_band_commander[32] - delta[0];
		if (fr_sh > param)
			fr_sh = fr_sh + fr_band_commander[34] - param;
		break;
	}
	case 9: {
		// 36 - 39
		int delta = fr_band_commander[37] - fr_band_commander[36];
		if (fr_sh > delta)
			fr_sh  = fr_sh + fr_band_commander[38] - delta;
		else
			fr_sh  = fr_sh + fr_band_commander[36];
		break;
	}
	case 10:{
		// 40 - 45
		int delta[2] = {0,0};

		delta[0] = fr_band_commander[41] - fr_band_commander[40];
		delta[1] = fr_band_commander[43] - fr_band_commander[42];

		int param = delta[0] + delta[1];

		if (fr_sh < delta[0])
			fr_sh = fr_sh + fr_band_commander[40];
		if (fr_sh > delta[0] && fr_sh < param)
			fr_sh = fr_sh + fr_band_commander[42] - delta[0];
		if (fr_sh > param)
			fr_sh = fr_sh + fr_band_commander[44] - param;
		break;
	}
	default:break;
	}

	return fr_sh;
}

int CalcShiftCommandFreq(uint8_t RN_KEY, uint8_t SEC, uint8_t DAY, uint8_t HRS, uint8_t MIN,uint8_t interval)
{
	uint32_t tot_width_int[10] = {352,460,263,260,643,715,534,844,1314,1280};

	int FR_SH = (RN_KEY + 47*SEC + 22*MIN + 57*HRS + 43*DAY) % tot_width_int[interval-1];
	return FR_SH;
}







