/**
  ******************************************************************************
  * @file    qmmutex_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include <qmutex.h>

#include "qmdebug.h"
#include "qmmutex.h"

class QmMutexPrivate {
public:
	bool locked;
	QMutex *m;
};

QmMutex::QmMutex(RecursionMode mode) :
	impl(new QmMutexPrivate())
{
	QMutex::RecursionMode qt_mode;
	impl->locked = false;
	switch (mode) {
	case NonRecursive: qt_mode = QMutex::NonRecursive; break;
	case Recursive: qt_mode = QMutex::Recursive; break;
	default: QM_ASSERT(0);
	}
	impl->m = new QMutex(qt_mode);
}

QmMutex::~QmMutex() {
	QM_ASSERT(!impl->locked);
	delete impl->m;
	delete impl;
}

void QmMutex::lock() {
	impl->m->lock();
	QM_ASSERT(!impl->locked);
	impl->locked = true;
}

void QmMutex::unlock() {
	QM_ASSERT(impl->locked);
	impl->locked = false;
	impl->m->unlock();
}
