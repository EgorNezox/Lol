/**
  ******************************************************************************
  * @file    qmapplication_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifdef PORT__TARGET_DEVICE_REV1
#include "FreeRTOS.h"
#include "task.h"


#define QMDEBUGDOMAIN QmCore
#include "qm.h"
#include "qmdebug.h"
#include "qmevent.h"
#include "qmthread.h"
#include "qmthread_p.h"
#include "qmeventdispatcher.h"
#include "qmapplication.h"
#include "qmapplication_p.h"
#include "qmmutexlocker.h"

QmApplicationPrivate::QmApplicationPrivate(QmApplication *q) :
	QmObjectPrivate(q)
{
}

QmApplicationPrivate::~QmApplicationPrivate()
{
}

void QmApplication::exec() {
	QM_ASSERT(self);
	self->d_func()->event_loop.exec();
}

bool QmApplication::sendEvent(QmObject* receiver, QmEvent* event) {
	if (receiver->thread()->d_func()->task_handle != xTaskGetCurrentTaskHandle()) {
		qmDebugMessage(QmDebug::Error, "QmApplication::sendEvent(): event(0x%p) cannot be delivered to receiver(0x%p) due to thread mismatch", event, receiver);
		return false;
	}
	return receiver->d_func()->deliverEvent(event);
}

void QmApplication::postEvent(QmObject* receiver, QmEvent* event) {
	QmEventDispatcher *dispatcher;
	QmThread *receiver_thread;
	QmMutexLocker locker(&(receiver->d_func()->ta_mutex));
	receiver_thread = receiver->d_func()->thread;
	if (!((receiver_thread) && !(receiver->d_func()->drop_events))) {
		qmDebugMessage(QmDebug::Warning, "QmApplication::postEvent(): event(0x%p) cannot be posted to receiver(0x%p) thread", event, receiver);
		delete event;
		return;
	}
	dispatcher = receiver_thread->d_func()->event_dispatcher;
	dispatcher->queueEvent(receiver, event);
	dispatcher->wakeUp();
}

void QmApplication::sendPostedEvents(QmObject* receiver, int event_type) {
	QmThread *t = QmThread::currentThread();
	QM_ASSERT(t);
	t->d_func()->event_dispatcher->processEvents(receiver, event_type);
}

void QmApplication::removePostedEvents(QmObject* receiver, int event_type) {
	QmThread *t = QmThread::currentThread();
	QM_ASSERT(t);
	t->d_func()->event_dispatcher->removeQueuedEvents(receiver, event_type);
}
#endif
