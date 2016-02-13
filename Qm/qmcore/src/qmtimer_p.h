/**
  ******************************************************************************
  * @file    qmtimer_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMTIMER_P_H_
#define QMTIMER_P_H_

#ifdef QM_PLATFORM_STM32F2XX
#include "hal_timer.h"
#endif /* QM_PLATFORM_STM32F2XX */

#include "qm.h"
#include "qmobject_p.h"

#ifdef QM_PLATFORM_QT
class QmTimerPrivateAdapter;
#endif /* QM_PLATFORM_QT */

class QmTimerPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmTimer)
public:
	QmTimerPrivate(QmTimer *q);
	virtual ~QmTimerPrivate();
	void init(bool single_shot);
	void deinit();
	bool is_active;
	unsigned int interval_value;
#ifdef QM_PLATFORM_STM32F2XX
	void postTimeoutEvent();
	hal_timer_handle_t timerhandle;
	bool is_single_shot;
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
	friend class QmTimerPrivateAdapter;
	QmTimerPrivateAdapter *qt_adapter;
#endif /* QM_PLATFORM_QT */
};

#endif /* QMTIMER_P_H_ */
