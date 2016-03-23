/**
 ******************************************************************************
 * @file    spibus.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    12.02.2016
 *
 ******************************************************************************
 */

#include "spibus.h"
#include "spideviceinterface.h"
#include "hardware_resources.h"

SPIBus::SPIBus() :
	enabled(false)
{
}

SPIBus::~SPIBus()
{
	Q_ASSERT(slaves.isEmpty()); // all of them must unregister first
}

SPIBus* SPIBus::openInstance(int bus_hw_resource) {
	SPIBus *instance = new SPIBus();
	QtHwEmu::acquireResource(bus_hw_resource, instance);
	return instance;
}

void SPIBus::closeInstance(SPIBus* instance) {
	QtHwEmu::releaseResource(instance);
	delete instance;
}

SPIBus* SPIBus::getInstance(int bus_hw_resource) {
	SPIBus *instance = qobject_cast<SPIBus *>(QtHwEmu::getResourceInterface(bus_hw_resource));
	Q_ASSERT(instance);
	return instance;
}

void SPIBus::registerSlave(int cs_hw_resource, SPIDeviceInterface* instance) {
	Q_ASSERT(instance->thread() == thread()); // thread affinity must be same
	slaves.insert(cs_hw_resource, instance);
}

void SPIBus::unregisterSlave(SPIDeviceInterface* instance) {
	slaves.remove(slaves.key(instance), instance);
}

QList<SPIDeviceInterface*> SPIBus::getAddressedSlaves(int cs_hw_resource) {
	QList<SPIDeviceInterface*> list;
	if (enabled) {
		list = slaves.values(cs_hw_resource);
		if (cs_hw_resource != -1)
			list += slaves.values(-1);
	}
	return list;
}

void SPIBus::transferFullDuplex8bit(int cs_hw_resource,
		quint8* rx_data, quint8* tx_data, int count) {
	for (int i = 0; i < count; i++)
		rx_data[i] = 0xFF;
	Q_FOREACH (SPIDeviceInterface* const &slave, getAddressedSlaves(cs_hw_resource))
		Q_EMIT slave->transferFullDuplex8bit(rx_data, tx_data, count);
}

void SPIBus::transferFullDuplex16bit(int cs_hw_resource,
		quint16* rx_data, quint16* tx_data, int count) {
	for (int i = 0; i < count; i++)
		rx_data[i] = 0xFFFF;
	Q_FOREACH (SPIDeviceInterface* const &slave, getAddressedSlaves(cs_hw_resource))
		Q_EMIT slave->transferFullDuplex16bit(rx_data, tx_data, count);
}
