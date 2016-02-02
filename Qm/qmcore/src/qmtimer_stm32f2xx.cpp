/**
  ******************************************************************************
  * @file    qmtimer_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include "qmevent.h"
#include "qmapplication.h"
#include "qmobject_p.h"
#include "qmtimer.h"
#include "qmtimer_p.h"

extern "C" {
static void qmTimerCallbackTimeout(hal_timer_handle_t handle) {
	// assuming sysconfigTIMER_TASK_PRIORITY > qmconfigSYSTEM_PRIORITY
	QmTimerPrivate *t = static_cast<QmTimerPrivate *>(hal_timer_get_userid(handle));
	t->postTimeoutEvent();
}
}

QmTimerPrivate::QmTimerPrivate(QmTimer *q) :
	QmObjectPrivate(q),
	is_active(false), interval_value(0),
	timerhandle(0), is_single_shot(false)
{
}

QmTimerPrivate::~QmTimerPrivate()
{
}

void QmTimerPrivate::init(bool single_shot) {
	is_single_shot = single_shot;
	hal_timer_params_t params;
	params.userid = (void *)this;
	params.callbackTimeout = qmTimerCallbackTimeout;
	timerhandle = hal_timer_create(&params);
}

void QmTimerPrivate::deinit() {
	hal_timer_delete(timerhandle);
}

void QmTimerPrivate::postTimeoutEvent() {
	QM_Q(QmTimer);
	QmApplication::postEvent(q, new QmEvent(QmEvent::Timer));
}

void QmTimer::start() {
	QM_D(QmTimer);
	if (isActive())
		stop();
	d->is_active = true;
	hal_timer_start(d->timerhandle, d->interval_value, 0);
}

void QmTimer::stop() {
	QM_D(QmTimer);
	hal_timer_stop(d->timerhandle);
	d->is_active = false;
	QmApplication::removePostedEvents(this, QmEvent::Timer);
}

void QmTimer::setSingleShot(bool enable) {
	QM_D(QmTimer);
	if (d->is_single_shot == enable)
		return;
	stop();
	d->deinit();
	d->init(enable);
}

bool QmTimer::event(QmEvent* event) {
	QM_D(QmTimer);
	if (event->type() == QmEvent::Timer) {
		if (d->is_active) {
			if (!d->is_single_shot)
				hal_timer_start(d->timerhandle, d->interval_value, 0);
			timeout.emit();
		}
		return true;
	}
	return QmObject::event(event);
}
