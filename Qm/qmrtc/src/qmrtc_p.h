/**
  ******************************************************************************
  * @file    qmrtc_p.h
  * @author  Petr Dmitriev
  * @date    23.11.2016
  *
  ******************************************************************************
 */

#ifndef QM_QMRTC_SRC_QMRTC_P_H_
#define QM_QMRTC_SRC_QMRTC_P_H_

#include "../../qmcore/src/qmobject_p.h"
#include "qmrtc.h"

#ifdef QM_PLATFORM_STM32F2XX
//#include "hal_gpio.h"
#include "hal_exti.h"
#include "hal_rtc.h"
#include "../../qmcore/src/qm_core.h"
#endif /* QM_PLATFORM_STM32F2XX */

#ifdef QM_PLATFORM_STM32F2XX
class QmRtcWakeupEvent : public QmSystemEvent
{
public:
	QmRtcWakeupEvent(QmRtc *o);
private:
	QmRtc *o;
	void process();
};
#endif /* QM_PLATFORM_STM32F2XX */

class QmRtcPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmRtc)
public:
	QmRtcPrivate(QmRtc *q);
	virtual ~QmRtcPrivate();
private:
	void init();
	void deinit();
	int hw_resource;
#ifdef QM_PLATFORM_STM32F2XX
	int exti_line;
	hal_exti_handle_t exti_handle;
	QmRtcWakeupEvent wakeup_event;
#endif /* QM_PLATFORM_STM32F2XX */
};

#endif /* QM_QMRTC_SRC_QMRTC_P_H_ */
