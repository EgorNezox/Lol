/**
  ******************************************************************************
  * @file    qmapplication_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMAPPLICATION_P_H
#define QMAPPLICATION_P_H

#include "qmobject_p.h"
#ifdef QM_PLATFORM_QT
#include <qapplication.h>
#endif /* QM_PLATFORM_QT */
#ifdef QM_PLATFORM_STM32F2XX
#include "qmeventloop.h"
#endif /* QM_PLATFORM_STM32F2XX */

class QmApplicationPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmApplication)
public:
	QmApplicationPrivate(QmApplication *q);
	virtual ~QmApplicationPrivate();
#ifdef QM_PLATFORM_QT
	static void convertPostedEventsArguments(const QmObject* receiver, const int event_type, QObject* &qt_object_receiver, int &qt_event_type);
	QApplication app;
#endif /* QM_PLATFORM_QT */
#ifdef QM_PLATFORM_STM32F2XX
	QmEventLoop event_loop;
#endif /* QM_PLATFORM_STM32F2XX */
};

#endif // QMAPPLICATION_P_H
