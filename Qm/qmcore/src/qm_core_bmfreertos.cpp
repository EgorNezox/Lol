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

#include "qm.h"
#include "qmdebug.h"
#include "qm_core.h"
#include "qmthread.h"
#include "qmthread_p.h"

class QmSystemThread : public QmThread
{
public:
	QmSystemThread() : QmThread("qmsystem") {
		QM_D(QmThread);
		QM_ASSERT(self == 0);
		self = this;
		d->startSystemPriority();
	}
	void start(Priority priority) {
		QM_UNUSED(priority);
		QM_ASSERT(0); // cannot be started in such way
	}
	void exit(int return_code) {
		QM_UNUSED(return_code);
		QM_ASSERT(0); // thread isn't allowed to exit
	}
	static QmSystemThread *self;
};

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

QmSystemThread* QmSystemThread::self = 0;
QmMainThread* QmMainThread::self = 0;

int main(void) {
	QM_ASSERT(configTICK_RATE_HZ <= 1000); // QmTimer implementation requirement
	new QmSystemThread();
	new QmMainThread();
	/* Запуск планировщика задач, после чего управление передается главному потоку (qmMain) */
	vTaskStartScheduler();
	QM_ASSERT(0); // FreeRTOS scheduler failed to start (out of memory ?) or finished unexpectedly
	while(1); // never return
	return 0;
}

QmThread* qmGetSystemThread() {
	return QmSystemThread::self;
}
