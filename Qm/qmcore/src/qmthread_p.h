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

#ifdef QM_PLATFORM_STM32F2XX
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "qmeventloop.h"
#endif /* QM_PLATFORM_STM32F2XX */
#include "qmobject_p.h"

#ifdef QM_PLATFORM_QT
class QThread;
#endif /* QM_PLATFORM_QT */
#ifdef QM_PLATFORM_STM32F2XX
class QmEventDispatcher;
#endif /* QM_PLATFORM_STM32F2XX */

class QmThreadPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmThread)
public:
	QmThreadPrivate(QmThread *q);
	virtual ~QmThreadPrivate();
#ifdef QM_PLATFORM_QT
	bool is_main;
	QThread *qt_adapter;
#endif /* QM_PLATFORM_QT */
#ifdef QM_PLATFORM_STM32F2XX
	QmEventDispatcher *event_dispatcher;
	QmEventLoop event_loop;
	TaskHandle_t task_handle;
	SemaphoreHandle_t sync_semaphore;
#endif /* QM_PLATFORM_STM32F2XX */
	bool running;
#ifdef QM_PLATFORM_QT
	friend class QmThreadPrivateAdapter;
#endif /* QM_PLATFORM_QT */
#ifdef QM_PLATFORM_STM32F2XX
	void taskFunction();
#endif /* QM_PLATFORM_STM32F2XX */
};

#endif // QMTHREAD_P_H
