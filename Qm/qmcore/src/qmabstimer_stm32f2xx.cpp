/**
  ******************************************************************************
  * @file    qmabstimer_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
  */

#include "qmevent.h"
#include "qmapplication.h"
#include "qmobject_p.h"
#include "qmabstimer.h"
#include "qmabstimer_p.h"
#include "qmtimestamp.h"
#include "qmtimestamp_p.h"

extern "C" {
static void qmTimerCallbackTimeout(hal_timer_handle_t handle) {
	// assuming sysconfigTIMER_TASK_PRIORITY > qmconfigSYSTEM_PRIORITY
	QmAbsTimerPrivate *t = static_cast<QmAbsTimerPrivate *>(hal_timer_get_userid(handle));
	t->postTimeoutEvent();
}
}

QmAbsTimerPrivate::QmAbsTimerPrivate(QmAbsTimer *q) :
	QmObjectPrivate(q),
	is_active(false),
	timerhandle(0)
{
}

QmAbsTimerPrivate::~QmAbsTimerPrivate()
{
}

void QmAbsTimerPrivate::init() {
	hal_timer_params_t params;
	params.userid = (void *)this;
	params.callbackTimeout = qmTimerCallbackTimeout;
	timerhandle = hal_timer_create(&params);
}

void QmAbsTimerPrivate::deinit() {
	hal_timer_delete(timerhandle);
}

void QmAbsTimerPrivate::postTimeoutEvent() {
	QM_Q(QmAbsTimer);
	QmApplication::postEvent(q, new QmEvent(QmEvent::Timer));
}

bool QmAbsTimer::start(QmTimestamp *timestamp, unsigned int msec) {
	QM_D(QmAbsTimer);
	if (!timestamp->impl->valid)
		return false;
	if (isActive())
		stop();
	d->is_active = true;
	hal_timer_start_from(d->timerhandle, timestamp->impl->value, msec, 0);
	return true;
}

void QmAbsTimer::stop() {
	QM_D(QmAbsTimer);
	hal_timer_stop(d->timerhandle);
	d->is_active = false;
	QmApplication::removePostedEvents(this, QmEvent::Timer);
}

bool QmAbsTimer::event(QmEvent* event) {
	QM_D(QmAbsTimer);
	if (event->type() == QmEvent::Timer) {
		if (d->is_active) {
			d->is_active = false;
			timeout.emit();
		}
		return true;
	}
	return QmObject::event(event);
}
