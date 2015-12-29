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
	bus = I2CBus::getInstance(qmi2cdeviceprivate->bus_hw_resource);
	QObject::connect(bus, &I2CBus::transferResponse, this, &QmI2CDevicePrivateAdapter::processTransferResponse);
	QObject::connect(this, &QmI2CDevicePrivateAdapter::transferRequested, bus, &I2CBus::requestTransfer);
}

QmI2CDevicePrivateAdapter::~QmI2CDevicePrivateAdapter()
{
}

void QmI2CDevicePrivateAdapter::processTransferResponse(uint8_t address, bool ack, bool pec_present, const QByteArray& rx_data) {
	if (address != qmi2cdeviceprivate->address)
		return;
	qmi2cdeviceprivate->finishTransfer(ack, pec_present, rx_data);
}

QmI2CDevicePrivate::QmI2CDevicePrivate(QmI2CDevice *q) :
	QmObjectPrivate(q),
	bus_hw_resource(-1), transfer_timeout(0), transfer_in_progress(false),
	i2cdevice_adapter(0), address(0), transfer_uses_pec(false)
{
}

QmI2CDevicePrivate::~QmI2CDevicePrivate()
{
}

void QmI2CDevicePrivate::init(uint8_t address) {
	this->address = address;
	i2cdevice_adapter = new QmI2CDevicePrivateAdapter(this);
}

void QmI2CDevicePrivate::deinit()
{
	delete i2cdevice_adapter;
}

bool QmI2CDevicePrivate::startTransfer() {
	if (transfer_in_progress)
		return false;
	transfer_in_progress = true;
	transfer_uses_pec = false;
	transfer_rx_data.clear();
	return true;
}

void QmI2CDevicePrivate::finishTransfer(bool ack, bool pec_present, const QByteArray& rx_data) {
	QM_Q(QmI2CDevice);
	Q_ASSERT(transfer_in_progress);
	QmI2CDevice::TransferResult result = QmI2CDevice::transferSuccess;
	transfer_in_progress = false;
	if (ack) {
		if (!rx_data.isEmpty() && transfer_uses_pec && (!pec_present || (rx_data.size() != transfer_rx_data.size())))
			result = QmI2CDevice::transferErrorPEC;
		transfer_rx_data = rx_data.leftJustified(transfer_rx_data.size(), 0xFF, true);
	} else {
		result = QmI2CDevice::transferErrorAddressNACK;
	}
	if (result != QmI2CDevice::transferSuccess)
		transfer_rx_data.clear();
	q->transferCompleted.emit(result);
}

bool QmI2CDevice::startEmptyTransfer() {
	QM_D(QmI2CDevice);
	if (!d->startTransfer())
		return false;
	Q_EMIT d->i2cdevice_adapter->transferRequested(d->address, false, QByteArray(), 0);
	return true;
}

bool QmI2CDevice::startRxTransfer(bool use_pec, uint32_t size) {
	QM_D(QmI2CDevice);
	Q_ASSERT(size > 0);
	if (!d->startTransfer())
		return false;
	d->transfer_uses_pec = use_pec;
	d->transfer_rx_data.resize(size);
	Q_EMIT d->i2cdevice_adapter->transferRequested(d->address, use_pec, QByteArray(), size);
	return true;
}

bool QmI2CDevice::startTxTransfer(bool use_pec, uint8_t* data, uint32_t size) {
	QM_D(QmI2CDevice);
	Q_ASSERT(size > 0);
	if (!d->startTransfer())
		return false;
	d->transfer_uses_pec = use_pec;
	Q_EMIT d->i2cdevice_adapter->transferRequested(d->address, use_pec, QByteArray((char *)data, size), 0);
	return true;
}

bool QmI2CDevice::startTxRxTransfer(bool use_pec, uint8_t* tx_data, uint32_t tx_size, uint32_t rx_size) {
	QM_D(QmI2CDevice);
	Q_ASSERT((tx_size > 0) && (rx_size > 0));
	if (!d->startTransfer())
		return false;
	d->transfer_uses_pec = use_pec;
	d->transfer_rx_data.resize(rx_size);
	Q_EMIT d->i2cdevice_adapter->transferRequested(d->address, use_pec, QByteArray((char *)tx_data, tx_size), rx_size);
	return true;
}

int64_t QmI2CDevice::readRxData(uint8_t* buffer, uint32_t size, int offset) {
	QM_D(QmI2CDevice);
	if (!(!d->transfer_in_progress && !d->transfer_rx_data.isEmpty()))
		return -1;
	int64_t read = qmMax(0, qmMin((d->transfer_rx_data.size() - offset), (int)size));
	memcpy(buffer, (d->transfer_rx_data.data() + offset), read);
	d->transfer_rx_data.clear();
	return read;
}

bool QmI2CDevice::event(QmEvent* event) {
	return QmObject::event(event);
}
