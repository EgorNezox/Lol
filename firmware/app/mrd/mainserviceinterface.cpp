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

} /* namespace Multiradio */
