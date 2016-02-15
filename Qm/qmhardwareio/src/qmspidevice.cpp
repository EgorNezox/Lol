/**
  ******************************************************************************
  * @file    qmspidevice.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.02.2016
  *
  ******************************************************************************
  */

#include "qmspidevice_p.h"

QmSPIDevice::QmSPIDevice(int bus_hw_resource, BusConfigStruct *bus_config, int cs_hw_resource, QmObject *parent) :
	QmObject(*new QmSPIDevicePrivate(this), parent)
{
	QM_D(QmSPIDevice);
	d->init(bus_hw_resource, bus_config, cs_hw_resource);
}

QmSPIDevice::~QmSPIDevice() {
	QM_D(QmSPIDevice);
	d->deinit();
}
