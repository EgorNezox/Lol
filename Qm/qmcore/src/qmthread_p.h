/**
  ******************************************************************************
  * @file    qmthread_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMTHREAD_P_H
#define QMTHREAD_P_H

#ifdef QMCORE_PLATFORM_BMFREERTOS
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "qmeventloop.h"
#endif /* QMCORE_PLATFORM_BMFREERTOS */
#include "qmobject_p.h"

#ifdef QMCORE_PLATFORM_QT
class QThread;
#endif /* QMCORE_PLATFORM_QT */
#ifdef QMCORE_PLATFORM_BMFREERTOS
class QmEventDispatcher;
#endif /* QMCORE_PLATFORM_BMFREERTOS */

class QmThreadPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmThread)
public:
	QmThreadPrivate(QmThread *q);
	virtual ~QmThreadPrivate();
#ifdef QMCORE_PLATFORM_QT
	bool is_main;
	QThread *qt_adapter;
#endif /* QMCORE_PLATFORM_QT */
#ifdef QMCORE_PLATFORM_BMFREERTOS
	QmEventDispatcher *event_dispatcher;
	QmEventLoop event_loop;
	TaskHandle_t task_handle;
	SemaphoreHandle_t sync_semaphore;
#endif /* QMCORE_PLATFORM_BMFREERTOS */
	bool running;
#ifdef QMCORE_PLATFORM_QT
	friend class QmThreadPrivateAdapter;
#endif /* QMCORE_PLATFORM_QT */
#ifdef QMCORE_PLATFORM_BMFREERTOS
	void taskFunction();
#endif /* QMCORE_PLATFORM_BMFREERTOS */
};

#endif // QMTHREAD_P_H
