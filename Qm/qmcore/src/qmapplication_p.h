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
#ifdef QMCORE_PLATFORM_QT
#include <QApplication>
#endif /* QMCORE_PLATFORM_QT */
#ifdef QMCORE_PLATFORM_BMFREERTOS
#include "qmeventloop.h"
#endif /* QMCORE_PLATFORM_BMFREERTOS */

class QmApplicationPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmApplication)
public:
	QmApplicationPrivate(QmApplication *q);
	virtual ~QmApplicationPrivate();
#ifdef QMCORE_PLATFORM_QT
	static void convertPostedEventsArguments(const QmObject* receiver, const int event_type, QObject* &qt_object_receiver, int &qt_event_type);
	QApplication app;
#endif /* QMCORE_PLATFORM_QT */
#ifdef QMCORE_PLATFORM_BMFREERTOS
	QmEventLoop event_loop;
#endif /* QMCORE_PLATFORM_BMFREERTOS */
};

#endif // QMAPPLICATION_P_H
