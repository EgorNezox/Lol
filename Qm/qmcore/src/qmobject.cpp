/**
  ******************************************************************************
  * @file    qmobject.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#define QMDEBUGDOMAIN QmCore
#include "qm.h"
#include "qmdebug.h"
#include "qmobject.h"
#include "qmobject_p.h"

QmObject::QmObject(QmObject *parent) :
	d_ptr(new QmObjectPrivate(this))
{
	setParent(parent);
}

QmObject::QmObject(QmObjectPrivate& dd, QmObject *parent) :
	d_ptr(&dd)
{
	setParent(parent);
}

QmObject::~QmObject() {
	QM_D(QmObject);
	if (!d->pending_delete)
		d->pending_delete = true;
	destroyed.emit(this);
	d->cleanup();
	delete d_ptr;
}

QmThread* QmObject::thread() const {
	QM_D(const QmObject);
	return d->thread;
}

void QmObject::moveToThread(QmThread* thread) {
	QM_D(QmObject);
	QM_ASSERT(thread);
	if (parent()) {
		qmDebugMessage(QmDebug::Error, "QmObject::moveToThread(0x%p): failed (object has a parent)", this);
		return;
	}
	d->moveToThread(thread);
}
