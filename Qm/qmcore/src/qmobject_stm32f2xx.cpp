/**
  ******************************************************************************
  * @file    qmobject_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#define QMDEBUGDOMAIN QmCore
#include "qmdebug.h"
#include "qmevent.h"
#include "qmobject.h"
#include "qmobject_p.h"
#include "qmthread.h"
#include "qmthread_p.h"
#include "qmapplication.h"
#include "qmeventdispatcher.h"
#include "qmmutexlocker.h"

QmObjectPrivate::QmObjectPrivate(QmObject *q) :
	q_ptr(q), thread(QmThread::currentThread()), pending_delete(false),
	parent(0), ta_mutex(QmMutex::Recursive), drop_events(false)
{
}

QmObjectPrivate::~QmObjectPrivate()
{
}

bool QmObjectPrivate::deliverEvent(QmEvent *event) {
	if (!drop_events)
		return q_ptr->event(event);
	return false;
}

void QmObjectPrivate::moveToThread(QmThread* thread) {
	QmEventDispatcher *source_event_dispatcher, *target_event_dispatcher;
	ta_mutex.lock();
	source_event_dispatcher = this->thread->d_func()->event_dispatcher;
	target_event_dispatcher = thread->d_func()->event_dispatcher;
	target_event_dispatcher->blockProcessing();
	lockAndAssignOtherThreadRecursively(q_ptr, thread, true);
	unlockRecursively(q_ptr, true);
	ta_mutex.unlock();
	QmEventDispatcher::moveQueuedEvents(q_ptr, source_event_dispatcher, target_event_dispatcher);
	target_event_dispatcher->unblockProcessing();
	target_event_dispatcher->wakeUp();
}

void QmObjectPrivate::lockAndAssignOtherThreadRecursively(QmObject *object, QmThread* other_thread, bool is_root) {
	if (!is_root)
		object->d_ptr->ta_mutex.lock();
	object->d_ptr->thread = other_thread;
	for (auto& i : object->d_ptr->children)
		lockAndAssignOtherThreadRecursively(i, other_thread, false);
}

void QmObjectPrivate::unlockRecursively(QmObject *object, bool is_root) {
	for (auto& i : object->d_ptr->children)
		unlockRecursively(i, false);
	if (!is_root)
		object->d_ptr->ta_mutex.unlock();
}

void QmObjectPrivate::cleanup() {
	drop_events = true;
	for (auto& i : children)
		delete i;
	QmApplication::removePostedEvents(q_ptr);
}

QmObjectList QmObject::children() const {
	QM_D(const QmObject);
	return d->children;
}

void QmObject::setParent(QmObject *parent) {
	QM_D(QmObject);
	if (parent)
		if (thread() != parent->thread()) {
			qmDebugMessage(QmDebug::Error, "QmObject::setParent(0x%p): parent(0x%p) belongs to different thread", this, parent);
			return;
		}
	if (d->parent) {
		auto &parent_children = d->parent->d_ptr->children;
		for (auto i = parent_children.begin(); i != parent_children.end(); ++i) {
			if (*i == this) {
				parent_children.erase(i);
				break;
			}
		}
	}
	if (parent)
		parent->d_ptr->children.push_back(this);
	d->parent = parent;
}

QmObject* QmObject::parent() const {
	QM_D(const QmObject);
	return d->parent;
}

void QmObject::deleteLater() {
	QM_D(QmObject);
	if (d->pending_delete)
		return;
	d->pending_delete = true;
	QmApplication::postEvent(this, new QmEvent(QmEvent::DeferredDelete));
}

QmThread* QmObject::thread() const {
	QM_D(const QmObject);
	QmMutexLocker locker(&(d->ta_mutex));
	return d->thread;
}

bool QmObject::event(QmEvent* event) {
	QM_D(QmObject);
	if (event->type() == QmEvent::DeferredDelete) {
		QM_ASSERT(d->pending_delete);
		delete this;
		return true;
	}
	return false;
}
