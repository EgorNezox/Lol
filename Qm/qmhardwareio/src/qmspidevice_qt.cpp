/**
  ******************************************************************************
  * @file    qmspidevice_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    12.02.2016
  *
  ******************************************************************************
  */

#include <qevent.h>

#include "qm.h"
#include "qmspidevice_p.h"

QmSPIDevicePrivateAdapter::QmSPIDevicePrivateAdapter(QmSPIDevicePrivate *qmspideviceprivate) :
	qmspideviceprivate(qmspideviceprivate)
{
	bus = SPIBus::getInstance(qmspideviceprivate->bus_hw_resource);
	connectBus();
}

QmSPIDevicePrivateAdapter::~QmSPIDevicePrivateAdapter()
{
}

void QmSPIDevicePrivateAdapter::connectBus() {
	Qt::ConnectionType type;
	if (thread() == bus->thread())
		type = Qt::DirectConnection;
	else
		type = Qt::BlockingQueuedConnection;
	QObject::connect(this, &QmSPIDevicePrivateAdapter::transferFullDuplex8bit, bus, &SPIBus::transferFullDuplex8bit, type);
	QObject::connect(this, &QmSPIDevicePrivateAdapter::transferFullDuplex16bit, bus, &SPIBus::transferFullDuplex16bit, type);
}

bool QmSPIDevicePrivateAdapter::event(QEvent *e) {
	if (e->type() == QEvent::ThreadChange) {
		QObject::disconnect(this, 0, 0, 0);
		QMetaObject::invokeMethod(this, "connectBus", Qt::QueuedConnection);
	}
	return QObject::event(e);
}

QmSPIDevicePrivate::QmSPIDevicePrivate(QmSPIDevice *q) :
	QmObjectPrivate(q),
	bus_hw_resource(-1), cs_hw_resource(-1), spidevice_adapter(0)
{
}

QmSPIDevicePrivate::~QmSPIDevicePrivate()
{
}

void QmSPIDevicePrivate::init(int bus_hw_resource, QmSPIDevice::BusConfigStruct *bus_config, int cs_hw_resource) {
	QM_UNUSED(bus_config);
	this->bus_hw_resource = bus_hw_resource;
	this->cs_hw_resource = cs_hw_resource;
	spidevice_adapter = new QmSPIDevicePrivateAdapter(this);
}

void QmSPIDevicePrivate::deinit()
{
	delete spidevice_adapter;
}

bool QmSPIDevice::transferFullDuplex8bit(uint8_t *rx_data, uint8_t *tx_data, int count) {
	QM_D(QmSPIDevice);
	Q_EMIT d->spidevice_adapter->transferFullDuplex8bit(d->cs_hw_resource, rx_data, tx_data, count);
	return true;
}

bool QmSPIDevice::transferFullDuplex16bit(uint16_t *rx_data, uint16_t *tx_data, int count) {
	QM_D(QmSPIDevice);
	Q_EMIT d->spidevice_adapter->transferFullDuplex16bit(d->cs_hw_resource, rx_data, tx_data, count);
	return true;
}
