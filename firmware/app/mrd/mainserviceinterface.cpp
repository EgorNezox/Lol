/**
 ******************************************************************************
 * @file    mainserviceinterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "mainserviceinterface.h"
#include "dispatcher.h"

namespace Multiradio {

MainServiceInterface::MainServiceInterface(Dispatcher *dispatcher) :
	QmObject(dispatcher),
	current_status(StatusNotReady)
{
}

MainServiceInterface::~MainServiceInterface()
{
}

MainServiceInterface::Status MainServiceInterface::getStatus() {
	return current_status;
}

void MainServiceInterface::setStatus(Status value) {
	if (current_status != value) {
		current_status = value;
		statusChanged(value);
	}
}

void MainServiceInterface::startAleRx() {
	//...
}

void MainServiceInterface::startAleTxVoiceMail(uint8_t address) {
	QM_UNUSED(address);
	//...
}

MainServiceInterface::AleResult MainServiceInterface::stopAle() {
	//...
	return AleResultNone;
}

MainServiceInterface::AleState MainServiceInterface::getAleState() {
	//...
	return AleState_IDLE;
}

uint8_t MainServiceInterface::getAleRxAddress() {
	//...
	return 0;
}

} /* namespace Multiradio */
