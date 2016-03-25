#ifndef GUI_STACK
#define GUI_STACK

#include <string>
#include <vector>
#include "texts.h"

enum GuiWindowTypes
{
    mainWindow,\
    messangeWindow,\
    menuWindow,
    dialogWindow
};

class CState
{
    GuiWindowTypes type;
    std::string name;
public:
    std::vector<CState*> nextState;
    CState         *prevState;

public:
    CState(){ setType(menuWindow); nextState.clear(); prevState = nullptr; }
    CState(const char *newName){ CState(); setName(newName); }
    CState(GuiWindowTypes newType, const char *newName){ CState(); setName(newName); setType(newType); }
    const char* getName(){ return name.c_str(); }
    void  setName(const char* newName){ name.clear(); name.append(newName); }
    int getType(){ return type; }
    void setType(GuiWindowTypes newType){ type = newType; }
};

class CEndState: public CState
{
    //
public:
    int chack();
};

class CGuiTree
{
public:
//    static CGuiTree& getInstance()
//    {
//         instance;
//        return instance;
//    }

    ~CGuiTree();

    void append(GuiWindowTypes, char*);
    void getLastElement(CState&);
    void delLastElement();
    CState getCurrentState();
    void resetCurrentState();
    int advance(int);
    int backvard();

    CGuiTree(){ init(); }
private:
    CGuiTree( const CGuiTree& );
    CGuiTree& operator=( CGuiTree& );

    CState* currentState;
    std::vector<CState*> statesStack;

    CState MainWindow;
    // 0 - 4
    CState main, call, recv, data, settings;
    // 1.1 - 1.5
    CState condCmd, sms, post, groupCondCommand;
    // 1.1.1 - 1.1.2
    CState condCmdSimpl, condCmdDupl;
    // 1.2.1.1 - 1.2.1.2
    CState condCmdSimplGroupCall, condCmdSimplIndivCall;
    // 1.3.1 - 1.3.2
    CState smsNoRetrans, smsRetrans;
    // 1.5.1 - 1.5.2
    CState groupCondCommandSimpl, groupCondCommandDupl;
    // 1.5.1.1 - 1.5.1.2
    CState groupCondCommandSimplGroupCall, groupCondCommandSimplIndivCall;
    // 2.1 - 2.3
    CState recvTlf, recvSms, recvGroupCondCommsnds;
    // 3.1 - 3.3
    CState dataRecv, dataSend;
    // 3.1.1 - 3.1.4
    CState dataRecvCondCmd, dataRecvSms, dataRecvPost, dataRecvGroupCondCmd;
    // 3.2.1 - 3.2.4
    CState dataSendCondCmd, dataSendSms, dataSendPost, dataSendGroupCondCmd;
    // 4.1 - 4.3
    CState sttDateTime, sttConnParam, sttScan;
    // 4.1.1 - 4.1.2
    CState sttConnParamGPS, sttConnParamHand;
    // 4.1.2.1 - 4.1.2.2
    CState sttSetDate, sttSetTime;
    // 4.2.1 - 4.2.2
    CState sttSetFreq, sttSetSpeed;

    void init();

};




#endif // GUI_STACK
