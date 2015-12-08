/**
  ******************************************************************************
  * @file    qmi2cdevice_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.11.2015
  *
  ******************************************************************************
  */

#include "qmi2cdevice_p.h"

QmI2CDevicePrivateAdapter::QmI2CDevicePrivateAdapter(QmI2CDevicePrivate *qmi2cdeviceprivate) :
	qmi2cdeviceprivate(qmi2cdeviceprivate)
{
	interface = I2CDeviceInterface::getInstance(qmi2cdeviceprivate->hw_resource);
}

QmI2CDevicePrivateAdapter::~QmI2CDevicePrivateAdapter()
{
}

QmI2CDevicePrivate::QmI2CDevicePrivate(QmI2CDevice *q) :
	QmObjectPrivate(q),
	hw_resource(-1), i2cdevice_adapter(0)
{
}

QmI2CDevicePrivate::~QmI2CDevicePrivate()
{
}

void QmI2CDevicePrivate::init() {
	i2cdevice_adapter = new QmI2CDevicePrivateAdapter(this);
}

void QmI2CDevicePrivate::deinit()
{
	delete i2cdevice_adapter;
}

bool QmI2CDevice::event(QmEvent* event) {
	return QmObject::event(event);
}
