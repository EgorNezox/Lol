/**
  ******************************************************************************
  * @file    qmuart_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  *
  ******************************************************************************
  */

#include "qmuart_p.h"

QmUartPrivateAdapter::QmUartPrivateAdapter(QmUartPrivate *qmuartprivate) :
	qmuartprivate(qmuartprivate)
{
	interface = UartInterface::getInstance(qmuartprivate->hw_resource);
	interface->setConfig((struct QmUart_ConfigStruct *)&(qmuartprivate->config));
	QObject::connect(interface, &UartInterface::rxStatusUpdated, this, &QmUartPrivateAdapter::processRxStatus);
	QObject::connect(interface, &UartInterface::txStatusUpdated, this, &QmUartPrivateAdapter::processTxStatus);
	QObject::connect(this, &QmUartPrivateAdapter::writeTx, interface, &UartInterface::pushTxData);
}

QmUartPrivateAdapter::~QmUartPrivateAdapter()
{
}

void QmUartPrivateAdapter::processRxStatus() {
	QmUart * const q = qmuartprivate->q_func();
	UartInterface::RxErrorsDescriptor rx_errors;
	if (qmuartprivate->active && interface->popRxErrors(rx_errors))
		q->rxError(rx_errors.data, rx_errors.overflow);
	if (qmuartprivate->active && (interface->getRxDataAvailable() > 0))
		q->dataReceived.emit();
	if (!qmuartprivate->active) {
		interface->popRxErrors(rx_errors);
		interface->popRxData(interface->getRxDataAvailable());
	}
}

void QmUartPrivateAdapter::processTxStatus() {
	QmUart * const q = qmuartprivate->q_func();
	if (qmuartprivate->active && (interface->getTxSpaceAvailable() > 0))
		q->dataTransmitted.emit();
}

quint32 QmUartPrivateAdapter::getInterfaceRxDataAvailable() {
	return interface->getRxDataAvailable();
}

quint32 QmUartPrivateAdapter::getInterfaceTxSpaceAvailable() {
	return interface->getTxSpaceAvailable();
}

QByteArray QmUartPrivateAdapter::popInterfaceRxData(quint32 size) {
	return interface->popRxData(size);
}

void QmUartPrivateAdapter::flushInterfaceTx() {
	interface->flushTx();
}

QmUartPrivate::QmUartPrivate(QmUart *q) :
	QmObjectPrivate(q),
	hw_resource(-1), active(false), uart_adapter(0)
{
}

QmUartPrivate::~QmUartPrivate()
{
}

void QmUartPrivate::init() {
	uart_adapter = new QmUartPrivateAdapter(this);
}

void QmUartPrivate::deinit()
{
	active = false;
	delete uart_adapter;
}

bool QmUart::isOpen() {
	QM_D(QmUart);
	return d->active;
}

bool QmUart::open() {
	QM_D(QmUart);
	if (d->active)
		return false;
	d->active = true;
	return true;
}

bool QmUart::close() {
	QM_D(QmUart);
	if (!d->active)
		return false;
	d->uart_adapter->flushInterfaceTx();
	d->active = false;
	return true;
}

int64_t QmUart::readData(uint8_t* buffer, uint32_t max_size) {
	QM_D(QmUart);
	if (!d->active)
		return -1;
	quint32 read = qMin(max_size, d->uart_adapter->getInterfaceRxDataAvailable());
	const QByteArray data = d->uart_adapter->popInterfaceRxData(read);
	if (buffer)
		memcpy(buffer, data.data(), data.size());
	return (int64_t)data.size();
}

int64_t QmUart::writeData(const uint8_t* data, uint32_t data_size) {
	QM_D(QmUart);
	if (!d->active)
		return -1;
	int64_t written = qMin(data_size, d->uart_adapter->getInterfaceTxSpaceAvailable());
	Q_EMIT d->uart_adapter->writeTx(QByteArray::fromRawData((char *)data, written));
    return written;
}

uint32_t QmUart::getRxDataAvailable() {
	QM_D(QmUart);
	if (!d->active)
		return 0;
	return d->uart_adapter->getInterfaceRxDataAvailable();
}

uint32_t QmUart::getTxSpaceAvailable() {
	QM_D(QmUart);
	if (!d->active)
		return 0;
	return d->uart_adapter->getInterfaceTxSpaceAvailable();
}

bool QmUart::event(QmEvent* event) {
	return QmObject::event(event);
}
