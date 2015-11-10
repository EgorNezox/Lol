/**
  ******************************************************************************
  * @file    qm_core_bmfreertos.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "qm.h"
#include "qmdebug.h"
#include "qm_core.h"
#include "qmthread.h"
#include "qmthread_p.h"

class QmMainThread : public QmThread
{
public:
	QmMainThread() : QmThread("qmmain") {
		QM_ASSERT(self == 0);
		self = this;
		start();
	}
	void run() {
		qmMain();
	}
	static QmMainThread *self;
};

extern "C" {
static portTASK_FUNCTION(qmsystemThreadEntry, pvParameters);
}

QmMainThread* QmMainThread::self = 0;
static QmSystemEvent *qm_sys_queue_top, *qm_sys_queue_bottom;
static SemaphoreHandle_t qm_sys_queue_semaphore;

QmSystemEvent::QmSystemEvent() :
	pending(false), previous(0), next(0)
{
	QM_ASSERT(uxTaskPriorityGet(NULL) < qmconfigSYSTEM_PRIORITY);
}

QmSystemEvent::~QmSystemEvent() {
	QM_ASSERT(uxTaskPriorityGet(NULL) < qmconfigSYSTEM_PRIORITY);
	portENTER_CRITICAL();
	if (pending)
		removeFromQueue();
	portEXIT_CRITICAL();
}

void QmSystemEvent::setPending() {
	portENTER_CRITICAL();
	checkAndPostToQueue();
	portEXIT_CRITICAL();
	xSemaphoreGive(qm_sys_queue_semaphore);
}

void QmSystemEvent::setPendingFromISR(signed long * pxHigherPriorityTaskWoken) {
	checkAndPostToQueue();
	xSemaphoreGiveFromISR(qm_sys_queue_semaphore, pxHigherPriorityTaskWoken);
}

void QmSystemEvent::checkAndPostToQueue() {
	if (pending)
		return;
	if (qm_sys_queue_bottom) {
		QM_ASSERT(qm_sys_queue_top != 0);
		previous = qm_sys_queue_bottom;
		qm_sys_queue_bottom = this;
	} else {
		QM_ASSERT(qm_sys_queue_top == 0);
		qm_sys_queue_bottom = this;
		qm_sys_queue_top = qm_sys_queue_bottom;
	}
	pending = true;
}

void QmSystemEvent::removeFromQueue() {
	QM_ASSERT(pending);
	if (previous) {
		QM_ASSERT(qm_sys_queue_top != 0);
		previous->next = next;
		previous = 0;
	} else {
		QM_ASSERT(qm_sys_queue_top == this);
		qm_sys_queue_top = next;
	}
	if (next) {
		QM_ASSERT(qm_sys_queue_bottom != 0);
		next->previous = previous;
		next = 0;
	} else {
		QM_ASSERT(qm_sys_queue_bottom == this);
		qm_sys_queue_bottom = previous;
	}
	pending = false;
}

static portTASK_FUNCTION(qmsystemThreadEntry, pvParameters) {
	while (xSemaphoreTake(qm_sys_queue_semaphore, portMAX_DELAY))
		qmcoreProcessQueuedSystemEvents();
}

void qmcoreProcessQueuedSystemEvents() {
	QmSystemEvent *next_event = 0;
	do {
		portENTER_CRITICAL();
		next_event = qm_sys_queue_top;
		if (next_event)
			next_event->removeFromQueue();
		portEXIT_CRITICAL();
		if (next_event)
			next_event->process();
	} while (next_event);
}

int main(void) {
	QM_ASSERT(configTICK_RATE_HZ <= 1000); // QmTimer implementation requirement
	qm_sys_queue_top = qm_sys_queue_bottom = 0;
	qm_sys_queue_semaphore = xSemaphoreCreateBinary();
	xTaskCreate(qmsystemThreadEntry, "qmsystem", qmconfigSYSTEM_STACK_SIZE, 0, qmconfigSYSTEM_PRIORITY, 0);
	new QmMainThread();
	/* Запуск планировщика задач, после чего управление передается главному потоку (qmMain) */
	vTaskStartScheduler();
	QM_ASSERT(0); // FreeRTOS scheduler failed to start (out of memory ?) or finished unexpectedly
	while(1); // never return
	return 0;
}
