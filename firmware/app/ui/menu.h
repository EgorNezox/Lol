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

#include "gui_tree.h"
//#include "service.h"
#include "elements.h"
#include "keyboard.h"
#include "ui_keys.h"
#include "texts.h"
#include "datastorage/fs.h"

#define MARGIN			4
#define BUTTON_HEIGHT	33
#define BUTTON_WIDTH	30

extern MoonsGeometry ui_common_dialog_area;
extern MoonsGeometry ui_indicator_area;

class CGuiDialog: public GUI_Obj
{
public:
    CGuiDialog(MoonsGeometry* area):GUI_Obj(area){}
    virtual ~CGuiDialog(){}
    void Draw();
    MoonsGeometry windowArea;
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

    uint8_t focus;
    void initItems(std::list<std::string>, const char*, int);
    void initDialog(CEndState);
    void initCondCommDialog(CEndState);
    void initGroupCondCmd(CEndState);
    void initVolumeDialog();
    void initScanDialog();
    void initAruarmDialog();
    void initIncludeDialog();
    void initSuppressDialog();
    void initTwoStateDialog();
    void initGpsCoordinateDialog(std::string, std::string);
    void initSetParametersDialog(std::string);
    void initSetDateOrTimeDialog(std::string);
    void initSetSpeedDialog();
    void initSelectVoiceModeParameters(bool);
    void initSelectChEmissTypeParameters(bool);
    void initFailedSms(int stage);

    void initSmsStageDialog(std::string);

    void setTitle(const char*);
    void keyPressed(UI_Key);
    bool isEditing(){ return editing; }

    void setFS(DataStorage::FS* fs);

    void TxCondCmdPackage(int value);    // �������� ��  ������
    int command_tx30 = 0;

    std::string dstAddr, newDstAddr;
    std::string message, newMessage;

    uint8_t offset = 0;
    uint8_t oldOffset = 0;

    // tx cond cmd
    bool useCmdRetrans = false;
    int txCondCmdStage = 0;
    int condCmdModeSelect = 0;
    void setCondCommParam(CEndState, UI_Key);
    // rx cond cmd
    void initRxCondCmdDialog();
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
    void initTxGroupCondComm(CEndState);
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
    UI_Key prevKey = keyBack;
    char ch = ' ';
    int keyPressCount = 0;
    std::chrono::time_point<std::chrono::steady_clock> ct = std::chrono::steady_clock::now();
    std::string smsValueStrStatus;
    uint8_t smsStage = 0;


    // recv
    bool recvStatus = false;
    uint8_t recvStage = 0;

    // recv sms
    void initRxSmsDialog(std::string);

    // volume
    void incrVolume(){ if ( vol < 100) vol += 5; }
    void decrVolume(){ if ( vol >  0 ) vol -= 5; }
    uint8_t  getVolume(){ return vol;}

    // aru arm
    uint8_t getAruArmAsu() { return aruArmAsuStatus[focus]; }
    bool aruArmAsuStatus[3] = {true, true, true};
    uint8_t inclStatus = 1;

    // gps coordinate
    std::string coord_lat;
    std::string coord_log;
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

    //ZOND
    std::map<int, std::string> sheldure;
    void initZondDialog(int focus, std::vector<std::string> &data);
    int32_t scrollIndex = 0;
    int32_t scrollIndexMax = 0;

    //files
    uint8_t filesStage = 0;
    uint8_t firstVisFileElem = 0;
    DataStorage::FS::FileType fileType;

    std::vector<std::string> tFiles[4];

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

    void initFileManagerDialog(uint8_t stage);
    uint8_t filesStageFocus[3] = {0,0,0};
private:
    GUI_Obj obj;
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
};


#endif // MENU

