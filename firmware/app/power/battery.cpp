/**
 ******************************************************************************
 * @file    battery.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  Petr Dmitriev
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "qm.h"
#include "qmdebug.h"
#include "qmtimer.h"
#include "qmi2cdevice.h"

#include "battery.h"

#define BATTERY_VOLTAGE 1

namespace Power {

#define BATTERY_SMBUS_ADDRESS			0x0B
#define BATTERY_LOW_CHARGE_THRESHOLD	10//%
#define BATTERY_UPDATE_INTERVAL			4000

Battery::Battery(int smbus_i2c_resource) :
		state(StateNone), status(StatusNotReady), charge_level(0)
{
	battery_device = new QmI2CDevice(smbus_i2c_resource, BATTERY_SMBUS_ADDRESS);
	battery_device->transferCompleted.connect(sigc::mem_fun(this, &Battery::processDataTransferCompleted));
	poll_timer = new QmTimer(false);
	poll_timer->timeout.connect(sigc::mem_fun(this, &Battery::processBatteryDevicePolling));
	poll_timer->setInterval(BATTERY_UPDATE_INTERVAL);
	poll_timer->start();
}

Battery::~Battery() {
	delete battery_device;
	delete poll_timer;
}

Battery::Status Battery::getStatus() {
	return status;
}

int Battery::getChargeLevel() {
	return charge_level;
}

int Battery::getVoltage() {
    return charge_level;
}

void Battery::setStatus(Status new_status) {
	if (status != new_status) {
		status = new_status;
		statusChanged(status);
	}
}

void Battery::processBatteryDevicePolling()
{
    if (recParam == recCharge){
        bool success = false;
        requireChargeLevel(&success);
        if (!success) {
            setStatus(StatusFailure);
            recParam = recVoltage;
            return;
        }
        recParam = recVoltage;
    }
    else if (recParam == recVoltage){
        bool success = false;
        requireVoltage(&success);
        if (!success) {
            setStatus(StatusFailure);
            recParam = recCharge;
            return;
        }
        recParam = recCharge;
    }
}

void Battery::requireChargeLevel(bool* success) {
	if (state != StateNone) {
		*success = false;
		return;
	}
	state = StateReqRelativeStateOfCharge;
	uint8_t tx_data = batCmdRelativeStateOfCharge;
	*success = battery_device->startTxRxTransfer(false, &tx_data, 1, 1);
}

#if BATTERY_VOLTAGE

void Battery::requireVoltage(bool* success) {
	if (state != StateNone) {
		*success = false;
		return;
	}
	state = StateReqVoltage;
	uint8_t tx_data = batCmdVoltage;
	*success = battery_device->startTxRxTransfer(false, &tx_data, 1, 2);
}

void Battery::processDataTransferCompleted(QmI2CDevice::TransferResult result) {
	if (state == StateNone)
		return;
	if (result != QmI2CDevice::transferSuccess) {
		setStatus(StatusFailure);
		state = StateNone;
		return;
	}
	switch (state) {
	case StateReqVoltage: {
		uint8_t rx_data[2];
		if (2 != battery_device->readRxData(rx_data, 2)) {
			setStatus(StatusFailure);
			break;
		}
		int actual_voltage = *(uint16_t*)rx_data;
		if (actual_voltage < 0 || actual_voltage > 18000) {
			setStatus(StatusFailure);
			break;
		}
            if (actual_voltage != voltage) {
                voltage = actual_voltage;
                voltageChanged(voltage);
                voltageReceived(actual_voltage);
            }
		break;
	}
	case StateReqRelativeStateOfCharge: {
		uint8_t rx_data;
		if (1 != battery_device->readRxData(&rx_data, 1)) {
			setStatus(StatusFailure);
			break;
		}
		int actual_charge_level = rx_data;
		if (actual_charge_level < 0 || actual_charge_level > 100) {
			setStatus(StatusFailure);
			break;
		}
		if (actual_charge_level != charge_level) {
			charge_level = actual_charge_level;
            chargeLevelChanged(charge_level);
			if (charge_level <= BATTERY_LOW_CHARGE_THRESHOLD) {
				setStatus(StatusLow);
			} else {
				setStatus(StatusNormal);
			}
		}
		break;
	}
	default: QM_ASSERT(0); break;
	}
	state = StateNone;
}

#else

void Battery::processDataTransferCompleted(QmI2CDevice::TransferResult result) {
	switch (state) {
	case StateReqRelativeStateOfCharge: {
		uint8_t rx_data;
		if (result != QmI2CDevice::transferSuccess) {
			setStatus(StatusFailure);
			break;
		}
		if (1 != battery_device->readRxData(&rx_data, 1)) {
			setStatus(StatusFailure);
			break;
		}
		int actual_charge_level = rx_data;
		if (actual_charge_level < 0 || actual_charge_level > 100) {
			setStatus(StatusFailure);
			break;
		}
		if (actual_charge_level != charge_level) {
			charge_level = actual_charge_level;
			chargeLevelChanged(charge_level);
			if (charge_level <= BATTERY_LOW_CHARGE_THRESHOLD) {
				setStatus(StatusLow);
			} else {
				setStatus(StatusNormal);
			}
		}
		break;
	}
	default: QM_ASSERT(0); break;
	}
	state = StateNone;
}

#endif

void Battery::setMinimalActivityMode(bool enabled)
{
	if (enabled) {
		poll_timer->stop();
	} else {
		poll_timer->start();
	}
}

} /* namespace Power */
