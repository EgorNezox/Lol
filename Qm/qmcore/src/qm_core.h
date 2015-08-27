/**
  ******************************************************************************
  * @file    qm_core.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QM_CORE_H_
#define QM_CORE_H_

#ifdef QMCORE_PLATFORM_QT
#include <QEvent>
#endif /* QMCORE_PLATFORM_QT */

#ifdef QMCORE_PLATFORM_BMFREERTOS
class QmThread;
#endif /* QMCORE_PLATFORM_BMFREERTOS */

#ifdef QMCORE_PLATFORM_QT
class QmEvent;
class QmCoreEvent : public QEvent
{
public:
	QmCoreEvent(QmEvent *qmevent, bool auto_destroy = true);
	~QmCoreEvent();
	static inline QEvent::Type toQtEventType(int qm_event_type) {
		return static_cast<QEvent::Type>(static_cast<int>(QEvent::User) + qm_event_type);
	}
	static inline bool typeMatch(QEvent *qt_event) {
		return ((QEvent::User <= qt_event->type()) && (qt_event->type() <= QEvent::MaxUser));
	}
	QmEvent *qmevent;
	bool auto_destroy;
};
#endif /* QMCORE_PLATFORM_QT */

#ifdef QMCORE_PLATFORM_QT
struct qmcore_global_environment_t {
	int argc;
	char **argv;
	int exit_code;
};
#endif /* QMCORE_PLATFORM_QT */

#ifdef QMCORE_PLATFORM_QT
extern qmcore_global_environment_t qmcore_global_environment;
#endif /* QMCORE_PLATFORM_QT */

void qmMain();
#ifdef QMCORE_PLATFORM_BMFREERTOS
QmThread* qmGetSystemThread();
#endif /* QMCORE_PLATFORM_BMFREERTOS */

#endif /* QM_CORE_H_ */
