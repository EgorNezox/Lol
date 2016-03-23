/**
 ******************************************************************************
 * @file    qmelapsedtimer_qt.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    20.02.2016
 *
 ******************************************************************************
 */

#include <qelapsedtimer.h>

#include "qmelapsedtimer.h"

class QmElapsedTimerPrivate {
public:
	QElapsedTimer q;
};

QmElapsedTimer::QmElapsedTimer() :
	d_ptr(new QmElapsedTimerPrivate())
{
}

QmElapsedTimer::~QmElapsedTimer()
{
	delete d_ptr;
}

void QmElapsedTimer::start() {
	d_ptr->q.start();
}

int64_t QmElapsedTimer::restart() {
	return d_ptr->q.restart();
}

void QmElapsedTimer::invalidate() {
	d_ptr->q.invalidate();
}

bool QmElapsedTimer::isValid() const {
	return d_ptr->q.isValid();
}

int64_t QmElapsedTimer::elapsed() const {
	return d_ptr->q.elapsed();
}

bool QmElapsedTimer::hasExpired(int64_t timeout) const {
	return d_ptr->q.hasExpired(timeout);
}
