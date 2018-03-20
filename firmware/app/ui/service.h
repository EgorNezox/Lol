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

#include <qmtimer.h>
#include "qmmatrixkeyboard.h"
#include "qmpushbuttonkey.h"
#include "keyboard.h"
#include "ui_keys.h"
#include <sigc++/sigc++.h>
#include "gui_obj.h"
#include "menu.h"
#include "gui_tree.h"
#include "sheldurer.h"
#include "../mrd/voiceserviceinterface.h"
#include "../headset/controller.h"
#include "../power/battery.h"
#include "../navigation/navigator.h"
#include "../mrd/aleservice.h"

/*FORWARD DECLARATIONS*/
class GUI_Dialog_MainScr;
class GUI_Indicator;
class GUI_Dialog_MsgBox;
class CGuiMenu;

namespace Headset {
    class Controller;
}
namespace Multiradio {
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

class Service : public QmObject
{
public:
    Service(matrix_keyboard_t 					matrixkb_desc,
            aux_keyboard_t 						auxkb_desc,
            Headset::Controller 				*headset_controller,
            Multiradio::VoiceServiceInterface 	*mr_voice_service,
            Power::Battery 						*power_battery,
            Navigation::Navigator 				*navigator,
            DataStorage::FS 					*Fs
			);
    ~Service();


    bool checkSessionTimeSchedule();
    void onScheduleSessionTimer();
    void calcNextSessionIndex();
    void updateSessionTimeSchedule();

    void    loadSheldure();
    void    uploadSheldure();
    uint8_t &setSheldure();

    void showAtuMalfunction();
    void onStartCondReceiving();

    void playSchedulePromptSignal();
    void startSchedulePromptTimer();
    void stopSchedulePromptTimer();

    void updateHeadset				   (Headset::Controller::Status);
    void updateMultiradio			   (Multiradio::VoiceServiceInterface::Status);

    void batteryVoltageChanged		   (int newVoltage);
    void batteryChargeChanged		   (int newVoltage);
    void updateBattery();

    void updateAleState 			   (AleState);
    void updateAleVmProgress		   (uint8_t);
    void updateHSState				   (Headset::Controller::SmartHSState);

    int  getFreq();
    void parsingGucCommand			   (uint8_t *str);
    void setCoordDate				   (Navigation::Coord_Date);
    void gucFrame					   (int value, bool isTxAsk);

    void errorGucCrc();
    void setFreqLabelValue		       (int value);
    void updateSmsStatus			   (int value);
    void setPswfStatus			       (bool var);
    void TxCondCmdPackage			   (int value);
    uint8_t* getGpsGucCoordinat	       (uint8_t *coord);

    void showMessage	   			   (const char *title, const char *text);
    void showSchedulePrompt		       (DataStorage::FS::FileType fileType, uint16_t minutes);

    void sheldureToStringList();
    void sheldureParsing			   (uint8_t *sMass);
    void sheldureUnparsing			   (uint8_t *sMass);

    void getCurrentTime				   (uint8_t *hour, uint8_t *minute, uint8_t *second);
    void setNotification			   (NotificationType type);
    void showMessage				   (const char *title, const char *text, MoonsGeometry area);
    void showDspHardwareFailure		   (uint8_t subdevice_code, uint8_t error_code);

    void onTransmitAsk					(bool on);
    void onQwitCounterChanged	  		(uint8_t counter);
    void onVirtualCounterChanged  		(uint8_t counter);
    void onCompletedStationMode	  		(bool isGoToVoice = true);
    void onDelaySpeachStateChanged		(bool isOn);

    void emulkeyHandler(int key);

    void playSoundSignal				(uint8_t mode, uint8_t speakerVolume, uint8_t gain, uint8_t soundNumber, uint8_t duration, uint8_t micLevel);
    std::vector<uint8_t>* loadMessage	(DataStorage::FS::FileType typeF, DataStorage::FS::TransitionFileType tft, uint8_t fileNumber);
    std::vector<uint8_t>* loadVoiceMail	(uint8_t fileNumber, DataStorage::FS::TransitionFileType tft);

private:
    void msgBox							(const char*);
    void msgBox							(const char*, const char*);
    void msgBox							(const char*, const int);
    void msgBox							(const char*, const int, const int, const int, uint8_t*);
    void msgBoxSms						(const char *text);

    void chNextHandler();
    void chPrevHandler();
    void keyPressed(UI_Key key);

    void keyHandler					    (int key_id, QmMatrixKeyboard::PressType pr_type);
    void FirstPacketPSWFRecieved		(int packet, uint8_t address, bool isRec);

    void msgGucTXQuit					(int ans);
    void onRecievingBatteryVoltage	    (int voltage);
    void onWaveInfoRecieved				(float wave, float power);


    void setColorScheme					(uint32_t back,uint32_t front);
    void smsMessage						(int value);
    void FailedSms						(int stage);
    void onSmsCounterChange				(int param);

    void setFreq();
    void readSynchMode();

    void startTest();
    void onDspStarted();
    void getBatteryVoltage();

    void checkHeadsetStatus();
    void garnitureStart();

    void voiceChannelChanged();
    void resetLogicDSPforGarniture();

    void draw();
    void drawMenu();
    void drawIndicator();
    void drawMainWindow();

    void showReceivedSms();
    void updateSystemTime();
    void onTestMsgTimer();
    void startRxQuit();

    matrix_keyboard_t      matrix_kb;
    aux_keyboard_t         aux_kb;
    static bool 		   single_instance;

    Headset::Controller    *headset_controller;
    Headset::Controller    *pGetHeadsetController();

    Power::Battery   	   *power_battery;
    Power::Battery 	       *pGetPowerBattery();

    QmMatrixKeyboard       *keyboard;
    QmPushButtonKey        *chnext_bt;
    QmPushButtonKey        *chprev_bt;

    GUI_Dialog_MainScr     *main_scr;
    GUI_Dialog_MsgBox      *msg_box = nullptr;

    Navigation::Navigator  *navigator;
    GUI_Indicator          *indicator;

    CGuiMenu 			   *menu;
    CGuiTree  			   guiTree;

    DataStorage::FS        *storageFs = 0;
    MoonsGeometry          promptArea   = {15,62,140,124};

    std::string 		   coord_lat;
    std::string 		   coord_log;
    std::string 		   date;
    std::string 		   timeStr;
    std::string 		   schedulePromptText = "";

    std::vector<uint8_t>   fileMsg;
    std::vector<uint8_t>   condMsg;
    std::vector<int> 	   guc_command_vector;

    std::vector<std::string> 		 sheldure_data;
    std::vector<ScheduleTimeSession> sessionList;

    QmTimer schedulePromptTimer;
    QmTimer synchModeTimer;
    QmTimer testMsgTimer;
    QmTimer schedulePromptRedrawTimer;
    QmTimer *systemTimeTimer;

    Sheldure sheldure;
    SheldureSession tempSheldureSession;

    char gucCoords[26];

    bool pswf_status;
    bool isVm 				  = false;
    bool isStartCond          = false;
    bool isWaitAnswer         = false;
    bool isGucAnswerWaiting   = false;
    bool isFirstInputFreqEdit = false;

    bool isStartTestMsg       = true;
    bool isDspStarted         = false;
    bool isShowSchedulePrompt = false;
    bool setAsk               = false;
    bool isGucCoord           = false;

    bool gpsSynchronization   = true;
    bool isChangeGpsSynch     = false;
    bool isVolumeEdit 		  = false;

    bool isSmsMessageRec      = false;
    bool failFlag             = false;
    bool isLastFreeSym        = false;
    bool flashTestOn          = false;
    bool isValidGpsTime       = false;

    bool isTurnGuc 			  = false;
    bool isCondModeQwitTx     = false;
    bool isGucModeQwitTx      = false;
    bool isSmsCounterFull     = false;
    bool isDrawCondCmd        = false;

    bool inDateMenu			  = false;

    uint8_t gucAdd            = 0;
    uint8_t cmdSpaceCount     = 0;
    uint8_t cmdDigitCount     = 0;
    uint8_t cmdDigitCountLast = 0;
    uint8_t volumeLevel       = 0;
    uint8_t curMode           = 1; // rx = 1; tx = 2
    uint8_t nextSessionIndex  = 0;
    uint8_t position          = 0;
    uint8_t channelNumberSyms = 0;

    int RN_KEY 				  = 1;
    int sheldure_position     = 0;
    int isFreq                = 0;
    int command_rx_30         = 0;
    int cntSmsRx              = -1;
    int cntGucRx              = -1;
    int oldChannelNumber      = 2;
    int channelNumberEditing  = 0;
    int condCmdValue          = 0;

    uint8_t* vect         = nullptr;
    uint8_t* sheldureMass = 0;
    // 651 = 50 сеансов по 13 байт + 1 байт кол-во сеансов

    int valueRxSms;
    int mainWindowModeId;



    float 	waveValue  = 0;
    float 	powerValue = 0;
    uint32_t curAleFreq = 0;

    void onRxModeSetting();
    void onTxModeSetting();
    void drawWaveInfo();
    void onSettingAleFreq				(uint32_t freq);

    void mainWindow_keyPressed			(UI_Key key);
    void messangeWindow_keyPressed		(UI_Key key);
    void menuWindow_keyPressed			(UI_Key key);
    void endMenuWindow_keyPressed		(UI_Key key);

    void condCommand_keyPressed			(UI_Key key);
    void txGroupCondCmd_keyPressed		(UI_Key key);
    void txPutOffVoice_keyPressed		(UI_Key key);
    void txSmsMessage_keyPressed		(UI_Key key);
    void recvVoice_keyPressed			(UI_Key key);
    void recvCondCmd_keyPressed			(UI_Key key);
    void rxSmsMessage_keyPressed		(UI_Key key);
    void recvGroupCondCmd_keyPressed	(UI_Key key);
    void rxPutOffVoice_keyPressed		(UI_Key key);
    void volume_keyPressed				(UI_Key key);
    void scan_keyPressed				(UI_Key key);
    void suppress_keyPressed			(UI_Key key);
    void display_keyPressed				(UI_Key key);
    void aruarmaus_keyPressed			(UI_Key key);
    void gpsCoord_keyPressed			(UI_Key key);
    void gpsSync_keyPressed				(UI_Key key);
    void setDate_keyPressed				(UI_Key key);
    void setTime_keyPressed				(UI_Key key);
    void setFreq_keyPressed				(UI_Key key);
    void setSpeed_keyPressed			(UI_Key key);
    void editRnKey_keyPressed			(UI_Key key);
    void voiceMode_keyPressed			(UI_Key key);
    void channelEmissionType_keyPressed (UI_Key key);
    void filetree_keyPressed			(UI_Key key);
    void sheldure_keyPressed			(UI_Key key);

    void condCommand_keyPressed_stage   (UI_Key key);
    void condCommand_enter_keyPressed();
    void condCommand_back_keyPressed();
    void condCommand_send();

    std::string rememberTextSms;

    Multiradio::VoiceServiceInterface *voice_service;
    Multiradio::VoiceServiceInterface *pGetVoiceService();
    Multiradio::VoiceServiceInterface::Status multiradioStatus;
    Multiradio::voice_channel_speed_t currentSpeed = Multiradio::voice_channel_speed_t(0);
};
} /* namespace Ui */
#endif /* FIRMWARE_APP_UI_SERVICE_H_ */
