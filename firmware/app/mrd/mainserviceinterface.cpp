/**
 ******************************************************************************
 * @file    mainserviceinterface.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "mainserviceinterface.h"

namespace Multiradio {

MainServiceInterface::MainServiceInterface() {
	//...
}

MainServiceInterface::~MainServiceInterface() {
	//...
}

MainServiceInterface::Status MainServiceInterface::getStatus() {
	//...
	return StatusNotReady;
}

} /* namespace Multiradio */
