/**
 ******************************************************************************
 * @file    uartinterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    13.11.2015
 *
 ******************************************************************************
 */

#include <qmetaobject.h>
#include "qmuart.h"

#include "uartinterface.h"
#include "hardware_emulation.h"

// workaround for nested forward declaration
struct QmUart_ConfigStruct : QmUart::ConfigStruct
{
};

UartInterface::UartInterface() :
	mutex(QMutex::Recursive), config(new QmUart_ConfigStruct),
	rx_data_error(false), rx_overflow(false)
{
}

UartInterface::~UartInterface()
{
	delete config;
}

UartInterface* UartInterface::getInstance(int hw_resource) {
	UartInterface *instance = qobject_cast<UartInterface *>(QtHwEmu::getResourceInterface(hw_resource));
	Q_ASSERT(instance);
	return instance;
}

UartInterface* UartInterface::createInstance(int hw_resource) {
	UartInterface *instance = new UartInterface();
	QtHwEmu::acquireResource(hw_resource, instance);
	return instance;
}

void UartInterface::destroyInstance(UartInterface* instance) {
	QtHwEmu::releaseResource(instance);
	delete instance;
}

void UartInterface::transferRx(const QByteArray& data) {
	QMutexLocker locker(&mutex);
	if (!data.isEmpty()) {
		int free_space = config->rx_buffer_size - rx_buffer.size();
		if (data.size() > free_space)
			rx_overflow = true;
		rx_buffer.append(data.left(qMin(data.size(), free_space)));
		Q_EMIT rxStatusUpdated();
	}
}

void UartInterface::injectRxDataError() {
	QMutexLocker locker(&mutex);
	rx_data_error = true;
	Q_EMIT rxStatusUpdated();
}

void UartInterface::setConfig(struct QmUart_ConfigStruct* config) {
	QMutexLocker locker(&mutex);
	*(this->config) = *config;
}

quint32 UartInterface::getRxDataAvailable() {
	QMutexLocker locker(&mutex);
	return rx_buffer.size();
}

QByteArray UartInterface::popRxData(quint32 size) {
	QMutexLocker locker(&mutex);
	QByteArray data = rx_buffer.left(size);
	rx_buffer.remove(0, size);
	return data;
}

bool UartInterface::popRxErrors(RxErrorsDescriptor &descriptor) {
	QMutexLocker locker(&mutex);
	descriptor.data = rx_data_error;
	rx_data_error = false;
	descriptor.overflow = rx_overflow;
	rx_overflow = false;
	return (descriptor.data || descriptor.overflow);
}

quint32 UartInterface::getTxSpaceAvailable() {
	QMutexLocker locker(&mutex);
	return config->tx_buffer_size;
}

void UartInterface::pushTxData(const QByteArray& data) {
	Q_EMIT txTransferred(data);
	QMetaMethod::fromSignal(&UartInterface::txStatusUpdated).invoke(this, Qt::QueuedConnection); // emits delayed signal
}

void UartInterface::flushTx()
{
}
