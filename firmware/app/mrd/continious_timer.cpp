/*
 * ContiniousTimer.cpp
 *
 *  Created on: 06.02.2017
 *      Author: Alex
 */

#include <sigc++/sigc++.h>
#ifdef QM_PLATFORM_STM32F2XX
#include "FreeRTOS.h"
#include "task.h"
#endif
#include <stdio.h>
#include <stdarg.h>

#include "qmmutex.h"
#include "qmdebug.h"
#include "continious_timer.h"

ContTimer::ContTimer( void *CallBack )
{
    callback = (void(*)())CallBack;
	HI_level_timer = new QmTimer(false);
    HI_level_timer->timeout.connect(sigc::mem_fun(this,ContTimer::onTimerTimeOut));
	reset();
    RUN = false;
}

void ContTimer::onTimerTimeOut()
{
    callback();
}


void ContTimer::start_timer(unsigned int interval)
//void ContTimer::start(unsigned int interval)
{
    HI_level_timer->stop();
#ifdef QM_PLATFORM_STM32F2XX
	START_TICK = xTaskGetTickCount();
#endif
	LAST_INTERVAL=interval;
	SYSTEM_TICK = START_TICK + interval;
	HI_level_timer->start(interval);
    RUN = true;
}

//void ContTimer::stop()
void ContTimer::stop_timer()
{
#ifdef QM_PLATFORM_STM32F2XX
	SYSTEM_TICK = xTaskGetTickCount();
#endif
	HI_level_timer->stop();
    RUN = false;
}

void ContTimer::reset()
{
#ifdef QM_PLATFORM_STM32F2XX
	SYSTEM_TICK = xTaskGetTickCount();
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
#ifdef QM_PLATFORM_STM32F2XX
	CurrentTick = xTaskGetTickCount();
#endif
	if (CurrentTick > SYSTEM_TICK + interval)// to do сделать честную проверку
	{ rez = false;
	 START_TICK=CurrentTick;
	 SYSTEM_TICK = START_TICK + interval;
	 HI_level_timer->start(interval);

	}
	else
	{
		START_TICK = SYSTEM_TICK;
		DiffTick = CurrentTick - SYSTEM_TICK;
		SYSTEM_TICK = SYSTEM_TICK + interval;
		HI_level_timer->start(interval-DiffTick);
	}
	LAST_INTERVAL=interval;
	return rez;
}

void ContTimer::add_interval(unsigned int interval)
{
	unsigned long CurrentTick,DiffTick;
#ifdef QM_PLATFORM_STM32F2XX
		CurrentTick = xTaskGetTickCount();
#endif
		DiffTick = SYSTEM_TICK - CurrentTick;
		SYSTEM_TICK = SYSTEM_TICK + interval;
		HI_level_timer->start(interval+DiffTick);
		LAST_INTERVAL=interval + LAST_INTERVAL;
}

unsigned int ContTimer::get_timer_counter()
//unsigned int ContTimer::get_counter()
{
	return LAST_INTERVAL;//xTaskGetTickCount() - START_TICK;
}

unsigned int ContTimer::get_interval()
{
	return SYSTEM_TICK - START_TICK;
}
