/**
 ******************************************************************************
 * @file    battery.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_POWER_BATTERY_H_
#define FIRMWARE_APP_POWER_BATTERY_H_

#include "sigc++/signal.h"

namespace Power {

class Battery {
public:
	enum Status {
		StatusNotReady,
		StatusNormal,
		StatusLow,
		StatusFailure
	};

	Battery(int smbus_i2c_resource);
	~Battery();
	Status getStatus();
	int getChargeLevel();

	sigc::signal<void, Status/*new_status*/> statusChanged;
	sigc::signal<void, int/*new_level*/> chargeLevelChanged;
};

} /* namespace Power */

#endif /* FIRMWARE_APP_POWER_BATTERY_H_ */
