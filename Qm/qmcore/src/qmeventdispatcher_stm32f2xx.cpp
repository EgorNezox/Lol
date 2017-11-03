/**
  ******************************************************************************
  * @file    qmeventdispatcher_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include <list>
#ifdef PORT__TARGET_DEVICE_REV1

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define QMDEBUGDOMAIN QmCore
#include "qm.h"
#include "qmdebug.h"
#include "qmobject_p.h"
#include "qmmutex.h"
#include "qmevent.h"
#include "qmapplication.h"
#include "qmeventdispatcher.h"

struct queued_event_t {
	QmEvent *event;
	QmObject *receiver;
};

class QmEventDispatcherPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmEventDispatcher)
public:
	QmEventDispatcherPrivate(QmEventDispatcher *q) :
		QmObjectPrivate(q),
		wake_semaphore(0), interrupt(false), blocked(false), processing_i(list.end())
	{
	}
	static bool isEventMatch(const queued_event_t& e, const QmObject *receiver, int event_type) {
		bool result = true;
		if (receiver)
			result &= (e.receiver == receiver);
		if (event_type > 0)
			result &= (e.event->type() == (QmEvent::Type)event_type);
		return result;
	}
	static bool isEventMatchRecursively(const queued_event_t& e, const QmObject *receiver) {
		QmObjectList receiver_children;
		if (e.receiver == receiver)
			return true;
		receiver_children = receiver->children();
		for (auto& i : receiver_children)
			if (isEventMatchRecursively(e, i))
				return true;
		return false;
	}
	SemaphoreHandle_t wake_semaphore;
	bool interrupt, blocked;
	QmMutex list_mutex;
	std::list<queued_event_t> list;
	std::list<queued_event_t>::iterator processing_i;
};

QmEventDispatcher::QmEventDispatcher(QmObject *parent) :
	QmObject(*new QmEventDispatcherPrivate(this), parent)
{
	QM_D(QmEventDispatcher);
	d->interrupt = false;
	d->wake_semaphore = xSemaphoreCreateBinary();
}

QmEventDispatcher::~QmEventDispatcher()
{
	QM_D(QmEventDispatcher);
	if (!d->list.empty()) {
		qmDebugMessage(QmDebug::Warning, "QmEventDispatcher::~QmEventDispatcher(0x%p): dropping not delivered events", this);
		for (auto& i : d->list)
			delete i.event;
	}
	vSemaphoreDelete(d->wake_semaphore);
}

void QmEventDispatcher::queueEvent(QmObject* receiver, QmEvent* event) {
	QM_D(QmEventDispatcher);
	d->list_mutex.lock();
	d->list.push_back((queued_event_t){event, receiver});
	d->list_mutex.unlock();
}

void QmEventDispatcher::processEvents(QmObject *receiver, int event_type) {
	QM_D(QmEventDispatcher);

	d->list_mutex.lock();
	d->processing_i = d->list.begin();
	d->list_mutex.unlock();
	xSemaphoreTake(d->wake_semaphore, 0);

	while (!d->interrupt) {
		bool do_process;
		queued_event_t queued_event;

		d->list_mutex.lock();
		if (d->blocked) {
			d->list_mutex.unlock();
			break;
		}
		if (d->processing_i == d->list.end()) {
			d->list_mutex.unlock();
			break;
		}
		queued_event = *(d->processing_i);
		do_process = QmEventDispatcherPrivate::isEventMatch(queued_event, receiver, event_type);
		if (do_process)
			d->processing_i = d->list.erase(d->processing_i);
		else
			++(d->processing_i);
		d->list_mutex.unlock();

		if (do_process) {
			QmApplication::sendEvent(queued_event.receiver, queued_event.event);
			delete queued_event.event;
		}
	}

	if (!d->interrupt)
		xSemaphoreTake(d->wake_semaphore, portMAX_DELAY);
	d->interrupt = false;
}

void QmEventDispatcher::removeQueuedEvents(QmObject* receiver, int event_type) {
	QM_D(QmEventDispatcher);
	d->list_mutex.lock();
	for (auto i = d->list.begin(); i != d->list.end(); ) {
		if (QmEventDispatcherPrivate::isEventMatch(*i, receiver, event_type)) {
			if (i == d->processing_i)
				++(d->processing_i);
			delete i->event;
			i = d->list.erase(i);
		} else {
			++i;
		}
	}
	d->list_mutex.unlock();
}

void QmEventDispatcher::blockProcessing() {
	QM_D(QmEventDispatcher);
	d->list_mutex.lock();
	QM_ASSERT(d->blocked == false);
	d->blocked = true;
	d->list_mutex.unlock();
}

void QmEventDispatcher::unblockProcessing() {
	QM_D(QmEventDispatcher);
	d->list_mutex.lock();
	QM_ASSERT(d->blocked == true);
	d->blocked = false;
	d->list_mutex.unlock();
}

void QmEventDispatcher::moveQueuedEvents(QmObject *receiver_parent, QmEventDispatcher *source, QmEventDispatcher *target) {
	QmEventDispatcherPrivate *source_p = source->d_func();
	QmEventDispatcherPrivate *target_p = target->d_func();
	source_p->list_mutex.lock();
	target_p->list_mutex.lock();
	for (auto i = source_p->list.begin(); i != source_p->list.end(); ) {
		if (QmEventDispatcherPrivate::isEventMatchRecursively(*i, receiver_parent)) {
			if (i == source_p->processing_i)
				++(source_p->processing_i);
			target_p->list.push_back(*i);
			i = source_p->list.erase(i);
		} else {
			++i;
		}
	}
	target_p->list_mutex.unlock();
	source_p->list_mutex.unlock();
}

void QmEventDispatcher::wakeUp() {
	QM_D(QmEventDispatcher);
	xSemaphoreGive(d->wake_semaphore);
}

void QmEventDispatcher::interrupt() {
	QM_D(QmEventDispatcher);
	d->interrupt = true;
	wakeUp();
}
#endif
