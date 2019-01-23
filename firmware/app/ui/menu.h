#ifndef MENU
#define MENU

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <string.h>
#include <list>
#include <vector>
#include <chrono>
#include <ctime>
#include <map>
#include <sigc++/sigc++.h>
#include <qmtimer.h>

#include "gui_tree.h"
//#include "service.h"
#include "elements.h"
#include "keyboard.h"
#include "ui_keys.h"
#include "texts.h"
#include "../datastorage/fs.h"

#define no_speah_hack 1

#define flashTest 0

#define smsflashTest_size 27

#if flashTest
    #define grpFlashTest 1 // ���
    #define smsFlashTest 1 // ���
    #define cndFlashTest 1 // ��
#endif

#define MARGIN			4
#define BUTTON_HEIGHT	33
#define BUTTON_WIDTH	30

extern MoonsGeometry ui_common_dialog_area;
extern MoonsGeometry ui_indicator_area;

enum VoiceMailSource
{
	VMS_CHANNEL = 0,
	VMS_TX_FILE = 1,
	VMS_RX_FILE = 2
};

class CGuiDialog: public GUI_Obj
{
public:
    CGuiDialog(MoonsGeometry* area):GUI_Obj(area){}
    virtual ~CGuiDialog(){}
    void Draw();
    MoonsGeometry windowArea;
    MoonsGeometry miniArea;
    MoonsGeometry titleArea;
    LabelParams   titleParams;
    std::string   titleStr;
};

class CGuiMenu: public CGuiDialog
{
public:
    CGuiMenu(MoonsGeometry* area, const char *title, Alignment align);
    virtual ~CGuiMenu();
    void Draw();

    void initItems(std::list<std::string>, const char*, int);
    void initDialog(CEndState);
    void initCondCommDialog(CEndState, bool isSynch = false, bool isWaitingAnswer = false);
    void initGroupCondCmd(CEndState, bool isWatingAnswer = false);
    void initVolumeDialog();
    void initScanDialog();
    void initAruarmDialog();
    void initIncludeDialog();
    void initSuppressDialog();
    void initTwoStateDialog();
    void initGpsCoordinateDialog(char*, char*);
    void initSetParametersDialog(std::string);
    void initSetDateOrTimeDialog(std::string);
    void initSetSpeedDialog(std::string text);
    void initSelectVoiceModeParameters(bool);
    void initSelectChEmissTypeParameters(bool);
    void initFailedSms(int stage);
    void initDialog(std::string text);

    void drawTime(std::string text);

    void initSmsStageDialog(std::string);

    void setTitle(const char*);
    void keyPressed(UI_Key);
    bool isEditing(){ return editing; }

    void setFS(DataStorage::FS* fs);

    void TxCondCmdPackage(int value);    // �������� ��  ������
    int command_tx30 = 0;

    std::string dstAddr, newDstAddr;
    std::string message, newMessage;

    MoonsGeometry getDefaultTitleArea();
    uint8_t focus = 0;
    uint8_t offset = 0;
    uint8_t oldOffset = 0;

    // tx cond cmd
    bool useCmdRetrans = false;
    int txCondCmdStage = 0;
    int condCmdModeSelect = 0;
    void setCondCommParam(CEndState, UI_Key);
    // rx cond cmd
    void initRxCondCmdDialog(bool isSynch = false, bool isStart = false);
    int rxCondCmdStatus = 1;
    bool useTicket = false;

    uint8_t vmProgress;

    // group cond comm stage
    int groupCondCommStage = 0;
    void inputGroupCondCmd(CEndState);

    // put off voice
    std::string  channalNum;
    std::string  voiceAddr;
    int  putOffVoiceStatus = 1;
    void initTxPutOffVoiceDialog(int);
    void initRxPutOffVoiceDialog(int);

    // tx group condition commands
    int txGroupCondCommStatus = 1;
    bool useSndCoord = false;
    bool sndMode = false;

    // message ( SMS )
    uint8_t smsTxStage = 1;
    uint8_t focus_line = 1;
    uint8_t focus_rxline = 1;
    uint8_t max_line = 1;
    bool useSmsRetrans = false;
    void initTxSmsDialog(std::string, std::string);
    void inputSmsMessage(std::string*, UI_Key );
    void inputSmsAddr( std::string*, UI_Key );
    void inputUtc_Menu();
    UI_Key prevKey = keyBack;
    char ch = ' ';
    int keyPressCount = 0;
    std::string smsValueStrStatus;
    uint16_t smsScrollIndex;

    // recv
    bool recvStatus = false;
    uint8_t recvStage = 0;

    // recv sms
    void initRxSmsDialog(std::string, uint8_t stage = 0);

    // volume
    void incrVolume(){ if ( vol < 100) vol += 5; }
    void decrVolume(){ if ( vol >  0 ) vol -= 5; }
    uint8_t  getVolume(){ return vol;}
    void  setVolume(uint8_t volume){vol = volume;}

    // aru arm
    uint8_t getAruArmAsu() { return aruArmAsuStatus[focus]; }
    bool aruArmAsuStatus[3] = {true, true, true};
    uint8_t inclStatus = 1;

    // gps coordinate
    char coord_lat[11];
    char coord_log[12];
    std::string date;
    std::string time;

    // scan
    bool scanStatus = true;

    // supress
    uint8_t supressStatus = 0;

    // date, time, speed
    std::string localDate, localTime, speed;
    void setSttParam(CEndState, UI_Key);

    // RN_KEY
    std::string RN_KEY;
    void initEditRnKeyDialog();

    //SHELDURE
    std::map<int, std::string> sheldure;
    void initSheldureDialog(std::vector<std::string> *data, uint8_t sessionCount);
    int32_t scrollIndex = 0;
    int32_t scrollIndexMax = 0;
    uint8_t sheldureStage = 0;


    //files
    uint8_t filesStage = 0;
    uint8_t firstVisFileElem = 0;
    uint8_t firstVisSheldureElem[2] = {0,0};
    DataStorage::FS::FileType fileType;
    DataStorage::FS::TransitionFileType transitionfileType;

    std::vector<std::string> tFiles[5];
    uint16_t textAreaScrollIndex;
    uint16_t filesScrollIndex;

    uint8_t virtCounter = 0;
    bool isTransmitAsk = false;
    uint8_t qwitCounter = 0;
    uint8_t qwitCounterAll = 0;

    //
    bool useMode = false;
    bool ch_emiss_type = false;

    bool isNeedClearWindow  = true;
    int oldFocus = 0;

    int8_t oldVoiceStatus = 1;
    bool toVoiceMail = false;
    bool inVoiceMail = false;
    PGFONT voiceFont = GUI_EL_TEMP_LabelMode.font;
    PGFONT voiceDigitFont = GUI_EL_TEMP_LabelChannel.font;
    PGFONT smsTitleFont = GUI_EL_TEMP_LabelTitle.font;

    VoiceMailSource old_voiceMailSource = VMS_CHANNEL;
    VoiceMailSource voiceMailSource = VMS_CHANNEL;

    void TxVoiceDialogInitPaint(bool isClear = false);
    void VoiceDialogClearWindow();

    void TxVoiceDialogStatus5(int status, bool isClear = false);
    void TxVoiceDialogStatus4(int status, bool isClear = false);
    void TxVoiceDialogStatus3(int status, bool isClear = false);
    void TxVoiceDialogStatus2(int status, bool isClear = false);
    void TxVoiceDialogStatus1(int status, bool isClear = false);
    void initTxPutOffVoiceDialogTest(int status);

    void RxVoiceDialogStatus5(int status, bool isClear = false);
    void RxVoiceDialogStatus4(int status, bool isClear = false);
    void RxVoiceDialogStatus3(int status, bool isClear = false);
    void RxVoiceDialogStatus2(int status, bool isClear = false);
    void RxVoiceDialogStatus1(int status, bool isClear = false);
    void initRxPutOffVoiceDialogTest(int status);
    void RxSmsStatusPost(int value, bool clear = false, bool clearAll = false);

    void calcFilesCount();
    uint8_t recalcFileFocus(uint8_t focus, DataStorage::FS::FileType f, DataStorage::FS::TransitionFileType t);
    uint8_t calcPercent(uint8_t a, uint8_t b);

    uint8_t cmdCount = 0;
    uint16_t cmdScrollIndex = 0;
    std::chrono::time_point<std::chrono::steady_clock> ct = std::chrono::steady_clock::now();
    bool getIsInRepeatInterval();
    void initFileManagerDialog(uint8_t stage);

    uint8_t filesStageFocus[4] = {1,0,0,0};
    uint8_t sheldureStageFocus[6] = {0,0,0,0,0,0};

    std::vector<uint8_t>* fileMessage;

    uint8_t displayBrightness_tmp = 2;
    uint8_t displayBrightness = 2; //2 - max, 0 - min
    void initDisplayBrightnessDialog();
    std::string sheldureTimeStr;
    std::string sheldureFreqStr;
    void onInputTimer();

	uint8_t maxTransTypeCount = 0;
	uint8_t minTransTypeCount = 0;
	int index_store_sms		  = 0;

	bool isCoordValid = false;
	int currentFrequency;

    GUI_Obj obj;

    uint16_t cmdSymCount = 0;
    uint8_t UtcStatusStatus = 0;
    uint8_t UtcStatusStatusTemp = 0;

private:

    const char* keyChars[10] = {(const char*)&ch_key0, (const char*)&ch_key1, (const char*)&ch_key2, (const char*)&ch_key3, (const char*)&ch_key4,
                                (const char*)&ch_key5, (const char*)&ch_key6, (const char*)&ch_key7, (const char*)&ch_key8, (const char*)&ch_key9};
    uint8_t keyCharsCount[10] = {sizeof ch_key0, sizeof ch_key1, sizeof ch_key2, sizeof ch_key3, sizeof ch_key4,
                                 sizeof ch_key5, sizeof ch_key6, sizeof ch_key7, sizeof ch_key8, sizeof ch_key9};


    MoonsGeometry menuArea;
    MoonsGeometry textArea;

    TextAreaParams textAreaParams;
    std::string textStr;

    MenuItemParams itemParams;
    bool draw_mark;

    int numItem;
    char *tx;
    bool editing;

    uint8_t vol = 100;
    DataStorage::FS* storageFs;

    std::string length_message;

    QmTimer inputTimer;
    uint16_t inputInterval = 600;
    bool isInRepeatIntervalInput = false;

};

#endif // MENU

