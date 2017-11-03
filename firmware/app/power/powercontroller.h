/**
 ******************************************************************************
 * @file    powercontroller.h
 * @author  Petr Dmitriev
 * @date    30.03.2016
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_POWER_POWERCONTROLLER_H_
#define FIRMWARE_APP_POWER_POWERCONTROLLER_H_

class QmIopin;
class QmTimer;

namespace Power {

class Controller {
public:
	Controller(int hscontrol_iopin_resource, int controller_iopin_resource, int offint_iopin_resource, int source_iopin_resource);

	//sigc::signal<void> powerOffWarning;

private:
	void hsControlTriggered();
	void offIntTriggered();
	void ctrlPulseTimeout();
	void hscontrolDebounceTimeout();
	void ctrlPulseStart();

	QmIopin *hscontrol_iopin;
	QmIopin *controller_iopin;
	QmIopin *offint_iopin;
	QmIopin *source_iopin;
	QmTimer *ctrl_pulse_timer;
	QmTimer *hscontrol_debounce_timer;
};

} /* namespace Power */

#endif /* FIRMWARE_APP_POWER_POWERCONTROLLER_H_ */
