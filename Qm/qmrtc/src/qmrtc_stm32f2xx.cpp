/**
  ******************************************************************************
  * @file    qmrtc_stm32f2xx.cpp
  * @author  Petr Dmitriev
  * @date    23.11.2016
  *
  ******************************************************************************
 */

#include "system.h"
#include "system_hw_io.h"

#define QMDEBUGDOMAIN	QmRtc
#include "qmdebug.h"
#include "qmrtc_p.h"
#include "qmevent.h"
#include "qmapplication.h"

static void qmrtcExtiTriggerIsrCallback(hal_exti_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	QmRtcWakeupEvent *system_event = static_cast<QmRtcWakeupEvent *>(hal_exti_get_userid(handle));
	system_event->setPendingFromISR(pxHigherPriorityTaskWoken);
	hal_rtc_clear_wakeup_it_pending_bit();
}

QmRtcWakeupEvent::QmRtcWakeupEvent(QmRtc *o) :
	o(o)
{
}

void QmRtcWakeupEvent::process() {
	QmApplication::postEvent(o, new QmEvent(QmEvent::RtcWakeup));
}


QmRtcPrivate::QmRtcPrivate(QmRtc *q) :
	QmObjectPrivate(q),
	hw_resource(-1), exti_line(-1), exti_handle(0),
	wakeup_event(q)
{
}

QmRtcPrivate::~QmRtcPrivate()
{
}

void QmRtcPrivate::init() {
	exti_line = stm32f2_get_exti_line(hw_resource);
	hal_exti_params_t exti_params;
	exti_params.mode = hextiMode_Rising;
	exti_params.isrcallbackTrigger = qmrtcExtiTriggerIsrCallback;
	exti_params.userid = static_cast<void *>(&wakeup_event);
	exti_handle = hal_exti_open(exti_line, &exti_params);
	hal_rtc_init();
}

void QmRtcPrivate::deinit() {
	hal_exti_close(exti_handle);
}

bool QmRtc::event(QmEvent* event) {
	if (event->type() == QmEvent::RtcWakeup) {
		qmDebugMessage(QmDebug::Dump, "RTC Wakeup");
		pps();
		return true;
	}
	return QmObject::event(event);
}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(QmRtc, LevelDefault)
#include "qmdebug_domains_end.h"
