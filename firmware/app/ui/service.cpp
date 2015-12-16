/**
 ******************************************************************************
 * @file    service.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "qm.h"
#include "qmdebug.h"

#include "service.h"

namespace Ui {

bool Service::single_instance = false; // зависимость от единственного дисплея в системе

Service::Service(matrix_keyboard_t matrixkb_desc, aux_keyboard_t auxkb_desc,
		Headset::Controller *headset_controller,
		Multiradio::MainServiceInterface *mr_main_service, Multiradio::VoiceServiceInterface *mr_voice_service,
		Power::Battery *power_battery) {
	QM_UNUSED(matrixkb_desc);
	QM_UNUSED(auxkb_desc);
	QM_UNUSED(headset_controller);
	QM_UNUSED(mr_main_service);
	QM_UNUSED(mr_voice_service);
	QM_UNUSED(power_battery);
	QM_ASSERT(single_instance == false);
	single_instance = true;
	//...
}

Service::~Service() {
	QM_ASSERT(single_instance == true);
	single_instance = false;
	//...
}

void Service::setNotification(NotificationType type) {
	QM_UNUSED(type);
	//...
}

} /* namespace Ui */
