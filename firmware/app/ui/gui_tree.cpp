#include "gui_tree.h"

void CGuiTree::init()
{
    MainWindow.setName((const char*)"Сажень-Н");
    MainWindow.setType(mainWindow);
    // 0 - 4
    main.setName(mainMenu[0]); call.setName(mainMenu[1]); recv.setName(mainMenu[2]); data.setName(mainMenu[3]); settings.setName(mainMenu[4]);
    // 1.1 - 1.4
    condCmd.setName(callSubMenu[0]); sms.setName(callSubMenu[1]); post.setName(callSubMenu[2]); groupCondCommand.setName(callSubMenu[3]);
    // 1.2.1 - 1.2.2
    condCmdSimpl.setName(commandsSubMenu[0]); condCmdDupl.setName(commandsSubMenu[1]);
    // 1.2.1.1 - 1.2.1.2
    condCmdSimplGroupCall.setName(smplSubMenu[0]); condCmdSimplIndivCall.setName(smplSubMenu[1]);
    // 1.5.1 - 1.5.2
    groupCondCommandSimpl.setName(groupCommandsSubMenu[0]); groupCondCommandDupl.setName(groupCommandsSubMenu[1]);
    // 1.5.1.1 - 1.5.1.2
    groupCondCommandSimplGroupCall.setName(groupCommandsSimplSubMenu[0]); groupCondCommandSimplIndivCall.setName(groupCommandsSimplSubMenu[1]);
    // 2.1 - 2.3
    recvTlf.setName(reciveSubMenu[0]); recvSms.setName(reciveSubMenu[1]); recvGroupCondCommsnds.setName(reciveSubMenu[2]);
    // 3.1 - 3.3
    dataRecv.setName(dataSubMenu[0]), dataSend.setName(dataSubMenu[1]);
    // 3.1.1 - 3.1.4
    dataRecvCondCmd.setName(dataSubSubMenu[0]); dataRecvSms.setName(dataSubSubMenu[1]); dataRecvPost.setName(dataSubSubMenu[2]); dataRecvGroupCondCmd.setName(dataSubSubMenu[3]);
    // 3.2.1 - 3.2.4
    dataSendCondCmd.setName(dataSubSubMenu[0]); dataSendSms.setName(dataSubSubMenu[1]); dataSendPost.setName(dataSubSubMenu[2]); dataSendGroupCondCmd.setName(dataSubSubMenu[3]);
    // 4.1 - 4.2
    sttDateTime.setName(settingsSubMenu[0]); sttConnParam.setName(settingsSubMenu[1]); sttScan.setName(settingsSubMenu[2]); swAruArm.setName(settingsSubMenu[3]); sttSound.setName(settingsSubMenu[4]); sttSuppress.setName(settingsSubMenu[5]);
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
    // 1 - Вызов
    call.prevState = &main;
    call.nextState.push_back(&condCmd);
    call.nextState.push_back(&sms);
    call.nextState.push_back(&post);
    call.nextState.push_back(&groupCondCommand);
    // 1.1 - Условные команды
    condCmd.prevState = &call;
    condCmd.nextState.push_back(&condCmdSimpl);
    condCmd.nextState.push_back(&condCmdDupl);
    // 1.1.1 - Одност.связь
    condCmdSimpl.prevState = &condCmd;
    condCmdSimpl.nextState.push_back(&condCmdSimplGroupCall);
    condCmdSimpl.nextState.push_back(&condCmdSimplIndivCall);
    // 1.1.1.1 - Групповой вызов
    condCmdSimplGroupCall.subType = GuiWindowsSubType::call;
    condCmdSimplGroupCall.prevState = &condCmdSimpl;
    condCmdSimplGroupCall.nextState.clear();
    condCmdSimplGroupCall.listItem.push_back(&condCmdSimplGroupCallParameters );
    // 1.1.1.2 - Индивидуальный
    condCmdSimplIndivCall.subType = GuiWindowsSubType::call;
    condCmdSimplIndivCall.prevState = &condCmdSimpl;
    condCmdSimplIndivCall.nextState.clear();
    condCmdSimplIndivCall.listItem.push_back( &condCmdSimplIndivCallParameters1 );
    condCmdSimplIndivCall.listItem.push_back( &condCmdSimplIndivCallParameters2 );
    // 1.1.2 - Двухст.связь | только индивидуально
    condCmdDupl.prevState = &condCmd;
    condCmdDupl.nextState.clear();
    // 1.2 - SMS
    sms.prevState = &call;
    sms.nextState.clear();
    // 1.3 - Голосовая почта
    post.prevState = &call;
    post.nextState.clear();
    // 1.4 - Группа условных команд
    groupCondCommand.prevState = &call;
    groupCondCommand.nextState.push_back(&groupCondCommandSimpl);
    groupCondCommand.nextState.push_back(&groupCondCommandDupl);
    // 1.4.1 - Односторонняя связь
    groupCondCommandSimpl.prevState = &groupCondCommand;
    groupCondCommandSimpl.nextState.push_back(&groupCondCommandSimplGroupCall);
    groupCondCommandSimpl.nextState.push_back(&groupCondCommandSimplIndivCall);
    // 1.4.1.1 - Групповой вызов
    groupCondCommandSimplGroupCall.prevState = &groupCondCommandSimpl;
    groupCondCommandSimplGroupCall.nextState.clear();
    // 1.4.1.2 - Индивидуальный
    groupCondCommandSimplIndivCall.prevState = &groupCondCommandSimpl;
    groupCondCommandSimplIndivCall.nextState.clear();
    // 1.4.2 - двухсторонняя связь
    groupCondCommandDupl.prevState = &groupCondCommand;
    groupCondCommandDupl.nextState.clear();
    // 2 - Прием
    recv.prevState = &main;
    recv.nextState.push_back(&recvTlf);
    recv.nextState.push_back(&recvSms);
    recv.nextState.push_back(&recvGroupCondCommsnds);
    // 2.3 - Речь
    recvTlf.prevState = &recv;
    recvTlf.nextState.clear();
    // 2.5 - Сообщение (АУС)
    recvSms.prevState = &recv;
    recvSms.nextState.clear();
    // 2.7 - Группа условных команд
    recvGroupCondCommsnds.prevState = &recv;
    recvGroupCondCommsnds.nextState.clear();
    // 3 - Данные
    data.prevState = &main;
    data.nextState.push_back(&dataRecv);
    data.nextState.push_back(&dataSend);
    // 3.1 - Принятые
    dataRecv.prevState = &data;
    dataRecv.nextState.push_back(&dataRecvCondCmd);
    dataRecv.nextState.push_back(&dataRecvSms);
    dataRecv.nextState.push_back(&dataRecvPost);
    dataRecv.nextState.push_back(&dataRecvGroupCondCmd);
    // 3.1.1 - Условные команды
    dataRecvCondCmd.prevState = &dataRecv;
    dataRecvCondCmd.nextState.clear();
    // 3.1.2 - SMS
    dataRecvSms.prevState = &dataRecv;
    dataRecvSms.nextState.clear();
    // 3.1.3 - Голосовая почта
    dataRecvPost.prevState = &dataRecv;
    dataRecvPost.nextState.clear();
    // 3.1.4 - Группа условных команд
    dataRecvGroupCondCmd.prevState = &dataRecv;
    dataRecvGroupCondCmd.nextState.clear();
    // 3.2 - Отправленные
    dataSend.prevState = &data;
    dataSend.nextState.push_back(&dataSendCondCmd);
    dataSend.nextState.push_back(&dataSendSms);
    dataSend.nextState.push_back(&dataSendPost);
    dataSend.nextState.push_back(&dataSendGroupCondCmd);
    // 3.2.1 - Условные команды
    dataSendCondCmd.prevState = &dataSend;
    dataSendCondCmd.nextState.clear();
    // 3.2.2 - SMS
    dataSendSms.prevState = &dataSend;
    dataSendSms.nextState.clear();
    // 3.2.3 - Голосовая почта
    dataSendPost.prevState = &dataSend;
    dataSendPost.nextState.clear();
    // 3.2.4 - Группа условных команд
    dataSendGroupCondCmd.prevState = &dataSend;
    dataSendGroupCondCmd.nextState.clear();
    // 4 - Настройки
    settings.prevState = &main;
    settings.nextState.push_back(&sttDateTime);
    settings.nextState.push_back(&sttConnParam);
    settings.nextState.push_back(&sttScan);
    settings.nextState.push_back(&swAruArm);
    settings.nextState.push_back(&sttSound);
    settings.nextState.push_back(&sttSuppress);
    // 4.1 - Дата/время
    sttDateTime.prevState = &settings;
    sttDateTime.nextState.push_back(&sttConnParamGPS);
    sttDateTime.nextState.push_back(&sttConnParamHand);
    // 4.1.1 - GPS синх-ция
    sttConnParamGPS.prevState = &sttDateTime;
    sttConnParamGPS.nextState.clear();
    // 4.1.2 - Ручная установка
    sttConnParamHand.prevState = &sttDateTime;
    sttConnParamHand.nextState.push_back(&sttSetDate);
    sttConnParamHand.nextState.push_back(&sttSetTime);
    // 4.1.2.1 - Установить дату
    sttSetDate.prevState = &sttConnParamHand;
    sttSetDate.nextState.clear();
    // 4.1.2.2 - Установить время
    sttSetTime.prevState = &sttConnParamHand;
    sttSetTime.nextState.clear();
    // 4.2 - Параметры связи
    sttConnParam.prevState = &settings;
    sttConnParam.nextState.push_back(&sttSetFreq);
    sttConnParam.nextState.push_back(&sttSetSpeed);
    // 4.2.1 - Частота
    sttSetFreq.prevState = &sttConnParam;
    sttSetFreq.nextState.clear();
    // 4.2.2 - Скорость
    sttSetSpeed.prevState = &sttConnParam;
    sttSetSpeed.nextState.clear();
    // 4.3 - Сканирование
    sttScan.subType = GuiWindowsSubType::twoState;
    sttScan.prevState = &settings;
    sttScan.nextState.clear();
    // 4.4 - АРУ / АРМ
    swAruArm.subType = GuiWindowsSubType::twoState;
    swAruArm.prevState = &settings;
    swAruArm.nextState.clear();
    // 4.5 - Громкость
    sttSound.subType = GuiWindowsSubType::volume;
    sttSound.prevState = &settings;
    sttSound.nextState.clear();
    // 4.6 - Шумоподавитель
    sttSuppress.subType = GuiWindowsSubType::twoState;
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

void CGuiTree::append(GuiWindowTypes type, char* p)
{
    statesStack.push_back(&*(new CState(type, p)));
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
