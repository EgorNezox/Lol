/**
  ******************************************************************************
  * @file    qmsmbushost_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    08.12.2015
  *
  ******************************************************************************
  */

#include <string.h>
#include "system_hw_io.h"

#include "qmdebug.h"
#include "qmsmbushost_p.h"
#include "qmevent.h"
#include "qmapplication.h"

QmSMBusHostIOEvent::QmSMBusHostIOEvent(QmSMBusHost *o) :
	o(o)
{
}

void QmSMBusHostIOEvent::process() {
	QmApplication::postEvent(o, new QmEvent(QmEvent::HardwareIO));
}

QmSMBusHostPrivate::QmSMBusHostPrivate(QmSMBusHost *q) :
	QmObjectPrivate(q),
	hw_resource(-1),
	i2c_bus_instance(-1), i2c_smbus_handle(0), io_event(q)
{
}

QmSMBusHostPrivate::~QmSMBusHostPrivate()
{
}

void QmSMBusHostPrivate::init() {
	QM_Q(QmSMBusHost);
	stm32f2_ext_pins_init(hw_resource);
}

void QmSMBusHostPrivate::deinit() {
	QM_Q(QmSMBusHost);
	stm32f2_ext_pins_deinit(hw_resource);
}

void QmSMBusHostPrivate::processEventHardwareIO() {
	QM_Q(QmSMBusHost);
}

bool QmSMBusHost::event(QmEvent* event) {
	QM_D(QmSMBusHost);
	if (event->type() == QmEvent::HardwareIO) {
		d->processEventHardwareIO();
		return true;
	}
	return QmObject::event(event);
}
