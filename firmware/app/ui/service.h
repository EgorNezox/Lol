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
#include <time.h>
#include <sigc++/sigc++.h>

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

struct ScheduleTimeSession{
    uint8_t index; // in schedule
    DataStorage::FS::FileType type;
    uint16_t time; // in minutes hour*60+min
};

struct SheldureSession{
    std::string time = "00:00";
    DataStorage::FS::FileType type = DataStorage::FS::FT_SP;
    std::string freq = "10000";

    void clear(){
        time = "";
        type = DataStorage::FS::FT_SP;
        freq = "";
    }

    void reInit(){
        time = "00:00";
        type = DataStorage::FS::FT_SP;
        freq = "10000";
    }

    void copyFrom(SheldureSession* source){
        time = source->time;
        type = source->type;
        freq = source->freq;
    }
};

typedef std::vector<SheldureSession> Sheldure;

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
    void showAtuMalfunction();
    void showDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code);
    void errorGucCrc();
    void setFreqLabelValue(int value);
    void updateSmsStatus(int value);
    void setPswfStatus(bool var);
    void TxCondCmdPackage(int value);
    uint8_t* getGpsGucCoordinat(uint8_t *coord);
    uint8_t &setSheldure();

    std::vector<uint8_t>* onLoadVoiceMail(uint8_t fileNumber, DataStorage::FS::TransitionFileType tft);
    std::vector<uint8_t>* onLoadMessage(DataStorage::FS::FileType typeF, DataStorage::FS::TransitionFileType tft, uint8_t fileNumber);
    void showMessage(const char *title, const char *text);
    void showSchedulePrompt(DataStorage::FS::FileType fileType, uint16_t minutes);
    bool checkSessionTimeSchedule();
    void onScheduleSessionTimer();
    void calcNextSessionIndex();
    void updateSessionTimeSchedule();
    void getCurrentTime(uint8_t *hour, uint8_t *minute, uint8_t *second);
    void loadSheldure();
    void sheldureParsing(uint8_t *sMass);
    void sheldureUnparsing(uint8_t *sMass);
    void uploadSheldure();
    void sheldureToStringList();

    void startSchedulePromptTimer();
    void stopSchedulePromptTimer();
    void showMessage(const char *title, const char *text, MoonsGeometry area);
private:
    void msgBox(const char*);
    void msgBox(const char*, const char*);
    void msgBox(const char*, const int);
    void msgBox(const char*, const int, const int, const int, uint8_t*);
    void msgBoxSms(const char *text);

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

    int condCmdValue = 0 ;
    bool isDrawCondCmd = false;
    DataStorage::FS *storageFs = 0;

    MoonsGeometry promptArea = {15,62,140,124};

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

    void onSmsCounterChange(int param);
    bool isSmsCounterFull = false;

    void GucCoord();

    void FirstPacketPSWFRecieved(int packet);
    void smsMessage(int value);

    void drawMainWindow();

    void drawMenu();
    void draw();
    void drawIndicator();
    void updateSystemTime();
    void msgGucTXQuit(int ans);

    void FailedSms(int stage);
    void startRxQuit();

    void readSynchMode();
    // gps coordinate
    std::string coord_lat;
    std::string coord_log;
    std::string date;
    std::string timeStr;

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
    int sheldure_position = 0;
    std::vector<std::string> sheldure_data;
    bool pswf_status;

    int cntSmsRx = 1;
    int cntGucRx = 1;
    bool isSmsMessageRec = false;
    bool failFlag;

    uint8_t cmdSpaceCount = 0;
    uint8_t cmdDigitCount = 0;
    uint8_t cmdDigitCountLast = 0;
    bool isLastFreeSym = false;

    std::vector<uint8_t> fileMessage;
    bool flashTestOn = false;
    std::vector<uint8_t> condMsg;

    void setColorScheme(uint32_t back,uint32_t front);
    uint8_t* sheldureMass = 0;  // 651 = 50 ������� �� 13 ���� + 1 ���� ���-�� �������
    QmTimer schedulePromptTimer;
    QmTimer schedulePromptRedrawTimer;

    std::vector<ScheduleTimeSession> sessionList;
    uint8_t nextSessionIndex = 0;

    Sheldure sheldure;
    SheldureSession tempSheldureSession;
    std::string schedulePromptText = "";
    bool isShowSchedulePrompt = false;
    bool setAsk = false;

    QmTimer synchModeTimer;

};

} /* namespace Ui */

#endif /* FIRMWARE_APP_UI_SERVICE_H_ */
