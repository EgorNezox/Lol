/**
  ******************************************************************************
  * @file    qmi2cdevice.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.11.2015
  *
  ******************************************************************************
  */

#include "qmi2cdevice_p.h"

QmI2CDevice::QmI2CDevice(int hw_resource, QmObject* parent) :
	QmObject(*new QmI2CDevicePrivate(this), parent)
{
	QM_D(QmI2CDevice);
	d->hw_resource = hw_resource;
	d->init();
}

QmI2CDevice::~QmI2CDevice() {
	QM_D(QmI2CDevice);
	d->deinit();
}
