#ifndef GUI_STACK
#define GUI_STACK

#include <string>
#include <list>
#include <list>
#include <string>
#include <utility>
#include <cmath>

#include "texts.h"
#include "ui_keys.h"

enum GuiWindowTypes
{
    mainWindow,\
    messangeWindow,\
    menuWindow,
    endMenuWindow,\
    dialogWindow
};

enum GuiWindowsSubType
{
    //call,
    simpleCondComm,\
    duplCondComm,\
    message,
    recv,\
    data,\
    settings,\
    gpsCoord,\
    gpsSync,\
    setDate,\
    setTime,\
    setFreq,\
    setSpeed,\
    scan,\
    aruarm,\
    suppress,\
    twoState,\
    volume
};

enum modeCall
{
    groupCall,\
    indivCall
};

class CState
{
    GuiWindowTypes type;
    std::string    title;
    std::string    text;
public:
    std::list<CState*> nextState;
    CState*        prevState;

    CState()
    {
        setType(menuWindow);
        nextState.clear();
        prevState = nullptr;
    }
    CState(const char* newName)
    {
        CState();
        setName(newName);
    }
    CState(const char* newTitle, const char* newText)
    {
        CState();
        setName(newTitle);
        setText(newText);
    }
    CState(GuiWindowTypes newType, const char* newName)
    {
        CState();
        setName(newName);
        setType(newType);
        setText("");
    }

    CState(GuiWindowTypes newType, const char* newName, const char* newText)
    {
        CState();
        setName(newName);
        setType(newType);
        setText(newText);
    }

    ~CState(){ title.clear(); text.clear(); }
// private:
    const char* getName(){ return title.c_str(); }
    void setName(const char* newTitle){ title.clear(); title.append(newTitle); }
    int  getType(){ return type; }
    void setType(GuiWindowTypes newType){ type = newType; }
    void setText(const char* newText){ text.clear(); text.append(newText); }
    const char* getText(){ return text.c_str(); }
};

struct SInputItemParameters
{
    char* label;
    std::string inputStr;
    int maxLength, max, min;
};

class CEndState: public CState
{
public:
    CEndState(){ setType(endMenuWindow); }
    virtual ~CEndState(){}

    std::list<SInputItemParameters*> listItem;

    void keyPressed(UI_Key){}

    GuiWindowsSubType subType;
};

class CGuiTree
{
public:
    CGuiTree(){ init(); }
    ~CGuiTree();

    void append(GuiWindowTypes, const char*);
    void append(GuiWindowTypes, const char*, const char*);
    void getLastElement(CState&);
    void delLastElement();
    void resetCurrentState();
    CState &getCurrentState();
    int advance(int);
    int backvard();
    int getPath();

private:
    CGuiTree( const CGuiTree& );
    CGuiTree& operator=( CGuiTree& );

    CState* currentState;
    std::list<CState*> statesStack;
    std::list<int> path;

    CState MainWindow;
    // 0 - 4
    CState main, call, recv, data, settings;
    // 1.1 - 1.5
    CState condCmd, sms, post, groupCondCommand;
    // 1.1.1 - 1.1.2
    CState condCmdSimpl;
    CEndState condCmdDupl;
    SInputItemParameters condCmdDuplParameters1{(char*)callTitle[1], "", 2, 0, 31},\
                         condCmdDuplParameters2{(char*)callTitle[0], "", 2, 0, 31};
    // 1.2.1.1 - 1.2.1.2
    CEndState condCmdSimplGroupCall, condCmdSimplIndivCall;
    SInputItemParameters condCmdSimplGroupCallParameters {(char*)callTitle[0], "", 2, 0, 99},
                         condCmdSimplIndivCallParameters1{(char*)callTitle[1], "", 2, 0, 31},
                         condCmdSimplIndivCallParameters2{(char*)callTitle[0], "", 2, 0, 99};
    // 1.5.1 - 1.5.2
    CState groupCondCommandSimpl;
    CEndState groupCondCommandDupl;
    SInputItemParameters groupCondCommandDuplParameters;
    // 1.5.1.1 - 1.5.1.2
    CEndState groupCondCommandSimplGroupCall, groupCondCommandSimplIndivCall;
    SInputItemParameters groupCondCommandSimplGroupCallParameters,
                         groupCondCommandSimplIndivCallParameters;
    // 2.1 - 2.3
    CEndState recvTlf, recvSms, recvGroupCondCommsnds;
    SInputItemParameters recvTlfParameters, recvSmsParameters, recvGroupCondCommsndsParameters;
    // 3.1 - 3.3
    CState dataRecv, dataSend;
    CEndState dataGps;
    // 3.1.1 - 3.1.4
    CEndState dataRecvCondCmd, dataRecvSms, dataRecvPost, dataRecvGroupCondCmd;

    // 3.2.1 - 3.2.4
    CEndState dataSendCondCmd, dataSendSms, dataSendPost, dataSendGroupCondCmd;
    // 4.1 - 4.3
    CState sttDateTime, sttConnParam;
    CEndState sttScan, swAruArm, sttSound, sttSuppress;
    // 4.1.1 - 4.1.2
    CState sttConnParamHand;
    CEndState sttConnParamGPS;
    // 4.1.2.1 - 4.1.2.2
    CEndState sttSetDate, sttSetTime;
    SInputItemParameters dateParameters, timeParameters;
    // 4.2.1 - 4.2.2
    CEndState sttSetFreq, sttSetSpeed;
    SInputItemParameters freqParameters {(char*)setConnParam[0], "1", 2, 0, 31} ,
                         speedParameters{(char*)setConnParam[1], "2", 2, 0, 31};

    void init();

};




#endif // GUI_STACK
