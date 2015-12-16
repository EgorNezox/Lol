/**
 ******************************************************************************
 * @file    service.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_UI_SERVICE_H_
#define FIRMWARE_APP_UI_SERVICE_H_

#include "keyboard.h"

namespace Headset {
    class Controller;
}
namespace Multiradio {
	class MainServiceInterface;
    class VoiceServiceInterface;
}
namespace Power {
    class Battery;
}

namespace Ui {

enum NotificationType {
	NotificationMissingVoiceChannelsTable
};

class Service {
public:
	Service(matrix_keyboard_t matrixkb_desc, aux_keyboard_t auxkb_desc,
			Headset::Controller *headset_controller,
			Multiradio::MainServiceInterface *mr_main_service, Multiradio::VoiceServiceInterface *mr_voice_service,
			Power::Battery *power_battery);
	~Service();
	void setNotification(NotificationType type);

private:
	static bool single_instance;
};

} /* namespace Ui */

#endif /* FIRMWARE_APP_UI_SERVICE_H_ */
