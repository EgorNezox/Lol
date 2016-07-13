#ifndef MENU
#define MENU

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <list>

#include "gui_tree.h"
#include "service.h"
#include "elements.h"
#include "keyboard.h"
#include "ui_keys.h"
#include "texts.h"

#include <chrono>
#include <ctime>

#define MARGIN			4
#define BUTTON_HEIGHT	13
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

    void setTitle(const char*);
    void keyPressed(UI_Key);
    bool isEditing(){ return editing; }

    std::string dstAddr, newDstAddr;
    std::string message, newMessage;

    // tx cond cmd
    bool useCmdRetrans = false;
    int txCondCmdStage = 1;
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
    bool useCbool = false;

    // message ( SMS )
    uint8_t smsTxStage = 1;
    bool useSmsRetrans = false;
    void initTxSmsDialog(std::string, std::string);
    void inputSmsMessage(std::string*, UI_Key );
    void inputSmsAddr( std::string*, UI_Key );
    UI_Key prevKey = keyBack;
    char ch = ' ';
    int keyPressCount = 0;
    std::chrono::time_point<std::chrono::steady_clock> ct = std::chrono::steady_clock::now();

    // recv
    bool recvStatus = false;
    uint8_t recvStage = 0;

    // recv sms
    void initRxSmsDialog();

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
    bool supressStatus = true;

    // date, time, speed
    std::string localDate, localTime, speed;
    void setSttParam(CEndState, UI_Key);

    // RN_KEY
    std::string RN_KEY;
    void initEditRnKeyDialog();

private:
    GUI_Obj obj;
    MoonsGeometry menuArea;
    MoonsGeometry textArea;

    TextAreaParams textAreaParams;
    std::string textStr;

    MenuItemParams itemParams;
    MoonsGeometry  itemArea[7];
    bool draw_mark;

    int numItem;
    GUI_EL_MenuItem *item[6];
    char *tx;
    bool editing;

    uint8_t vol = 100;
};


#endif // MENU

