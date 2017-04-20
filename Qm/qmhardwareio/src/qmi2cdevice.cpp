/**
  ******************************************************************************
  * @file    qmi2cdevice.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.11.2015
  *
  ******************************************************************************
  */

#include "qmi2cdevice_p.h"

QmI2CDevice::QmI2CDevice(int bus_hw_resource, uint8_t address, QmObject* parent) :
	QmObject(*new QmI2CDevicePrivate(this), parent)
{
	QM_D(QmI2CDevice);
	d->bus_hw_resource = bus_hw_resource;
	d->init(address);
}

QmI2CDevice::~QmI2CDevice() {
	QM_D(QmI2CDevice);
	d->deinit();
}

void QmI2CDevice::setTransferTimeout(int msec) {
	QM_D(QmI2CDevice);
	d->transfer_timeout = msec;
}

bool QmI2CDevice::isTransferInProgress() {
	QM_D(QmI2CDevice);
	return d->transfer_in_progress;
}

void QmI2CDevice::setAdress(uint8_t address)
{ QM_D(QmI2CDevice);
 // d->i2c_transfer.device.address = address;
}
