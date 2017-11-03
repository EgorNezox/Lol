/**
 ******************************************************************************
 * @file    qmelapsedtimer_stm32f2xx.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    20.02.2016
 *
 ******************************************************************************
 */
#ifdef PORT__TARGET_DEVICE_REV1
#include "FreeRTOS.h"
#include "task.h"

#include "qmelapsedtimer.h"

class QmElapsedTimerPrivate {
public:
	int64_t calcElapsed(TickType_t timestamp);
	bool valid;
	TickType_t start_timestamp;
};

int64_t QmElapsedTimerPrivate::calcElapsed(TickType_t timestamp) {
	if (!valid)
		return -1;
	return (TickType_t)(timestamp - start_timestamp);
}

QmElapsedTimer::QmElapsedTimer() :
	d_ptr(new QmElapsedTimerPrivate())
{
	d_ptr->valid = false;
}

QmElapsedTimer::~QmElapsedTimer()
{
	delete d_ptr;
}

void QmElapsedTimer::start() {
	d_ptr->start_timestamp = xTaskGetTickCount();
	d_ptr->valid = true;
}

int64_t QmElapsedTimer::restart() {
	TickType_t start_timestamp = xTaskGetTickCount();
	int64_t elapsed = d_ptr->calcElapsed(start_timestamp);
	d_ptr->start_timestamp = start_timestamp;
	d_ptr->valid = true;
	return elapsed;
}

void QmElapsedTimer::invalidate() {
	d_ptr->valid = false;
}

bool QmElapsedTimer::isValid() const {
	return d_ptr->valid;
}

int64_t QmElapsedTimer::elapsed() const {
	return d_ptr->calcElapsed(xTaskGetTickCount());
}

bool QmElapsedTimer::hasExpired(int64_t timeout) const {
	return (elapsed() > timeout);
}
#endif
