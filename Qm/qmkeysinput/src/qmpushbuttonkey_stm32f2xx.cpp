/**
  ******************************************************************************
  * @file    qmpushbuttonkey_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @author  Petr Dmitriev
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "system_hw_io.h"

#include "qmdebug.h"
#include "qmpushbuttonkey.h"
#include "qmpushbuttonkey_p.h"
#include "qmevent.h"
#include "qmapplication.h"

#define QMPBKEY_DEBOUNCE_DELAY 20

static void qmpushbuttonkeyStateChangedIsrCallback(hal_exti_handle_t handle, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	hal_exti_close(handle);
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	QmPushButtonKeyPrivate* qmpbkeyPrivate_ptr = static_cast<QmPushButtonKeyPrivate*>(hal_exti_get_userid(handle));
	xTimerResetFromISR(*(qmpbkeyPrivate_ptr->getDebounceTimer()), &xHigherPriorityTaskWoken);
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void qmpushbuttonkeyDebounceTimerFinished(xTimerHandle xTimer) {
	QmPushButtonKeyPrivate* qmpbkeyPrivate_ptr = static_cast<QmPushButtonKeyPrivate*>(pvTimerGetTimerID(xTimer));
	qmpbkeyPrivate_ptr->extiEnable();
	qmpbkeyPrivate_ptr->postPbStateChangedEvent();
}

QmPushButtonKeyPrivate::QmPushButtonKeyPrivate(QmPushButtonKey *q) :
	QmObjectPrivate(q),
	hw_resource(-1), updated_state(false),
	exti_line(-1), exti_handle(0), event_posting_available(true)
{
	hal_exti_set_default_params(&exti_params);
	debounce_timer = xTimerCreate(static_cast<const char*>("QmPushButtonKeyDebounceTimer"),
			QMPBKEY_DEBOUNCE_DELAY / portTICK_RATE_MS, pdFALSE, static_cast<void*>(this), qmpushbuttonkeyDebounceTimerFinished);
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

xTimerHandle* QmPushButtonKeyPrivate::getDebounceTimer() {
	return &debounce_timer;
}

void QmPushButtonKeyPrivate::init() {
	gpio_pin = stm32f2_get_gpio_pin(hw_resource);
	exti_line = stm32f2_get_exti_line(hw_resource);
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
	if (hgpioLow == hal_gpio_get_input(gpio_pin))
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
			stateChanged();
		}
		return true;
	}
	return QmObject::event(event);
}
