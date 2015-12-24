/**
 ******************************************************************************
 * @file    mainserviceinterface.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_MRD_MAINSERVICEINTERFACE_H_
#define FIRMWARE_APP_MRD_MAINSERVICEINTERFACE_H_

#include "qmobject.h"

namespace Multiradio {

class Dispatcher;

class MainServiceInterface : QmObject
{
public:
	enum Status {
		StatusNotReady,
		StatusIdle,
		StatusVoiceRx,
		StatusVoiceTx
	};

	Status getStatus();

	sigc::signal<void, Status/*new_status*/> statusChanged;

private:
	friend class Dispatcher;

	MainServiceInterface(Dispatcher *dispatcher);
	~MainServiceInterface();
	void setStatus(Status value);

	Status current_status;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_MAINSERVICEINTERFACE_H_ */
