/**
  ******************************************************************************
  * @file    qm_core_stm32f2xx.cpp
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

static_assert(sysconfigTIMER_TASK_PRIORITY > qmconfigSYSTEM_PRIORITY, ""); // requirement of hal timer interfacing implementation

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
static DLLIST_LIST_TYPE(qm_core_system_queue) qm_sys_queue;
static SemaphoreHandle_t qm_sys_queue_semaphore;

QmSystemEvent::QmSystemEvent() :
	DLLIST_ELEMENT_CLASS_INITIALIZER(qm_core_system_queue), pending(false)
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
	QM_ASSERT(DLLIST_IS_IN_LIST(qm_core_system_queue, 0, this));
	DLLIST_ADD_TO_LIST_BACK(qm_core_system_queue, &qm_sys_queue, this);
	pending = true;
}

void QmSystemEvent::removeFromQueue() {
	QM_ASSERT(pending);
	QM_ASSERT(DLLIST_IS_IN_LIST(qm_core_system_queue, &qm_sys_queue, this));
	DLLIST_REMOVE_FROM_LIST(qm_core_system_queue, &qm_sys_queue, this);
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
		next_event = DLLIST_GET_LIST_FRONT(&qm_sys_queue);
		if (next_event)
			next_event->removeFromQueue();
		portEXIT_CRITICAL();
		if (next_event)
			next_event->process();
	} while (next_event);
}

int main(void) {
	DLLIST_INIT_LIST(&qm_sys_queue);
	qm_sys_queue_semaphore = xSemaphoreCreateBinary();
	xTaskCreate(qmsystemThreadEntry, "qmsystem", qmconfigSYSTEM_STACK_SIZE, 0, qmconfigSYSTEM_PRIORITY, 0);
	new QmMainThread();
	/* Запуск планировщика задач, после чего управление передается главному потоку (qmMain) */
	vTaskStartScheduler();
	QM_ASSERT(0); // FreeRTOS scheduler failed to start (out of memory ?) or finished unexpectedly
	while(1); // never return
	return 0;
}
