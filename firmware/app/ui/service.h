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


#include "qmmatrixkeyboard.h"
#include "qmpushbuttonkey.h"
#include "keyboard.h"
#include "ui_keys.h"
#include "../mrd/voiceserviceinterface.h"
#include "../mrd/mainserviceinterface.h"
#include "../headset/controller.h"
#include "../power/battery.h"

/*FORWARD DECLARATIONS*/
class GUI_Dialog_MainScr;
class GUI_Indicator;

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


class Service : public sigc::trackable {
public:
	Service(matrix_keyboard_t matrixkb_desc, aux_keyboard_t auxkb_desc,
			Headset::Controller *headset_controller,
			Multiradio::MainServiceInterface *mr_main_service, Multiradio::VoiceServiceInterface *mr_voice_service,
			Power::Battery *power_battery);
	~Service();
	void setNotification(NotificationType type);

	void keyHandler(int key_id, QmMatrixKeyboard::PressType pr_type);
	Headset::Controller *pGetHeadsetController();
	Multiradio::MainServiceInterface* pGetMultitradioService();
	Multiradio::VoiceServiceInterface* pGetVoiceService();
	Power::Battery * pGetPowerBattery();
private:
	matrix_keyboard_t matrix_kb;
	aux_keyboard_t aux_kb;
	Headset::Controller *headset_controller;
	Multiradio::MainServiceInterface *multiradio_service;
	Multiradio::VoiceServiceInterface *voice_service;
	Power::Battery *power_battery;
	QmMatrixKeyboard *keyboard;
	QmPushButtonKey *chnext_bt;
	QmPushButtonKey *chprev_bt;
	static bool single_instance;
	GUI_Dialog_MainScr *main_scr;
	GUI_Indicator *indicator;
	void chNextHandler();
	void chPrevHandler();
	void keyPressed(UI_Key key);
};

} /* namespace Ui */

#endif /* FIRMWARE_APP_UI_SERVICE_H_ */
