/**
  ******************************************************************************
  * @file    qmspidevice_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    12.02.2016
  *
  ******************************************************************************
  */

#include <string.h>
#include <qevent.h>
#include <qbytearray.h>

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
	cs_hw_resource(-1), spidevice_adapter(0), bus_hw_resource(-1)
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

bool QmSPIDevice::transferBurstFullDuplex8bit(FD8Burst *bursts, int count) {
	QM_D(QmSPIDevice);
	int total_count = 0;
	QByteArray rx_buffer, tx_buffer;
	for (int i = 0; i < count; i++) {
		if (bursts[i].tx_data)
			tx_buffer.append((char *)bursts[i].tx_data, bursts[i].count);
		else
			tx_buffer.append(QByteArray(bursts[i].count, 0xFF));
		total_count += bursts[i].count;
	}
	rx_buffer.resize(total_count);
	Q_EMIT d->spidevice_adapter->transferFullDuplex8bit(d->cs_hw_resource, (quint8 *)rx_buffer.data(), (quint8 *)tx_buffer.data(), total_count);
	total_count = 0;
	for (int i = 0; i < count; i++) {
		if (bursts[i].rx_data)
			memcpy(bursts[i].rx_data, rx_buffer.data() + total_count, bursts[i].count);
		total_count += bursts[i].count;
	}
	return true;
}

bool QmSPIDevice::transferBurstFullDuplex16bit(FD16Burst *bursts, int count) {
	QM_D(QmSPIDevice);
	int total_count = 0;
	QByteArray rx_buffer, tx_buffer;
	for (int i = 0; i < count; i++) {
		if (bursts[i].tx_data)
			tx_buffer.append((char *)(bursts[i].tx_data), 2*(bursts[i].count));
		else
			tx_buffer.append(QByteArray(2*(bursts[i].count), 0xFF));
		total_count += bursts[i].count;
	}
	rx_buffer.resize(2*total_count);
	Q_EMIT d->spidevice_adapter->transferFullDuplex16bit(d->cs_hw_resource, (quint16 *)rx_buffer.data(), (quint16 *)tx_buffer.data(), total_count);
	total_count = 0;
	for (int i = 0; i < count; i++) {
		if (bursts[i].rx_data)
			memcpy(bursts[i].rx_data, rx_buffer.data() + 2*total_count, 2*(bursts[i].count));
		total_count += bursts[i].count;
	}
	return true;
}
