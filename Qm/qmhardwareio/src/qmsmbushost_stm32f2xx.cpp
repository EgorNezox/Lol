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

static void qmi2cdeviceMessageReceivedIsrCallback(hal_i2c_smbus_handle_t handle, uint8_t address, uint16_t status, signed portBASE_TYPE *pxHigherPriorityTaskWoken) {
	QmSMBusHostIOEvent *system_event = static_cast<QmSMBusHostIOEvent *>(hal_i2c_get_smbus_userid(handle));
	if (system_event == 0)
		return;
	if (system_event->message_received)
		return;
	system_event->message_address = address;
	system_event->message_status = status;
	system_event->message_received = true;
	system_event->setPendingFromISR(pxHigherPriorityTaskWoken);
}

QmSMBusHostIOEvent::QmSMBusHostIOEvent(QmSMBusHost *o) :
	o(o),
	message_received(false), message_address(false), message_status(false)
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
	hal_i2c_smbus_host_params_t params;
	i2c_bus_instance = stm32f2_get_i2c_bus_instance(hw_resource);
	params.userid = static_cast<void *>(&io_event);
	params.isrcallbackMessageReceived = qmi2cdeviceMessageReceivedIsrCallback;
	i2c_smbus_handle = hal_i2c_open_smbus_host(i2c_bus_instance, &params);
}

void QmSMBusHostPrivate::deinit() {
	if (i2c_smbus_handle != 0)
		hal_i2c_close_smbus_host(i2c_smbus_handle);
}

void QmSMBusHostPrivate::processEventHardwareIO() {
	QM_Q(QmSMBusHost);
	if (io_event.message_received) {
		uint8_t address = io_event.message_address;
		uint16_t status = io_event.message_status;
		io_event.message_received = false;
		q->messageReceived(address, status);
	}
}

bool QmSMBusHost::event(QmEvent* event) {
	QM_D(QmSMBusHost);
	if (event->type() == QmEvent::HardwareIO) {
		d->processEventHardwareIO();
		return true;
	}
	return QmObject::event(event);
}
