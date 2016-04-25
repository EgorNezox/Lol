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

//#include <QDebug>

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

    int focus;
    void initItems(std::list<std::string>, const char*, int);
    void initDialog(CEndState);
    void initCondCommDialog(CEndState);
    void initVolumeDialog();
    void initScanDialog();
    void initAruarmDialog();
    void initIncludeDialog();
    void initSuppressDialog();
    void initTwoStateDialog();
    void initGpsCoordinateDialog();
    void initSetParametersDialog(std::string);

    void setTitle(const char*);
    void keyPressed(UI_Key);
    bool isEditing(){ return editing; }

    std::string dstAddr, newDstAddr;
    std::string message, newMessage;

    // call
    void setCondCommParam(CEndState, UI_Key);

    // message ( SMS )
    void initTxSmsDialog(const char *, std::string, std::string);
    void inputSmsMessage( CEndState, UI_Key );
    void inputSmsAddr( CEndState, UI_Key );
    UI_Key prevKey = keyBack;
    char ch = ' ';
    int keyPressCount = 0;
    std::chrono::time_point<std::chrono::steady_clock> ct = std::chrono::steady_clock::now();

    // recv sms
    void initRxSmsDialog();

    // volume
    void incrVolume(){ if ( vol < 100) vol += 5; }
    void decrVolume(){ if ( vol >  0 ) vol -= 5; }
    uint8_t  getVolume(){ return vol;}

    // aru arm
    void    incrAruArmAsu(GuiWindowsSubType);
    void    decrAruArmAsu(GuiWindowsSubType);
    uint8_t getAruArmAsu() { return aruArmAsuStatus[focus]; }
    uint8_t aruArmAsuStatus[3] = {1, 1, 1};
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
    GUI_EL_MenuItem *(item[6]);
    GUI_EL_Label *label[6];
    char *tx;
    bool editing = false;

    uint8_t vol = 100;
};


#endif // MENU

