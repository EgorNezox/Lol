/**
 ******************************************************************************
 * @file    i2cdeviceinterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    26.12.2015
 *
 ******************************************************************************
 */

#include "i2cdeviceinterface.h"
#include "i2cbus.h"

I2CDeviceInterface::I2CDeviceInterface(int bus_hw_resource, uint8_t address) :
	bus(I2CBus::getInstance(bus_hw_resource)),
	address_ack(false), state(stateIdle), transfer_uses_pec(false), transfer_rx_size(0)
{
	bus->registerSlave(address, this);
}

I2CDeviceInterface::~I2CDeviceInterface()
{
	bus->unregisterSlave(this);
}

I2CDeviceInterface::State I2CDeviceInterface::readTransfer(bool& use_pec, QByteArray& tx_data, uint32_t &rx_size) {
	State source_state = state;
	switch (state) {
	case stateIdle:
		break;
	case statePendingEmpty:
		state = stateIdle;
		bus->responseTransferDelayed(this, false, QByteArray());
		break;
	case statePendingRx:
		rx_size = transfer_rx_size;
		break;
	case statePendingTx:
		use_pec = transfer_uses_pec;
		tx_data = transfer_tx_data;
		state = stateIdle;
		bus->responseTransferDelayed(this, false, QByteArray());
		break;
	case statePendingTxRx:
		tx_data = transfer_tx_data;
		rx_size = transfer_rx_size;
		state = statePendingRx;
		break;
	}
	return source_state;
}

void I2CDeviceInterface::writeRx(bool use_pec, const QByteArray& data) {
	if (state == statePendingRx) {
		state = stateIdle;
		bus->responseTransferDelayed(this, use_pec, data);
	}
}

void I2CDeviceInterface::setAddressAck(bool enabled) {
	address_ack = enabled;
}

void I2CDeviceInterface::notifyHost(uint16_t status) {
	bus->processHostNotify(this, status);
}

bool I2CDeviceInterface::processTransfer(bool use_pec, const QByteArray& tx_data, uint32_t rx_size) {
	state = stateIdle;
	if (!address_ack)
		return false;
	if ((tx_data.size() == 0) && (rx_size == 0)) {
		state = statePendingEmpty;
	} else if ((tx_data.size() > 0) && (rx_size == 0)) {
		state = statePendingTx;
		transfer_uses_pec = use_pec;
		transfer_tx_data = tx_data;
	} else if ((tx_data.size() == 0) && (rx_size > 0)) {
		state = statePendingRx;
		transfer_rx_size = rx_size;
	} else if ((tx_data.size() > 0) && (rx_size > 0)) {
		state = statePendingTxRx;
		transfer_tx_data = tx_data;
		transfer_rx_size = rx_size;
	} else {
		return false;
	}
	Q_EMIT transferPending();
	return true;
}
