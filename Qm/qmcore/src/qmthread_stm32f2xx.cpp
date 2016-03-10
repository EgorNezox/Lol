/**
  ******************************************************************************
  * @file    qmthread_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include <map>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define QMDEBUGDOMAIN QmCore
#include "qm.h"
#include "qmdebug.h"
#include "qmevent.h"
#include "qmobject_p.h"
#include "qmthread.h"
#include "qmthread_p.h"
#include "qmmutex.h"
#include "qmeventdispatcher.h"
#include "qmeventloop.h"
#include "qmapplication.h"

static QmMutex registered_threads_mutex;
static std::map<TaskHandle_t,QmThread*> registered_threads;

extern "C" {
static portTASK_FUNCTION(qmthreadTaskEntry, pvParameters) {
	((QmThreadPrivate *)pvParameters)->taskFunction();
}
}

QmThreadPrivate::QmThreadPrivate(QmThread *q) :
		QmObjectPrivate(q), event_dispatcher(0),
		task_handle(0), sync_semaphore(0), running(false)
{
}

QmThreadPrivate::~QmThreadPrivate()
{
}

QmThread::QmThread(const char * const name, QmObject *parent) :
	QmThread(*new QmThreadPrivate(this), name, parent)
{
}

QmThread::QmThread(QmThreadPrivate& dd, const char * const name, QmObject* parent) :
	QmObject(dd, parent)
{
	QM_D(QmThread);
	d->event_dispatcher = new QmEventDispatcher(this);
	d->sync_semaphore = xSemaphoreCreateBinary();
	xTaskCreate(qmthreadTaskEntry, name, qmconfigAPP_STACK_SIZE, (void *)d, tskIDLE_PRIORITY, &(d->task_handle));
	registered_threads_mutex.lock();
	registered_threads[d->task_handle] = this;
	registered_threads_mutex.unlock();
}

QmThread::~QmThread() {
	QM_D(QmThread);
	QM_ASSERT(!d->running);
	registered_threads_mutex.lock();
	for (auto i = registered_threads.begin(); i != registered_threads.end(); i++) {
		if (this == (*i).second) {
			registered_threads.erase(i);
			break;
		}
	}
	registered_threads_mutex.unlock();
	vTaskDelete(d->task_handle);
	vSemaphoreDelete(d->sync_semaphore);
	delete d->event_dispatcher;
}

void QmThreadPrivate::taskFunction() {
	QM_Q(QmThread);
	setvbuf(stdout, NULL, _IONBF, 0); // to prevent stdlib internal malloc() on first access to stdio
	while (xSemaphoreTake(sync_semaphore, portMAX_DELAY)) {
		QmEvent *finish_event = new QmEvent(QmEvent::ThreadFinishSync);
		q->run();
		QmApplication::sendPostedEvents(0, QmEvent::DeferredDelete);
		QmApplication::postEvent(q, finish_event);
	}
}

QmThread* QmThread::currentThread() {
	QmThread *t = 0;
	if ((xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)) {
		registered_threads_mutex.lock();
		t = registered_threads.at(xTaskGetCurrentTaskHandle());
		registered_threads_mutex.unlock();
	}
	return t;
}

void QmThread::msleep(unsigned int msecs) {
	vTaskDelay(msecs/portTICK_PERIOD_MS);
}

void QmThread::start(Priority priority) {
	QM_D(QmThread);
	UBaseType_t freertos_priority = tskIDLE_PRIORITY;
	switch (priority) {
	case LowPriority: freertos_priority = qmconfigAPP_LOW_PRIORITY; break;
	case NormalPriority: freertos_priority = qmconfigAPP_NORMAL_PRIORITY; break;
	case HighPriority: freertos_priority = qmconfigAPP_HIGH_PRIORITY; break;
	default: QM_ASSERT(0); break;
	}
	if (d->running) {
		qmDebugMessage(QmDebug::Warning, "QmThread::start(0x%p): already running", this);
		return;
	}
	vTaskPrioritySet(d->task_handle, freertos_priority);
	d->running = true;
	xSemaphoreGive(d->sync_semaphore);
	started.emit();
}

bool QmThread::isRunning() {
	QM_D(QmThread);
	return d->running;
}

bool QmThread::event(QmEvent* event) {
	QM_D(QmThread);
	if (event->type() == QmEvent::ThreadFinishSync) {
		QM_ASSERT(d->running);
		d->running = false;
		finished.emit();
		return true;
	}
	return QmObject::event(event);
}

void QmThread::run() {
	exec();
}

int QmThread::exec() {
	QM_D(QmThread);
	return d->event_loop.exec();
}

void QmThread::exit(int return_code) {
	QM_D(QmThread);
	d->event_loop.exit(return_code);
}

void QmThread::quit() {
	exit(0);
}
