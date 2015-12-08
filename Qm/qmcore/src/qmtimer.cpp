/**
  ******************************************************************************
  * @file    qmtimer.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include "qmevent.h"
#include "qmtimer.h"
#include "qmtimer_p.h"

QmTimer::QmTimer(bool single_shot, QmObject *parent) :
	QmObject(*new QmTimerPrivate(this), parent)
{
	QM_D(QmTimer);
	d->init(single_shot);
}

QmTimer::~QmTimer() {
	QM_D(QmTimer);
	if (isActive())
		stop();
	d->deinit();
}

bool QmTimer::isActive() const {
	QM_D(const QmTimer);
	return d->is_active;
}

void QmTimer::setInterval(unsigned int msec) {
	QM_D(QmTimer);
	d->interval_value = msec;
}

unsigned int QmTimer::interval() const {
	QM_D(const QmTimer);
	return d->interval_value;
}

void QmTimer::start(unsigned int msec) {
	setInterval(msec);
	start();
}
