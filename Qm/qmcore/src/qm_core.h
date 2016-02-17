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

#ifdef QM_PLATFORM_STM32F2XX
#include "FreeRTOS.h"
#include "dl_list.h"
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
#include <qevent.h>
#endif /* QM_PLATFORM_QT */

#ifdef QM_PLATFORM_STM32F2XX
class QmSystemEvent;
DLLIST_TYPEDEF_LIST(class QmSystemEvent, qm_core_system_queue)
void qmcoreProcessQueuedSystemEvents();
class QmSystemEvent {
private:
	DLLIST_ELEMENT_FIELDS(class QmSystemEvent, qm_core_system_queue)
public:
	QmSystemEvent();
	virtual ~QmSystemEvent();
	void setPending();
	void setPendingFromISR(signed portBASE_TYPE *pxHigherPriorityTaskWoken);
protected:
	virtual void process() = 0;
private:
	friend void qmcoreProcessQueuedSystemEvents();
	void checkAndPostToQueue();
	void removeFromQueue();
	bool pending;
};
#endif /* QM_PLATFORM_STM32F2XX */

#ifdef QM_PLATFORM_QT
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
#endif /* QM_PLATFORM_QT */

#ifdef QM_PLATFORM_QT
struct qmcore_global_environment_t {
	int argc;
	char **argv;
	int exit_code;
};
#endif /* QM_PLATFORM_QT */

#ifdef QM_PLATFORM_QT
extern qmcore_global_environment_t qmcore_global_environment;
#endif /* QM_PLATFORM_QT */

void qmMain();

#endif /* QM_CORE_H_ */
