/**
  ******************************************************************************
  * @file    qmthread_bmfreertos.cpp
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
	QmObject(*new QmThreadPrivate(this), parent)
{
	QM_D(QmThread);
	d->init(name);
}

QmThread::QmThread(QmThreadPrivate& dd, const char * const name, QmObject* parent) :
	QmObject(dd, parent)
{
	QM_D(QmThread);
	d->init(name);
}

QmThread::~QmThread() {
	QM_D(QmThread);
	QM_ASSERT(!d->running);
	d->deinit();
}

void QmThreadPrivate::init(const char * const name) {
	QM_Q(QmThread);
	event_dispatcher = new QmEventDispatcher(q);
	sync_semaphore = xSemaphoreCreateBinary();
	xTaskCreate(qmthreadTaskEntry, name, usertaskDEFAULT_STACK_SIZE, (void *)this, tskIDLE_PRIORITY, &task_handle);
	registered_threads_mutex.lock();
	registered_threads[task_handle] = q;
	registered_threads_mutex.unlock();
}

void QmThreadPrivate::deinit() {
	QM_Q(QmThread);
	registered_threads_mutex.lock();
	for (auto i = registered_threads.begin(); i != registered_threads.end(); i++) {
		if (q == (*i).second) {
			registered_threads.erase(i);
			break;
		}
	}
	registered_threads_mutex.unlock();
	vTaskDelete(task_handle);
	vSemaphoreDelete(sync_semaphore);
	delete event_dispatcher;
}

void QmThreadPrivate::startSystemPriority() {
	QM_ASSERT(!running);
	vTaskPrioritySet(task_handle, usertaskSYSTEM_PRIORITY);
	running = true;
	xSemaphoreGive(sync_semaphore);
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

void QmThread::start(Priority priority) {
	QM_D(QmThread);
	UBaseType_t freertos_priority = tskIDLE_PRIORITY;
	switch (priority) {
	case LowPriority: freertos_priority = usertaskAPP_LOW_PRIORITY; break;
	case NormalPriority: freertos_priority = usertaskAPP_NORMAL_PRIORITY; break;
	case HighPriority: freertos_priority = usertaskAPP_HIGH_PRIORITY; break;
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
