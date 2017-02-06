/**
 ******************************************************************************
 * @file    battery.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  Petr Dmitriev
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_POWER_BATTERY_H_
#define FIRMWARE_APP_POWER_BATTERY_H_

#include "sigc++/signal.h"
#include "qmi2cdevice.h"

class QmTimer;

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

	/* Запрос уровня напряжения батареи. Значение возвращается сигналом voltageReceived. */
	void requireVoltage(bool* success);

	void setMinimalActivityMode(bool enabled);

	sigc::signal<void, Status/*new_status*/> statusChanged;
	sigc::signal<void, int/*new_level_charge*/> chargeLevelChanged;
    sigc::signal<void, int/*new_level_voltage*/> voltageChanged;
	sigc::signal<void, int/*voltage*/> voltageReceived;

    int getVoltage();
private:
	void setStatus(Status new_status);
	void processBatteryDevicePolling();
	void requireChargeLevel(bool* success);
	void processDataTransferCompleted(QmI2CDevice::TransferResult result);

	enum {
		batCmdVoltage 				= 0x09,
		batCmdRelativeStateOfCharge = 0x0D
	} batCmd;

	enum State {
		StateNone,
		StateReqVoltage,
		StateReqRelativeStateOfCharge
	};

	State state;
	Status status;
	int charge_level;
    int voltage;
    int deltaVoltage = 100; //mV
	QmI2CDevice* battery_device;
	QmTimer* poll_timer;

    enum {
        recVoltage = 0,
        recCharge  = 1
    } recParam  = recCharge;
};

} /* namespace Power */

#endif /* FIRMWARE_APP_POWER_BATTERY_H_ */
