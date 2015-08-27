/**
  ******************************************************************************
  * @file    qmmutex_bmfreertos.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "qmdebug.h"
#include "qmmutex.h"

class QmMutexPrivate {
public:
	QmMutex::RecursionMode mode;
	bool locked;
	SemaphoreHandle_t handle;
};

QmMutex::QmMutex(RecursionMode mode) :
	impl(new QmMutexPrivate())
{
	impl->locked = false;
	impl->mode = mode;
	switch (mode) {
	case NonRecursive:
		impl->handle = xSemaphoreCreateMutex();
		break;
	case Recursive:
		impl->handle = xSemaphoreCreateRecursiveMutex();
		break;
	default:
		QM_ASSERT(0);
	}
}

QmMutex::~QmMutex() {
	QM_ASSERT(!impl->locked);
	vSemaphoreDelete(impl->handle);
	delete impl;
}

void QmMutex::lock() {
	if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
		switch (impl->mode) {
		case NonRecursive:
			xSemaphoreTake(impl->handle, portMAX_DELAY);
			break;
		case Recursive:
			xSemaphoreTakeRecursive(impl->handle, portMAX_DELAY);
			break;
		}
	}
	QM_ASSERT(!impl->locked);
	impl->locked = true;
}

void QmMutex::unlock() {
	QM_ASSERT(impl->locked);
	impl->locked = false;
	if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
		switch (impl->mode) {
		case NonRecursive:
			xSemaphoreGive(impl->handle);
			break;
		case Recursive:
			xSemaphoreGiveRecursive(impl->handle);
			break;
		}
	}
}
