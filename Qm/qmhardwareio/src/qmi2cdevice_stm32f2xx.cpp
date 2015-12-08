/**
  ******************************************************************************
  * @file    qmi2cdevice_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.11.2015
  *
  ******************************************************************************
  */

#include <string.h>
#include "system_hw_io.h"

#include "qmdebug.h"
#include "qmi2cdevice_p.h"
#include "qmevent.h"
#include "qmapplication.h"

QmI2CDeviceIOEvent::QmI2CDeviceIOEvent(QmI2CDevice *o) :
	o(o)
{
}

void QmI2CDeviceIOEvent::process() {
	QmApplication::postEvent(o, new QmEvent(QmEvent::HardwareIO));
}

QmI2CDevicePrivate::QmI2CDevicePrivate(QmI2CDevice *q) :
	QmObjectPrivate(q),
	hw_resource(-1),
	io_event(q)
{
}

QmI2CDevicePrivate::~QmI2CDevicePrivate()
{
}

void QmI2CDevicePrivate::init() {
	QM_Q(QmI2CDevice);
}

void QmI2CDevicePrivate::deinit() {
	QM_Q(QmI2CDevice);
}

void QmI2CDevicePrivate::processEventHardwareIO() {
	QM_Q(QmI2CDevice);
}

bool QmI2CDevice::event(QmEvent* event) {
	QM_D(QmI2CDevice);
	if (event->type() == QmEvent::HardwareIO) {
		d->processEventHardwareIO();
		return true;
	}
	return QmObject::event(event);
}
