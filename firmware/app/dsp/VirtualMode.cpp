/**
 ******************************************************************************
 * @file    PswfModes.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    25.09.2017
 *
 ******************************************************************************
 */

#include "qmendian.h"
#include <stdio.h>
#include "dspcontroller.h"
#include <vector>
#include "qmthread.h"
#include "qmdebug.h"
#include "dsptransport.h"
#include "qmtimer.h"
#include <cstring>
#include "../synchro/virtual_timer.h"

namespace Multiradio
{

void DspController::startVirtualPpsModeTx()
{
	setPswfTx();

	boomVirtualPPS = false;
	masterVirtualPps = 0;

	ParameterValue comandValue;  //0x60 2 5 1
	comandValue.param = 1;
	sendCommandEasy(PSWFReceiver,5,comandValue);

	RtcTxRole = true;
	RtcRxRole = false;
	RtcTxCounter = 0;
	virtGuiCounter = 0;
	//radio_state = radiostatePswf;

	count_VrtualTimer = startVirtTxPhaseIndex;
}

void DspController::startVirtualPpsModeRx()
{
	setRx();

	ParameterValue comandValue;
	comandValue.param = 1;
	sendCommandEasy(PSWFReceiver,5,comandValue);
	comandValue.param = 2;
	sendCommandEasy(PSWFReceiver,4,comandValue);

	RtcRxRole = true;
	RtcTxRole = false;
	RtcFirstCatch = 0;
#ifndef PORT__PCSIMULATOR
	t = rtc->getTime();
	d = rtc->getDate();
#endif
	antiSync = false;
	pswf_in_virt = 0;
	count_VrtualTimer = NUMS;

	virtGuiCounter = 0;
}

void DspController::sendSynchro(uint32_t freq, uint8_t cnt)
{
//	qmDebugMessage(QmDebug::Dump, "freq virtual %d", freq);
	if (RtcTxRole)
	{
		uint8_t tx_address = 0x72;
		uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
		int tx_data_len = 0;
		qmToBigEndian((uint8_t)20, tx_data + tx_data_len);
		++tx_data_len;
		qmToBigEndian((uint8_t)2, tx_data + tx_data_len);
		++tx_data_len;
		qmToBigEndian((uint32_t)freqVirtual, tx_data + tx_data_len);
		tx_data_len = tx_data_len + 4;
		qmToBigEndian((uint8_t)1, tx_data + tx_data_len);
		++tx_data_len;
		qmToBigEndian((uint8_t)cnt, tx_data + tx_data_len);
		++tx_data_len;

		transport->transmitFrame(tx_address,tx_data,tx_data_len);
	}
}

void DspController::correctTime(uint8_t num)
{
	// correction time
//	qmDebugMessage(QmDebug::Dump, "correctTime() data[7] as num %d", num);
//	qmDebugMessage(QmDebug::Dump, "correctTime() before getTime t.seconds %d", t.seconds);

//	qmDebugMessage(QmDebug::Dump, "correctTime() after getTime t.seconds %d", t.seconds);
	t.seconds = 12 * (t.seconds / 12) + 7;
//	qmDebugMessage(QmDebug::Dump, "correctTime() after correct t.seconds %d", t.seconds);
	count_VrtualTimer = num;
//    qmDebugMessage(QmDebug::Dump, "correctTime() COUNTER VIRTUAL %d",count_VrtualTimer);

	RtcFirstCatch = -1;
}

bool DspController::getVirtualMode()
{
	return virtual_mode;
}

void DspController::setVirtualMode(bool param)
{
	virtual_mode = param;
	if (!virtual_mode)
	{
		ParameterValue comandValue;  //0x60 2 5 1
		comandValue.param = 0;
		sendCommandEasy(PSWFReceiver,5,comandValue);
		virtGuiCounter = 0;
	}
}

void DspController::getVirtualDate(uint8_t *day, uint8_t *month, uint8_t *year)
{
#ifndef PORT__PCSIMULATOR
	d = rtc->getDate();
	*day = d.day;
	*month = d.month;
	*year = d.year;
#endif
}

void DspController::setVirtualDate(uint8_t *param)
{
    uint8_t date[3];

    for(int i = 0; i<6;i++) param[i] = param[i] - 48;

    date[0] =  10*param[0] + param[1];
    date[1] =  10*param[2] + param[3];
    date[2] =  10*param[4] + param[5];

#ifndef PORT__PCSIMULATOR
    d.day = date[0];
    d.month = date[1];
    d.year  = date[2];
    rtc->setDate(d);
    d = rtc->getDate();
#endif
}

void DspController::setVirtualTime(uint8_t *param)
{
    for(int i = 0; i<6;i++) param[i] = param[i] - 48;

    uint8_t time[3] = {0,0,0};

    time[2] = 10*param[0] + param[1];
    time[1] = 10*param[2] + param[3];
    time[0] = 10*param[4] + param[5];

#ifndef PORT__PCSIMULATOR
    t.seconds  = time[0];
    t.minutes  = time[1];
    t.hours    = time[2];
    rtc->setTime(t);
#endif

}

uint8_t* DspController::getVirtualTime()
{
#ifndef PORT__PCSIMULATOR
	QmRtc::Time time = rtc->getTime();
	char param = 0;
	char ms[3] = {0,0,0};

	ms[2] = time.seconds;
	ms[1] = time.minutes;
	ms[0] = time.hours;

	for(int i = 0; i<3;i++)
	if (ms[i] > 9)
	{
		param = ms[i] / 10;
		virtualTime[2*i]   = param + 48;
		param = ms[i] % 10;
		virtualTime[2*i+1] = param + 48;
	}
	else
	{
		virtualTime[2*i] = 48;
		virtualTime[2*i+1] = ms[i] + 48;
	}
#endif
	return &virtualTime[0];
}

void DspController::vm1Pps()
{
    int hrs(0), min(0), sec(0);

    if (virtual_mode)
    {
#ifndef PORT__PCSIMULATOR
        QmRtc::Time time;
        time = rtc->getTime();

        hrs = time.hours;
        min = time.minutes;
        sec = time.seconds;
#endif
    }
    else
    {
        Navigation::Coord_Date date = navigator->getCoordDate();

        char hr_ch[3] = {0,0,0};
        char mn_ch[3] = {0,0,0};
        char sc_ch[3] = {0,0,0};

        memcpy(hr_ch,&date.time[0],2);
        memcpy(mn_ch,&date.time[2],2);
        memcpy(sc_ch,&date.time[4],2);

        hrs = atoi(hr_ch);
        min = atoi(mn_ch);
        sec = atoi(sc_ch);

        sec += 1;
        if (sec >= 60) {
        	sec %= 60;
        	min++;
        	if (min >= 60) {
        		min %= 60;
        		hrs++;
        		if (hrs >= 24) {
        			hrs %= 24;
        			min = 0;
        			sec = 0;
        		}
        	}
        }

    }

    vm1PpsPulse(hrs, min, sec);
}

} /* namespace Multiradio */
