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

#ifdef QMCORE_PLATFORM_BMFREERTOS
#include "FreeRTOS.h"
#include "timers.h"
#endif /* QMCORE_PLATFORM_BMFREERTOS */

#include "qm.h"
#include "qmobject_p.h"

#ifdef QMCORE_PLATFORM_QT
class CoreTimer;
#endif /* QMCORE_PLATFORM_QT */

class QmTimerPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmTimer)
public:
	QmTimerPrivate(QmTimer *q);
	virtual ~QmTimerPrivate();
	void init(bool single_shot);
	void deinit();
#ifdef QMCORE_PLATFORM_BMFREERTOS
	void postTimeoutEvent();
	void callback();
	TimerHandle_t timerhandle;
	bool is_single_shot;
	bool awaiting_callback;
#endif /* QMCORE_PLATFORM_BMFREERTOS */
#ifdef QMCORE_PLATFORM_QT
	void processTimeout();
	CoreTimer *qtimer;
#endif /* QMCORE_PLATFORM_QT */
};

#endif /* QMTIMER_P_H_ */
