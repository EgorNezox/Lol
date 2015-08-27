/**
  ******************************************************************************
  * @file    qmtimer_bmfreertos.cpp
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
static void qmTimerCallbackEntry(TimerHandle_t xTimer) {
	QmTimerPrivate *t = static_cast<QmTimerPrivate *>(pvTimerGetTimerID(xTimer));
	t->callback();
}
}

QmTimerPrivate::QmTimerPrivate(QmTimer *q) :
	QmObjectPrivate(q), timerhandle(0), is_single_shot(false), awaiting_callback(false)
{
}

QmTimerPrivate::~QmTimerPrivate()
{
}

void QmTimerPrivate::init(bool single_shot) {
	is_single_shot = single_shot;
	timerhandle = xTimerCreate("qm_", portMAX_DELAY, (single_shot ? pdFALSE : pdTRUE), (void *)this, qmTimerCallbackEntry);
}

void QmTimerPrivate::deinit() {
	xTimerDelete(timerhandle, portMAX_DELAY);
}

void QmTimerPrivate::postTimeoutEvent() {
	QM_Q(QmTimer);
	QmApplication::postEvent(q, new QmEvent(QmEvent::Timer));
}

void QmTimerPrivate::callback() {
	if (!awaiting_callback)
		return;
	awaiting_callback = false;
	postTimeoutEvent();
}

void QmTimer::start() {
	QM_D(QmTimer);
	if (isActive())
		stop();
	is_active = true;
	if (interval_value > 0) {
		d->awaiting_callback = true;
		xTimerChangePeriod(d->timerhandle, interval_value/portTICK_PERIOD_MS, portMAX_DELAY);
		xTimerStart(d->timerhandle, portMAX_DELAY);
	} else {
		d->postTimeoutEvent();
	}
}

void QmTimer::stop() {
	QM_D(QmTimer);
	if (xTimerIsTimerActive(d->timerhandle))
		xTimerStop(d->timerhandle, portMAX_DELAY);
	is_active = false;
	QmApplication::removePostedEvents(this, QmEvent::Timer);
}

bool QmTimer::event(QmEvent* event) {
	QM_D(QmTimer);
	if (event->type() == QmEvent::Timer) {
		if (is_active) {
			if (interval_value > 0) {
				d->awaiting_callback = true;
			} else {
				if (!d->is_single_shot)
					d->postTimeoutEvent();
			}
			timeout.emit();
		}
		return true;
	}
	return QmObject::event(event);
}
