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

bool QmSPIDevice::transferFullDuplex8bit(uint8_t *rx_data, uint8_t *tx_data, int count) {
	FD8Burst single_burst = {rx_data, tx_data, count};
	return transferBurstFullDuplex8bit(&single_burst, 1);
}

bool QmSPIDevice::transferFullDuplex16bit(uint16_t *rx_data, uint16_t *tx_data, int count) {
	FD16Burst single_burst = {rx_data, tx_data, count};
	return transferBurstFullDuplex16bit(&single_burst, 1);
}
