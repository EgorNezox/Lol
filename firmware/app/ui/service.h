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
#include "../navigation/navigator.h"


#include "gui_obj.h"
#include "menu.h"
#include "gui_tree.h"

/*FORWARD DECLARATIONS*/
class GUI_Dialog_MainScr;
class GUI_Indicator;
class GUI_Dialog_MsgBox;
class CGuiMenu;



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
	NotificationMissingVoiceChannelsTable,
	NotificationMissingOpenVoiceChannels,
	NotificationMismatchVoiceChannelsTable
};


class Service : public sigc::trackable {
public:
    Service(matrix_keyboard_t matrixkb_desc,
            aux_keyboard_t auxkb_desc,
			Headset::Controller *headset_controller,
            Multiradio::MainServiceInterface *mr_main_service,
            Multiradio::VoiceServiceInterface *mr_voice_service,
            Power::Battery *power_battery,
            Navigation::Navigator *navigator);
	~Service();
	void setNotification(NotificationType type);
    // ������ �����������
    void updateHeadset(Headset::Controller::Status);
    void updateMultiradio(Multiradio::MainServiceInterface::Status);
    void updateBattery(int);
    int  getFreq();
    void setFreq(int isFreq);


    void setCoordDate(Navigation::Coord_Date);
    void getPSWF();

private:
    void msgBox(const char *text);
    void msgBox(const char* title, const char *text);

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
    Navigation::Navigator *navigator;
    GUI_Dialog_MainScr  *main_scr;
    GUI_Indicator       *indicator;
    GUI_Dialog_MsgBox   *msg_box;

    CGuiMenu *menu;
    CGuiTree  guiTree;

	void voiceChannelChanged();
	void chNextHandler();
	void chPrevHandler();
	void keyPressed(UI_Key key);
	void keyHandler(int key_id, QmMatrixKeyboard::PressType pr_type);
    Headset::Controller *pGetHeadsetController();
    Multiradio::MainServiceInterface* pGetMultitradioService();
    Multiradio::VoiceServiceInterface* pGetVoiceService();
    Power::Battery * pGetPowerBattery();
    int getLanguage();

    void FirstPacketPSWFRecieved(int packet);
    void smsMessage();

    void drawMainWindow();

    void drawMenu();
    void draw();
    void drawIndicator();
    void updateSystemTime();

    void FailedSms(int stage);

    int RN_KEY = 1;
    int mainWindowModeId;

    int isFreq = 0;
    int command_rx_30 = 0;
    std::list<int *> BasePswfCadr;

};

} /* namespace Ui */

#endif /* FIRMWARE_APP_UI_SERVICE_H_ */
