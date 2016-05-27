/**
  ******************************************************************************
  * @file    qmabstimer_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
  */

#ifndef QMABSTIMER_P_H_
#define QMABSTIMER_P_H_

#ifdef QM_PLATFORM_STM32F2XX
#include "hal_timer.h"
#endif /* QM_PLATFORM_STM32F2XX */

#include "qm.h"
#include "qmobject_p.h"

#ifdef QM_PLATFORM_QT
class QmAbsTimerPrivateAdapter;
#endif /* QM_PLATFORM_QT */

class QmAbsTimerPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmAbsTimer)
public:
	QmAbsTimerPrivate(QmAbsTimer *q);
	virtual ~QmAbsTimerPrivate();
	void init();
	void deinit();
	bool is_active;
#ifdef QM_PLATFORM_STM32F2XX
	void postTimeoutEvent();
	hal_timer_handle_t timerhandle;
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
	friend class QmAbsTimerPrivateAdapter;
	QmAbsTimerPrivateAdapter *qt_adapter;
#endif /* QM_PLATFORM_QT */
};

#endif /* QMABSTIMER_P_H_ */
