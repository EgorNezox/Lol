/**
  ******************************************************************************
  * @file    qmmutexlocker.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    29.09.2015
  *
  ******************************************************************************
  */

#include "qmdebug.h"
#include "qmmutexlocker.h"

QmMutexLocker::QmMutexLocker(QmMutex *mutex) :
	mutex(mutex)
{
	QM_ASSERT(mutex);
	relock();
}

QmMutexLocker::~QmMutexLocker() {
	unlock();
}

void QmMutexLocker::relock() {
	mutex->lock();
}

void QmMutexLocker::unlock() {
	mutex->unlock();
}
