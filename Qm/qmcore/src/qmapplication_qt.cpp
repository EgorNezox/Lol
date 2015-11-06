/**
  ******************************************************************************
  * @file    qmapplication_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include <hardware_emulation.h>
#include <QApplication>
#include "qmevent.h"
#include "qm_core.h"
#include "qmapplication.h"
#include "qmapplication_p.h"

QmApplicationPrivate::QmApplicationPrivate(QmApplication *q) :
	QmObjectPrivate(q), app(qmcore_global_environment.argc, qmcore_global_environment.argv)
{
	QtHwEmu::init();
}

QmApplicationPrivate::~QmApplicationPrivate()
{
	QtHwEmu::deinit();
}

void QmApplicationPrivate::convertPostedEventsArguments(const QmObject* receiver, const int event_type, QObject* &qt_object_receiver, int &qt_event_type) {
	qt_object_receiver = 0;
	qt_event_type = 0;
	if (receiver)
		qt_object_receiver = receiver->d_func()->qobject;
	if (event_type > 0)
		qt_event_type = static_cast<int>(QmCoreEvent::toQtEventType(event_type));
}

void QmApplication::exec() {
	qmcore_global_environment.exit_code = qApp->exec();
}

bool QmApplication::sendEvent(QmObject* receiver, QmEvent* event) {
	QmCoreEvent core_event(event, false);
	return qApp->sendEvent(receiver->d_func()->qobject, &core_event);
}

void QmApplication::postEvent(QmObject* receiver, QmEvent* event) {
	qApp->postEvent(receiver->d_func()->qobject, new QmCoreEvent(event));
}

void QmApplication::sendPostedEvents(QmObject* receiver, int event_type) {
	QObject *qt_object_receiver;
	int qt_event_type;
	QmApplicationPrivate::convertPostedEventsArguments(receiver, event_type, qt_object_receiver, qt_event_type);
	qApp->sendPostedEvents(qt_object_receiver, qt_event_type);
}

void QmApplication::removePostedEvents(QmObject* receiver, int event_type) {
	QObject *qt_object_receiver;
	int qt_event_type;
	QmApplicationPrivate::convertPostedEventsArguments(receiver, event_type, qt_object_receiver, qt_event_type);
	qApp->removePostedEvents(qt_object_receiver, qt_event_type);
}
