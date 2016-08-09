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
#include "datastorage/fs.h"
#include "../mrd/voiceserviceinterface.h"
#include "../mrd/mainserviceinterface.h"
#include "../headset/controller.h"
#include "../power/battery.h"
#include "../navigation/navigator.h"
#include <qmtimer.h>
#include <string.h>


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


class Service : public QmObject //sigc::trackable
{
public:
    Service(matrix_keyboard_t matrixkb_desc,
            aux_keyboard_t auxkb_desc,
            Headset::Controller *headset_controller,
            Multiradio::MainServiceInterface *mr_main_service,
            Multiradio::VoiceServiceInterface *mr_voice_service,
            Power::Battery *power_battery,
            Navigation::Navigator *navigator,
            DataStorage::FS *Fs
			);
    ~Service();
    void setNotification(NotificationType type);
    void updateHeadset(Headset::Controller::Status);
    void updateMultiradio(Multiradio::MainServiceInterface::Status);
    void updateBattery(int);
    void updateAleState(Multiradio::MainServiceInterface::AleState);
    void updateAleVmProgress(uint8_t);
    void updateHSState(Headset::Controller::SmartHSState);
    int  getFreq();
    void setFreq(int isFreq);
    void parsingGucCommand(uint8_t *str);
    void setCoordDate(Navigation::Coord_Date);
    void gucFrame(int value);
    void errorMessage();
    void showDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code);
    void errorGucCrc();
    void setFreqLabelValue(int value);
    void updateSmsStatus(int value);


private:
    void msgBox(const char*);
    void msgBox(const char*, const char*);
    void msgBox(const char*, const int);
    void msgBox(const char*, const int, const int, const int);

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

    DataStorage::FS *storageFs;

    QmTimer *systemTimeTimer;

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

    void GucCoord();

    void FirstPacketPSWFRecieved(int packet);
    void smsMessage();

    void drawMainWindow();

    void drawMenu();
    void draw();
    void drawIndicator();
    void updateSystemTime();
    void msgGucTXQuit(int ans);

    void FailedSms(int stage);

    // gps coordinate
    std::string coord_lat;
    std::string coord_log;
    std::string date;
    std::string time;

    Multiradio::voice_channel_speed_t currentSpeed = Multiradio::voice_channel_speed_t(0);

    int RN_KEY = 1;
    int mainWindowModeId;

    int isFreq = 0;
    int command_rx_30 = 0;
    bool gpsSynchronization = true;
    uint8_t* vect = nullptr;

    //
    std::vector<int> guc_command_vector;
    int position = 0;

    int zond_position = 0;
    std::vector<std::string> zond_data;
};

} /* namespace Ui */

#endif /* FIRMWARE_APP_UI_SERVICE_H_ */
