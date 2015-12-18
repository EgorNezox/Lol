/**
 ******************************************************************************
 * @file    battery.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "qm.h"

#include "battery.h"

namespace Power {

Battery::Battery(int smbus_i2c_resource) {
	QM_UNUSED(smbus_i2c_resource);
	//...
}

Battery::~Battery() {
	//...
}

Battery::Status Battery::getStatus() {
	//...
	return StatusNotReady;
}

int Battery::getChargeLevel() {
	//...
	return 0;
}

} /* namespace Power */
