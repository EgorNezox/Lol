/**
  ******************************************************************************
  * @file    qmabstimer.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
  */

#include "qmevent.h"
#include "qmabstimer.h"
#include "qmabstimer_p.h"

QmAbsTimer::QmAbsTimer(QmObject *parent) :
	QmObject(*new QmAbsTimerPrivate(this), parent)
{
	QM_D(QmAbsTimer);
	d->init();
}

QmAbsTimer::~QmAbsTimer() {
	QM_D(QmAbsTimer);
	if (isActive())
		stop();
	d->deinit();
}

bool QmAbsTimer::isActive() const {
	QM_D(const QmAbsTimer);
	return d->is_active;
}
