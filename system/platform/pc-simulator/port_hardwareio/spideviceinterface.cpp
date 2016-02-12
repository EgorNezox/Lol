/**
 ******************************************************************************
 * @file    spideviceinterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    12.02.2016
 *
 ******************************************************************************
 */

#include "spideviceinterface.h"
#include "spibus.h"

SPIDeviceInterface::SPIDeviceInterface(int bus_hw_resource, int cs_hw_resource) :
	bus(SPIBus::getInstance(bus_hw_resource))
{
	bus->registerSlave(cs_hw_resource, this);
}

SPIDeviceInterface::~SPIDeviceInterface()
{
	bus->unregisterSlave(this);
}
