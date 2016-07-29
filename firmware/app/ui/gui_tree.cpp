#include "gui_tree.h"

void CGuiTree::init()
{
    MainWindow.setName((const char*)"������-�");
    MainWindow.setType(mainWindow);
    // 0 - 4
    main.setName(mainMenu[0]); call.setName(mainMenu[1]); recv.setName(mainMenu[2]); data.setName(mainMenu[3]); settings.setName(mainMenu[4]);
    // 1.1 - 1.4
    condCmd.setName(callSubMenu[0]); sms.setName(callSubMenu[1]); txPutOffVoice.setName(callSubMenu[2]); groupCondCommand.setName(callSubMenu[3]);
    // 1.2.1 - 1.2.2
//    condCmdSimpl.setName(commandsSubMenu[0]); condCmdDupl.setName(commandsSubMenu[1]);
    condCmdSimpl.setName(commandsSubMenu[0]); condCmdDupl.setName(commandsSubMenu[1]);
    // 1.2.1.1 - 1.2.1.2
    condCmdSimplGroupCall.setName(smplSubMenu[0]); condCmdSimplIndivCall.setName(smplSubMenu[1]);
    // 1.5.1 - 1.5.2
    groupCondCommandSimpl.setName(groupCommandsSubMenu[0]); groupCondCommandDupl.setName(groupCommandsSubMenu[1]);
    // 1.5.1.1 - 1.5.1.2
    groupCondCommandSimplGroupCall.setName(groupCommandsSimplSubMenu[0]); groupCondCommandSimplIndivCall.setName(groupCommandsSimplSubMenu[1]);
    // 2.1 - 2.3
    recvVoice.setName(reciveSubMenu[0]); recvSms.setName(reciveSubMenu[1]); rxPutOffVoice.setName(reciveSubMenu[2]); recvCondCommand.setName(reciveSubMenu[3]); recvGroupCondCommsnds.setName(reciveSubMenu[4]); recvSilence.setName(reciveSubMenu[5]);
    // 3.1 - 3.3
    dataRecv.setName(dataSubMenu[0]), dataSend.setName(dataSubMenu[1]), dataGps.setName(dataSubMenu[3]);
    // 3.1.1 - 3.1.4
    dataRecvCondCmd.setName(dataSubSubMenu[0]); dataRecvSms.setName(dataSubSubMenu[1]); dataRecvPost.setName(dataSubSubMenu[2]); dataRecvGroupCondCmd.setName(dataSubSubMenu[3]);
    // 3.2.1 - 3.2.4
    dataSendCondCmd.setName(dataSubSubMenu[0]); dataSendSms.setName(dataSubSubMenu[1]); dataSendPost.setName(dataSubSubMenu[2]); dataSendGroupCondCmd.setName(dataSubSubMenu[3]);
    // 4.1 - 4.2
    sttDateTime.setName(settingsSubMenu[0]);
    sttConnParam.setName(settingsSubMenu[1]);
    sttScan.setName(settingsSubMenu[2]);
    swAruArm.setName(settingsSubMenu[3]);
    sttSound.setName(settingsSubMenu[4]);
    sttSuppress.setName(settingsSubMenu[5]);
    sttWaitGuk.setName(settingsSubMenu[6]);
    sttEditRnKey.setName(settingsSubMenu[7]);

    // 4.1.1 - 4.1.2
    sttConnParamGPS.setName(dateAndTimeSubMenu[0]); sttConnParamHand.setName(dateAndTimeSubMenu[1]);
    // 4.1.2.1 - 4.1.2.2
    sttSetDate.setName(setDateOrTime[0]); sttSetTime.setName(setDateOrTime[1]);
    // 4.2.1 - 4.2.2
    sttSetFreq.setName(setConnParam[0]); sttSetSpeed.setName(setConnParam[1]);

    MainWindow.prevState = nullptr;
    MainWindow.nextState.push_back(&main);
    // 0
    main.prevState = &MainWindow;
    main.nextState.push_back(&call);
    main.nextState.push_back(&recv);
    main.nextState.push_back(&data);
    main.nextState.push_back(&settings);
    // 1 - �����
    call.prevState = &main;
    call.nextState.push_back(&condCmd);
    call.nextState.push_back(&sms);
    call.nextState.push_back(&txPutOffVoice);
    call.nextState.push_back(&groupCondCommand);
    // 1.1 - �������� �������
    condCmd.prevState = &call;
    condCmd.nextState.push_back(&condCmdSimpl);
    condCmd.nextState.push_back(&condCmdDupl);
    // 1.1.1 - ������.�����
    condCmdSimpl.prevState = &condCmd;
    condCmdSimpl.nextState.push_back(&condCmdSimplGroupCall);
    condCmdSimpl.nextState.push_back(&condCmdSimplIndivCall);
    // 1.1.1.1 - ��������� �����
    condCmdSimplGroupCall.subType = GuiWindowsSubType::simpleCondComm;
    condCmdSimplGroupCall.prevState = &condCmdSimpl;
    condCmdSimplGroupCall.nextState.clear();
    condCmdSimplGroupCall.listItem.push_back(&condCmdSimplGroupCallParameters1 );
    condCmdSimplGroupCall.listItem.push_back(&condCmdSimplGroupCallParameters2 );
    // 1.1.1.2 - ��������������
    condCmdSimplIndivCall.subType = GuiWindowsSubType::simpleCondComm;
    condCmdSimplIndivCall.prevState = &condCmdSimpl;
    condCmdSimplIndivCall.nextState.clear();
    condCmdSimplIndivCall.listItem.push_back( &condCmdSimplIndivCallParameters1 );
    condCmdSimplIndivCall.listItem.push_back( &condCmdSimplIndivCallParameters2 );
    condCmdSimplIndivCall.listItem.push_back( &condCmdSimplIndivCallParameters3 );
    // 1.1.2 - ������.����� | ������ �������������
    condCmdDupl.subType = GuiWindowsSubType::duplCondCmd;
    condCmdDupl.prevState = &condCmd;
    condCmdDupl.nextState.clear();
    condCmdDupl.listItem.push_back( &condCmdDuplParameters1 );
    condCmdDupl.listItem.push_back( &condCmdDuplParameters2 );
    condCmdDupl.listItem.push_back( &condCmdDuplParameters3 );
    // 1.2 - SMS
    sms.subType = GuiWindowsSubType::message;
    sms.prevState = &call;
    sms.nextState.clear();
    sms.listItem.push_back(&smsParameters1);
    sms.listItem.push_back(&smsParameters2);
    sms.listItem.push_back(&smsParameters3);
    // 1.3 - ��������� �����
    txPutOffVoice.subType = GuiWindowsSubType::txPutOffVoice;
    txPutOffVoice.prevState = &call;
    txPutOffVoice.nextState.clear();
    // 1.4 - ������ �������� ������
//    groupCondCommand.subType = GuiWindowsSubType::txGroupCondCmd;
    groupCondCommand.prevState = &call;
    groupCondCommand.nextState.clear();
//    groupCondCommand.listItem.push_back(&groupCondCommandParameters1);
//    groupCondCommand.listItem.push_back(&groupCondCommandParameters2);
//    groupCondCommand.listItem.push_back(&groupCondCommandParameters3);
//    groupCondCommand.listItem.push_back(&groupCondCommandParameters4);
//    groupCondCommand.nextState.push_back(&groupCondCommandSimpl);k
    groupCondCommand.nextState.push_back(&groupCondCommandDupl);
    // 1.4.1 - ������������� �����
    groupCondCommandSimpl.prevState = &groupCondCommand;
    groupCondCommandSimpl.nextState.push_back(&groupCondCommandSimplGroupCall);
    groupCondCommandSimpl.nextState.push_back(&groupCondCommandSimplIndivCall);
    // 1.4.1.1 - ��������� �����
    groupCondCommandSimplGroupCall.subType = GuiWindowsSubType::txGroupCondCmd;
    groupCondCommandSimplGroupCall.prevState = &groupCondCommandSimpl;
    groupCondCommandSimplGroupCall.nextState.clear();
    groupCondCommandSimplGroupCall.listItem.push_back(&groupCondCommandSimplCallParameters[0]);
    groupCondCommandSimplGroupCall.listItem.push_back(&groupCondCommandSimplCallParameters[1]);
    groupCondCommandSimplGroupCall.listItem.push_back(&groupCondCommandSimplCallParameters[2]);
    // 1.4.1.2 - ��������������
    groupCondCommandSimplIndivCall.subType = GuiWindowsSubType::txGroupCondCmd;
    groupCondCommandSimplIndivCall.prevState = &groupCondCommandSimpl;
    groupCondCommandSimplIndivCall.nextState.clear();
    groupCondCommandSimplIndivCall.listItem.push_back(&groupCondCommandSimplCallParameters[0]);
    groupCondCommandSimplIndivCall.listItem.push_back(&groupCondCommandSimplCallParameters[1]);
    groupCondCommandSimplIndivCall.listItem.push_back(&groupCondCommandSimplCallParameters[2]);
    groupCondCommandSimplIndivCall.listItem.push_back(&groupCondCommandSimplCallParameters[3]);
    // 1.4.2 - ������������� �����
//  groupCondCommandDupl.subType = GuiWindowsSubType::txGroupCondCmd;
    groupCondCommandDupl.prevState = &groupCondCommand;
    groupCondCommandDupl.nextState.clear();

    // 2 - �����
    recv.prevState = &main;
    recv.nextState.push_back(&recvSms);
    recv.nextState.push_back(&recvCondCommand);
    recv.nextState.push_back(&recvGroupCondCommsnds);
    recv.nextState.push_back(&rxPutOffVoice);
    recv.nextState.push_back(&recvSilence);
    // 2.3 - ����
    recvVoice.prevState = &recv;
    recvVoice.nextState.clear();
    // 2.5 - ��������� (���)
    recvSms.subType = GuiWindowsSubType::recvSms;
    recvSms.prevState = &recv;
    recvSms.nextState.clear();
    // 2.6 - ��������� (���)
    rxPutOffVoice.subType = GuiWindowsSubType::rxPutOffVoice;
    rxPutOffVoice.prevState = &recv;
    rxPutOffVoice.nextState.clear();
    // 2.7 - �������� �������
    recvCondCommand.subType = GuiWindowsSubType::recvCondCmd;
    recvCondCommand.prevState = &recv;
    recvCondCommand.nextState.clear();
    // 2.8 - ������ �������� ������
    recvGroupCondCommsnds.subType = GuiWindowsSubType::recvGroupCondCmd;
    recvGroupCondCommsnds.prevState = &recv;
    recvGroupCondCommsnds.nextState.clear();
    // 2.9 - ����� �������������
    recvSilence.subType = GuiWindowsSubType::recvSilence;
    recvSilence.prevState = &recv;
    recvSilence.nextState.clear();
    // 3 - ������
    data.prevState = &main;
//    data.nextState.push_back(&dataRecv);
//    data.nextState.push_back(&dataSend);
    data.nextState.push_back(&dataGps);
    // 3.1 - ��������
    dataRecv.prevState = &data;
    dataRecv.nextState.push_back(&dataRecvCondCmd);
    dataRecv.nextState.push_back(&dataRecvSms);
    dataRecv.nextState.push_back(&dataRecvPost);
    dataRecv.nextState.push_back(&dataRecvGroupCondCmd);
    // 3.1.1 - �������� �������
    dataRecvCondCmd.prevState = &dataRecv;
    dataRecvCondCmd.nextState.clear();
    // 3.1.2 - SMS
    dataRecvSms.prevState = &dataRecv;
    dataRecvSms.nextState.clear();
    // 3.1.3 - ��������� �����
    dataRecvPost.prevState = &dataRecv;
    dataRecvPost.nextState.clear();
    // 3.1.4 - ������ �������� ������
    dataRecvGroupCondCmd.prevState = &dataRecv;
    dataRecvGroupCondCmd.nextState.clear();
    // 3.2 - ������������
    dataSend.prevState = &data;
    dataSend.nextState.push_back(&dataSendCondCmd);
    dataSend.nextState.push_back(&dataSendSms);
    dataSend.nextState.push_back(&dataSendPost);
    dataSend.nextState.push_back(&dataSendGroupCondCmd);
    // 3.2.1 - �������� �������
    dataSendCondCmd.prevState = &dataSend;
    dataSendCondCmd.nextState.clear();
    // 3.2.2 - SMS
    dataSendSms.prevState = &dataSend;
    dataSendSms.nextState.clear();
    // 3.2.3 - ��������� �����
    dataSendPost.prevState = &dataSend;
    dataSendPost.nextState.clear();
    // 3.2.4 - ������ �������� ������
    dataSendGroupCondCmd.prevState = &dataSend;
    dataSendGroupCondCmd.nextState.clear();
    // 3.3 - GPS ����������
    dataGps.subType = GuiWindowsSubType::gpsCoord;
    dataGps.prevState = &data;
    dataGps.nextState.clear();
    // 4 - ���������
    settings.setType(GuiWindowTypes::menuWindow);
    settings.prevState = &main;
    settings.nextState.push_back(&sttDateTime);
    settings.nextState.push_back(&sttConnParam);
    settings.nextState.push_back(&sttScan);
    settings.nextState.push_back(&sttSound);
    settings.nextState.push_back(&sttSuppress);
    // 4.1 - ����/�����
    sttDateTime.setType(GuiWindowTypes::menuWindow);
    sttDateTime.prevState = &settings;
    sttDateTime.nextState.push_back(&sttConnParamGPS);
    sttDateTime.nextState.push_back(&sttConnParamHand);
    // 4.1.1 - GPS ����-���
    sttConnParamGPS.setType(GuiWindowTypes::menuWindow);
    sttConnParamGPS.subType = GuiWindowsSubType::gpsSync;
    sttConnParamGPS.prevState = &sttDateTime;
    sttConnParamGPS.nextState.clear();
    sttConnParamGPS.listItem.push_back(&gpsSynchronization);
    // 4.1.2 - ������ ���������
    sttConnParamHand.setType(GuiWindowTypes::menuWindow);
    sttConnParamHand.prevState = &sttDateTime;
    sttConnParamHand.nextState.push_back(&sttSetDate);
    sttConnParamHand.nextState.push_back(&sttSetTime);
    // 4.1.2.1 - ���������� ����
    sttSetDate.setType(GuiWindowTypes::endMenuWindow);
    sttSetDate.subType = GuiWindowsSubType::setDate;
    sttSetDate.prevState = &sttConnParamHand;
    sttSetDate.nextState.clear();
    sttSetDate.listItem.push_back(&dateParameters);
    // 4.1.2.2 - ���������� �����
    sttSetTime.setType(GuiWindowTypes::endMenuWindow);
    sttSetTime.subType = GuiWindowsSubType::setTime;
    sttSetTime.prevState = &sttConnParamHand;
    sttSetTime.nextState.clear();
    sttSetTime.listItem.push_back(&timeParameters);
    // 4.2 - ��������� �����
    sttConnParam.setType(GuiWindowTypes::menuWindow);
    sttConnParam.prevState = &settings;
    sttConnParam.nextState.push_back(&sttSetFreq);
    sttConnParam.nextState.push_back(&sttSetSpeed);
    sttConnParam.nextState.push_back(&swAruArm);
    sttConnParam.nextState.push_back(&sttWaitGuk);
    sttConnParam.nextState.push_back(&sttEditRnKey);
    // 4.2.1 - �������
    sttSetFreq.setType(GuiWindowTypes::endMenuWindow);
    sttSetFreq.subType = GuiWindowsSubType::setFreq;
    sttSetFreq.prevState = &sttConnParam;
    sttSetFreq.nextState.clear();
    sttSetFreq.listItem.push_back(&freqParameters);
    // 4.2.2 - ��������
    sttSetSpeed.setType(GuiWindowTypes::endMenuWindow);
    sttSetSpeed.subType = GuiWindowsSubType::setSpeed;
    sttSetSpeed.prevState = &sttConnParam;
    sttSetSpeed.nextState.clear();
    sttSetSpeed.listItem.push_back(&speedParameters);
    // 4.2.3 - ��� / ��� / ���
    swAruArm.subType = GuiWindowsSubType::aruarmaus;
    swAruArm.prevState = &sttConnParam;
    swAruArm.nextState.clear();
    // 4.2.4 - �������� ���
    sttWaitGuk.subType = GuiWindowsSubType::waitGuk;
    sttWaitGuk.prevState = &sttConnParam;
    sttWaitGuk.nextState.clear();
    // 4.2.5 - Edit RN_KEY
    sttEditRnKey.subType = GuiWindowsSubType::editRnKey;
    sttEditRnKey.prevState = &sttConnParam;
    sttEditRnKey.nextState.clear();
    // 4.3 - ������������
    sttScan.subType = GuiWindowsSubType::scan;
    sttScan.prevState = &settings;
    sttScan.nextState.clear();
    // 4.5 - ���������
    sttSound.subType = GuiWindowsSubType::volume;
    sttSound.prevState = &settings;
    sttSound.nextState.clear();
    // 4.6 - ��������������
    sttSuppress.subType = GuiWindowsSubType::suppress;
    sttSuppress.prevState = &settings;
    sttSuppress.nextState.clear();

    currentState = &MainWindow;
    statesStack.clear();
}

CState& CGuiTree::getCurrentState()
{
    if ( statesStack.size() > 0 )
    {
        return *statesStack.back();
    }
    else
    {
        return *currentState;
    }
}

void CGuiTree::resetCurrentState()
{
    currentState = &MainWindow;
    path.clear();
}

int CGuiTree::advance(int i)
{
    if ( currentState->nextState.size() > 0 )
    {
        int j = 0;
        for(auto &k: currentState->nextState)
        {
            if (i == j)
            {
                currentState = k;
                path.push_back(i);
                break;
            }
            j++;
        }
        return 1;
    }
    else
        return -1;
}

int CGuiTree::backvard()
{
    if ( currentState->prevState != nullptr )
    {
        currentState = currentState->prevState;
        path.pop_back();
        return 1;
    }
    else
        return -1;
}

void CGuiTree::append(GuiWindowTypes type, const char* title)
{
    statesStack.push_back(&*(new CState(type, title)));
}

void CGuiTree::append(GuiWindowTypes type, const char* title, const char* text)
{
    statesStack.push_back(&*(new CState(type, title, text)));
}

void CGuiTree::getLastElement( CState& st )
{
    if (statesStack.size() > 0)
    {
        st = *statesStack.back();
    }
    else
        st = *currentState;
}

void CGuiTree::delLastElement()
{
    if ( statesStack.size() > 0 )
    {
        delete statesStack.back();
        statesStack.pop_back();
    }
}

int CGuiTree::getPath()
{
    int rc = 0; int i = 0;
    for(auto &k: path)
    {
        rc += k*pow(10,(path.size() - i - 1));
        i++;
    }
    return rc;
}

CGuiTree::~CGuiTree()
{
}
