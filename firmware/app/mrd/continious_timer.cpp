/*
 * ContiniousTimer.cpp
 *
 *  Created on: 06.02.2017
 *      Author: Alex
 */

#include "qmdebug.h"

#include "continious_timer.h"

ContTimer::ContTimer()
{
	HI_level_timer = new QmTimer(false);
    HI_level_timer->timeout.connect(sigc::mem_fun(this, &ContTimer::onTimerTimeOut));
	reset();
    RUN = false;
}

void ContTimer::onTimerTimeOut()
{
    timerTimeout();
}


void ContTimer::start_timer(unsigned int interval)
//void ContTimer::start(unsigned int interval)
{
    HI_level_timer->stop();
#ifndef QM_PLATFORM_QT
	START_TICK  = QmDebug::getTicks(); //xTaskGetTickCount();
#endif
	LAST_INTERVAL=interval;
	SYSTEM_TICK = START_TICK + interval;
	HI_level_timer->start(interval);    
    //QmDebug::message("ALE_timer", QmDebug::Info, "Timer START, interval %u", interval);
    RUN = true;
}

//void ContTimer::stop()
void ContTimer::stop_timer()
{
#ifndef QM_PLATFORM_QT
	SYSTEM_TICK  = QmDebug::getTicks();//= xTaskGetTickCount();
#endif
	HI_level_timer->stop();
    RUN = false;
}

void ContTimer::reset()
{
#ifndef QM_PLATFORM_QT
	SYSTEM_TICK = QmDebug::getTicks();//xTaskGetTickCount();
#endif
	START_TICK = SYSTEM_TICK;
	HI_level_timer->stop();
    RUN = false;
}

bool ContTimer::get_timer_state()
{
	return RUN;
}

bool ContTimer::set_timer(unsigned int interval)
//bool ContTimer::set_interval(unsigned int interval)
{
    bool rez = true;
	unsigned long CurrentTick,DiffTick;
    if(RUN==false)
        return false;
#ifndef QM_PLATFORM_QT
	CurrentTick =  QmDebug::getTicks();//xTaskGetTickCount();
#endif
	if (CurrentTick > SYSTEM_TICK + interval)// to do ������� ������� ��������
    {
        rez = false;
        START_TICK=CurrentTick;
        SYSTEM_TICK = START_TICK + interval;
        HI_level_timer->start(interval);
        //QmDebug::message("ALE_timer", QmDebug::Info, "Timer set error, interval %u", (interval));
	}
	else
	{
		START_TICK = SYSTEM_TICK;
		DiffTick = CurrentTick - SYSTEM_TICK;
		SYSTEM_TICK = SYSTEM_TICK + interval;
		HI_level_timer->start(interval-DiffTick);
        //QmDebug::message("ALE_timer", QmDebug::Info, "Timer set, interval %u", (interval-DiffTick));
	}
	LAST_INTERVAL=interval;
	return rez;
}

unsigned int ContTimer::get_timer_counter()
//unsigned int ContTimer::get_counter()
{
#ifndef QM_PLATFORM_QT
    //return /*LAST_INTERVAL;//*/xTaskGetTickCount() - START_TICK;
    return QmDebug::getTicks() - START_TICK;
#else
    return LAST_INTERVAL;//xTaskGetTickCount() - START_TICK;
#endif
}
