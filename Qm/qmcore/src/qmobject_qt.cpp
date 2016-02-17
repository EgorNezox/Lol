/**
  ******************************************************************************
  * @file    qmobject_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include <qobject.h>
#include <qevent.h>

#include "qmobject.h"
#include "qmobject_p.h"
#include "qm_core.h"
#include "qmthread.h"
#include "qmthread_p.h"

class Object : public QObject
{
	Q_OBJECT
public:
	Object(QmObjectPrivate *qmprivateobject): QObject(0), qmprivateobject(qmprivateobject) {}
	~Object() {
		QmObject *qmobject = qmprivateobject->q_ptr;
		if (qmprivateobject->pending_delete)
			return;
		qmprivateobject = 0;
		delete qmobject;
	}
	QmObjectPrivate *qmprivateobject;
	bool event(QEvent *e) {
		if ((qmprivateobject) && (QmCoreEvent::typeMatch(e))) {
			return qmprivateobject->deliverEvent(static_cast<QmCoreEvent *>(e)->qmevent);
		}
		return QObject::event(e);
	}
};

QmObjectPrivate::QmObjectPrivate(QmObject *q) :
	q_ptr(q), thread(QmThread::currentThread()), pending_delete(false),
	qobject(new Object(this))
{
}

QmObjectPrivate::~QmObjectPrivate()
{
	if (!pending_delete) {
		pending_delete = true;
		delete qobject;
	}
}

bool QmObjectPrivate::deliverEvent(QmEvent *event) {
	return q_ptr->event(event);
}

void QmObjectPrivate::moveToThread(QmThread* thread) {
	assignOtherThreadRecursively(q_ptr, thread);
	qobject->moveToThread(thread->d_func()->qt_adapter);
}

void QmObjectPrivate::assignOtherThreadRecursively(QmObject *object, QmThread* other_thread) {
	object->d_ptr->thread = other_thread;
	Q_FOREACH (const QObject* qt_object, object->d_ptr->qobject->children())
		assignOtherThreadRecursively(static_cast<const Object *>(qt_object)->qmprivateobject->q_ptr, other_thread);
}

void QmObjectPrivate::cleanup() {
}

QmObjectList QmObject::children() const {
	QM_D(const QmObject);
	QmObjectList result;
	Q_FOREACH (const QObject* qt_object, d->qobject->children())
		result.push_back(static_cast<const Object *>(qt_object)->qmprivateobject->q_ptr);
	return result;
}

void QmObject::setParent(QmObject *parent) {
	QM_D(QmObject);
	if (parent)
		d->qobject->setParent(parent->d_ptr->qobject);
	else
		d->qobject->setParent(0);
}

QmObject* QmObject::parent() const {
	QM_D(const QmObject);
	const Object *object = static_cast<const Object*>(d->qobject->parent());
	if (object == 0)
		return 0;
	return object->qmprivateobject->q_ptr;
}

void QmObject::deleteLater() {
	QM_D(QmObject);
	d->qobject->deleteLater();
}

QmThread* QmObject::thread() const {
	QM_D(const QmObject);
	return d->thread;
}

bool QmObject::event(QmEvent* event) {
	QM_UNUSED(event);
	return false;
}

#include "qmobject_qt.moc"
