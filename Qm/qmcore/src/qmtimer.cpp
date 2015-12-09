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

QmTimer::QmTimer(QmObject *parent) : QmTimer(false, parent)
{
}

QmTimer::QmTimer(bool single_shot, QmObject *parent) :
	QmObject(*new QmTimerPrivate(this), parent),
	is_active(0), interval_value(0)
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

void QmTimer::setInterval(unsigned int msec) {
	interval_value = msec;
}

void QmTimer::start(unsigned int msec) {
	setInterval(msec);
	start();
}
