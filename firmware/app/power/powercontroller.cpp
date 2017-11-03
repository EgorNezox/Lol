/**
 ******************************************************************************
 * @file    powercontroller.cpp
 * @author  Petr Dmitriev
 * @date    30.03.2016
 *
 ******************************************************************************
 */

#define QMDEBUGDOMAIN	powercontroller

#include "qmdebug.h"
#include "qmiopin.h"
#include "qmtimer.h"
#include "powercontroller.h"

namespace Power {

#define CTRL_PULSE_DURATION		3000

Controller::Controller(int hscontrol_iopin_resource, int controller_iopin_resource, int offint_iopin_resource, int source_iopin_resource) {
	hscontrol_iopin = new QmIopin(hscontrol_iopin_resource);
	hscontrol_iopin->setInputTriggerMode(QmIopin::InputTrigger_Falling);
	hscontrol_iopin->inputTrigger.connect(sigc::mem_fun(this, &Controller::hsControlTriggered));
	controller_iopin = new QmIopin(controller_iopin_resource);
	controller_iopin->deinit();
	offint_iopin = new QmIopin(offint_iopin_resource);
	offint_iopin->setInputTriggerMode(QmIopin::InputTrigger_Falling);
	offint_iopin->inputTrigger.connect(sigc::mem_fun(this, &Controller::offIntTriggered));
	source_iopin = new QmIopin(source_iopin_resource);
	ctrl_pulse_timer = new QmTimer(true);
	ctrl_pulse_timer->setInterval(CTRL_PULSE_DURATION);
	ctrl_pulse_timer->timeout.connect(sigc::mem_fun(this, &Controller::ctrlPulseTimeout));
	hscontrol_debounce_timer = new QmTimer(true);
	hscontrol_debounce_timer->setInterval(100);
	hscontrol_debounce_timer->timeout.connect(sigc::mem_fun(this, &Controller::hscontrolDebounceTimeout));

	if (QmIopin::Level_Low == source_iopin->readInput()) {
		//qmDebugMessage(QmDebug::Dump, "powering on from headset detected");
		ctrlPulseStart();
	}
}

void Controller::hsControlTriggered() {
	hscontrol_debounce_timer->start();
}

void Controller::offIntTriggered() {
	//qmDebugMessage(QmDebug::Dump, "power off warning");
	//powerOffWarning();
}

void Controller::ctrlPulseTimeout() {
	controller_iopin->deinit();
}

void Controller::hscontrolDebounceTimeout() {
	if (QmIopin::Level_Low == hscontrol_iopin->readInput()) {
		ctrlPulseStart();
	}
}

void Controller::ctrlPulseStart() {
	controller_iopin->init();
	controller_iopin->writeOutput(QmIopin::Level_Low);
	ctrl_pulse_timer->start();
}

} /* namespace Power */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(powercontroller, LevelDefault)
#include "qmdebug_domains_end.h"
