/**
 ******************************************************************************
 * @file    i2cbus.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    26.12.2015
 *
 ******************************************************************************
 */

#include <QMetaMethod>

#include "i2cbus.h"
#include "i2cdeviceinterface.h"
#include "hardware_emulation.h"

I2CBus::I2CBus()
{
}

I2CBus::~I2CBus()
{
	Q_ASSERT(slaves.isEmpty()); // all of them must unregister first
}

I2CBus* I2CBus::openInstance(int hw_resource) {
	I2CBus *instance = new I2CBus();
	QtHwEmu::acquireResource(hw_resource, instance);
	return instance;
}

void I2CBus::closeInstance(I2CBus* instance) {
	QtHwEmu::releaseResource(instance);
	delete instance;
}

I2CBus* I2CBus::getInstance(int hw_resource) {
	I2CBus *instance = qobject_cast<I2CBus *>(QtHwEmu::getResourceInterface(hw_resource));
	Q_ASSERT(instance);
	return instance;
}

void I2CBus::registerSlave(uint8_t address, I2CDeviceInterface* instance) {
	Q_ASSERT(!slaves.contains(address)); // address must not be registered already with other instance
	Q_ASSERT(instance->thread() == thread()); // thread affinity must be same
	slaves.insert(address, instance);
}

void I2CBus::unregisterSlave(I2CDeviceInterface* instance) {
	slaves.remove(slaves.key(instance));
}

void I2CBus::responseTransferDelayed(uint8_t address, bool ack, bool pec_present, const QByteArray& rx_data) {
	// emits delayed signal
	QMetaMethod::fromSignal(&I2CBus::transferResponse).invoke(this, Qt::QueuedConnection,
			Q_ARG(uint8_t, address),
			Q_ARG(bool, ack),
			Q_ARG(bool, pec_present),
			Q_ARG(QByteArray, rx_data)
	);
}

void I2CBus::responseTransferDelayed(I2CDeviceInterface *slave, bool pec_present, const QByteArray& rx_data) {
	Q_ASSERT(!slaves.keys(slave).isEmpty());
	responseTransferDelayed(slaves.key(slave), true, pec_present, rx_data);
}

void I2CBus::processHostNotify(I2CDeviceInterface* slave, uint16_t status) {
	Q_ASSERT(!slaves.keys(slave).isEmpty());
	Q_EMIT messageHostNotify(slaves.key(slave), status);
}

void I2CBus::requestTransfer(uint8_t address, bool use_pec, const QByteArray& tx_data, uint32_t rx_size) {
	bool processed = false;
	I2CDeviceInterface* slave = slaves.value(address, 0);
	if (slave != 0)
		processed = slave->processTransfer(use_pec, tx_data, rx_size);
	if (!processed)
		responseTransferDelayed(address, false, false, QByteArray());
}
