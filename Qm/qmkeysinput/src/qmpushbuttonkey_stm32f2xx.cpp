/**
  ******************************************************************************
  * @file    qmpushbuttonkey_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @author  Petr Dmitriev
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#define QMDEBUGDOMAIN QmPushButtonKey

#include "system_hw_io.h"

#include "qmdebug.h"
#include "qmpushbuttonkey.h"
#include "qmpushbuttonkey_p.h"
#include "qmevent.h"
#include "qmapplication.h"

#define QMPBKEY_DEBOUNCE_DELAY 10

static void qmpushbuttonkeyStateChangedIsrCallback(hal_exti_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	QmPushButtonKeyPrivate* qmpbkeyPrivate_ptr = static_cast<QmPushButtonKeyPrivate*>(hal_exti_get_userid(handle));
	hal_exti_close(handle);
	hal_timer_start(qmpbkeyPrivate_ptr->getDebounceTimer(), QMPBKEY_DEBOUNCE_DELAY, pxHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(*pxHigherPriorityTaskWoken);
}

static void qmpushbuttonkeyDebounceTimerFinished(hal_timer_handle_t handle) {
	QmPushButtonKeyPrivate* qmpbkeyPrivate_ptr = static_cast<QmPushButtonKeyPrivate*>(hal_timer_get_userid(handle));
	qmpbkeyPrivate_ptr->extiEnable();
	qmpbkeyPrivate_ptr->postPbStateChangedEvent();
}

QmPushButtonKeyPrivate::QmPushButtonKeyPrivate(QmPushButtonKey *q) :
	QmObjectPrivate(q),
	hw_resource(-1), updated_state(false),
	exti_line(-1), exti_handle(0), event_posting_available(true)
{
	hal_exti_set_default_params(&exti_params);

	hal_timer_params_t params;
	params.userid = static_cast<void*>(this);
	params.callbackTimeout = qmpushbuttonkeyDebounceTimerFinished;
	debounce_timer = hal_timer_create(&params);
}

QmPushButtonKeyPrivate::~QmPushButtonKeyPrivate()
{
}

void QmPushButtonKeyPrivate::postPbStateChangedEvent() {
	QM_Q(QmPushButtonKey);
	if (event_posting_available) {
		event_posting_available = false;
		QmApplication::postEvent(q, new QmEvent(QmEvent::KeysInput));
	}
}

void QmPushButtonKeyPrivate::extiEnable() {
	exti_handle = hal_exti_open(exti_line, &exti_params);
}

hal_timer_handle_t QmPushButtonKeyPrivate::getDebounceTimer() {
	return debounce_timer;
}

void QmPushButtonKeyPrivate::init() {
	gpio_pin = stm32f2_get_gpio_pin(hw_resource);
	exti_line = stm32f2_get_exti_line(hw_resource);
	QM_ASSERT(exti_line != -1);
	stm32f2_ext_pins_init(hw_resource);
	exti_params.mode = hextiMode_Rising_Falling;
	exti_params.isrcallbackTrigger = qmpushbuttonkeyStateChangedIsrCallback;
	exti_params.userid = static_cast<void*>(this);
	extiEnable();
	updated_state = isGpioPressed();
}

void QmPushButtonKeyPrivate::deinit() {
	QM_Q(QmPushButtonKey);
	if (exti_line != -1) {
		hal_exti_close(exti_handle);
		hal_exti_set_default_params(&exti_params);
		QmApplication::removePostedEvents(q, QmEvent::KeysInput);
	}
	stm32f2_ext_pins_deinit(hw_resource);
}

bool QmPushButtonKeyPrivate::isGpioPressed() {
	if (hal_gpio_get_input(gpio_pin) == stm32f2_get_pushbutton_active_level(hw_resource))
		return true;
	else
		return false;
}

bool QmPushButtonKey::event(QmEvent* event) {
	QM_D(QmPushButtonKey);
	if (event->type() == QmEvent::KeysInput) {
		d->event_posting_available = true;
		bool current_state = d->isGpioPressed();
		if (d->updated_state != current_state) {
			d->updated_state = current_state;
			qmDebugMessage(QmDebug::Dump, "stateChanged id=0x%X", (int)this);
			stateChanged();
		}
		return true;
	}
	return QmObject::event(event);
}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(QmPushButtonKey, LevelDefault)
#include "qmdebug_domains_end.h"
