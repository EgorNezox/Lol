/**
  ******************************************************************************
  * @file    qmiopin_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "system_hw_io.h"

#include "qmdebug.h"
#include "qmiopin_p.h"
#include "qmevent.h"
#include "qmapplication.h"

static void qmiopinExtiTriggerIsrCallback(hal_exti_handle_t handle, void *userid, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	QM_UNUSED(handle);
	QmIoPinTriggerEvent *system_event = static_cast<QmIoPinTriggerEvent *>(userid);
	system_event->setPendingFromISR(pxHigherPriorityTaskWoken);
}

QmIoPinTriggerEvent::QmIoPinTriggerEvent(QmIopin *o) :
	o(o)
{
}

void QmIoPinTriggerEvent::process() {
	QmApplication::postEvent(o, new QmEvent(QmEvent::HardwareIO));
}

QmIopinPrivate::QmIopinPrivate(QmIopin *q) :
	QmObjectPrivate(q),
	hw_resource(-1), input_trigger_mode(QmIopin::InputTrigger_Disabled),
	exti_line(-1), exti_handle(0), trigger_event(q)
{
}

QmIopinPrivate::~QmIopinPrivate()
{
}

void QmIopinPrivate::init() {
	gpio_pin = stm32f2_get_gpio_pin(hw_resource);
	exti_line = stm32f2_get_exti_line(hw_resource);
	stm32f2_ext_pins_init(hw_resource);
}

void QmIopinPrivate::deinit() {
	if ((exti_line != -1) && (input_trigger_mode != QmIopin::InputTrigger_Disabled))
		hal_exti_deinit(exti_handle);
	stm32f2_ext_pins_deinit(hw_resource);
}

bool QmIopin::setInputTriggerMode(LevelTriggerMode mode) {
	QM_D(QmIopin);
	if (d->exti_line == -1)
		return false;
	d->input_trigger_mode = mode;
	if (mode != InputTrigger_Disabled) {
		hal_exti_params_t exti_params;
		switch (mode) {
		case InputTrigger_Disabled: QM_ASSERT(0); break;
		case InputTrigger_Rising: exti_params.mode = hextiMode_Rising; break;
		case InputTrigger_Falling: exti_params.mode = hextiMode_Falling; break;
		case InputTrigger_Both: exti_params.mode = hextiMode_Rising_Falling; break;
		}
		exti_params.isrcallbackTrigger = qmiopinExtiTriggerIsrCallback;
		exti_params.userid = static_cast<void *>(&d->trigger_event);
		d->exti_handle = hal_exti_init(d->exti_line, &exti_params);
	} else {
		hal_exti_deinit(d->exti_handle);
		QmApplication::removePostedEvents(this, QmEvent::HardwareIO);
	}
	return true;
}

QmIopin::Level QmIopin::readInput() {
	QM_D(QmIopin);
	hal_gpio_level_t value = hal_gpio_get_input(d->gpio_pin);
	return (value == hgpioLow)?Level_Low:Level_High;
}

void QmIopin::writeOutput(Level level) {
	QM_D(QmIopin);
	hal_gpio_level_t value = (level == Level_Low)?hgpioLow:hgpioHigh;
	hal_gpio_set_output(d->gpio_pin, value);
}

bool QmIopin::event(QmEvent* event) {
	if (event->type() == QmEvent::HardwareIO) {
		inputTrigger();
		return true;
	}
	return QmObject::event(event);
}
