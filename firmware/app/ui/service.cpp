#include <stdlib.h>
#include "qm.h"
#define QMDEBUGDOMAIN	service
#include "qmdebug.h"
#include "dialogs.h"
#include "service.h"
#include "texts.h"
#include <thread>
#include <navigation/navigator.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>

MoonsGeometry ui_common_dialog_area = { 0,24,GDISPW-1,GDISPH-1 };
MoonsGeometry ui_msg_box_area       = { 20,29,GDISPW-21,GDISPH-11 };
MoonsGeometry ui_menu_msg_box_area  = { 1,1,GDISPW-2,GDISPH-2 };
MoonsGeometry ui_indicator_area     = { 0,0,GDISPW-1,23 };



namespace Ui {

bool Service::single_instance = false;

Service::Service( matrix_keyboard_t                  matrixkb_desc,
                  aux_keyboard_t                     auxkb_desc,
                  Headset::Controller               *headset_controller,
                  Multiradio::MainServiceInterface  *mr_main_service,
                  Multiradio::VoiceServiceInterface *mr_voice_service,
                  Power::Battery                    *power_battery,
                  Navigation::Navigator             *navigator,
                  DataStorage::FS                   *fs
)
{
    QM_ASSERT(single_instance == false);
    single_instance = true;
    this->navigator = navigator;
    this->matrix_kb          = matrixkb_desc;
    this->aux_kb             = auxkb_desc;
    this->multiradio_service = mr_main_service;
    this->voice_service      = mr_voice_service;
    this->power_battery      = power_battery;
    this->headset_controller = headset_controller;
    this->storageFs          = fs;

    ginit();
    voice_service->currentChannelChanged.connect(sigc::mem_fun(this, &Service::voiceChannelChanged));

    keyboard = new QmMatrixKeyboard(matrix_kb.resource);
    keyboard->keyAction.connect(sigc::mem_fun(this, &Service::keyHandler));
    chnext_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChNext]);
    chprev_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChPrev]);
    chnext_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chNextHandler));
    chprev_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chPrevHandler));

    main_scr  = new GUI_Dialog_MainScr(&ui_common_dialog_area);
    indicator = new GUI_Indicator     (&ui_indicator_area);

    menu = nullptr;
    msg_box = nullptr;

	if( menu == nullptr )
	{
		menu = new CGuiMenu(&ui_menu_msg_box_area, mainMenu[0], {alignHCenter,alignTop});
	}

    this->headset_controller->statusChanged.connect(sigc::mem_fun(this, &Service::updateBattery));
    this->multiradio_service->statusChanged.connect(sigc::mem_fun(this, &Service::updateMultiradio));
    this->power_battery->chargeLevelChanged.connect(sigc::mem_fun(this, &Service::updateBattery));

    this->multiradio_service->aleStateChanged.connect(sigc::mem_fun(this, &Service::updateAleState));
    this->multiradio_service->aleVmProgressUpdated.connect(sigc::mem_fun(this, &Service::updateAleVmProgress));
    //this->headset_controller->statusChanged.connect(sigc::mem_fun(this, &Service::));
    this->headset_controller->smartHSStateChanged.connect(sigc::mem_fun(this, &Service::updateHSState));

    guiTree.append(messangeWindow, (char*)test_Pass, (char*)error_SWF/*voice_service->ReturnSwfStatus()*/);
    msgBox( guiTree.getCurrentState().getName(), guiTree.getCurrentState().getText() );

//    guc_command_vector.push_back(2);
//    guc_command_vector.push_back(15);
//    guc_command_vector.push_back(54);
//    msgBox( guiTree.getCurrentState().getName(), guc_command_vector.at(position), guc_command_vector.size(), position );
//    guiTree.append(messangeWindow, (char*)test_Pass, voice_service->ReturnSwfStatus());
    command_rx_30 = 0;

    voice_service->firstPacket.connect(sigc::mem_fun(this,&Service::FirstPacketPSWFRecieved));
    voice_service->smsMess.connect(sigc::mem_fun(this,&Service::smsMessage));
    voice_service->smsFailed.connect(sigc::mem_fun(this,&Service::FailedSms));
    voice_service->respGuc.connect(sigc::mem_fun(this,&Service::gucFrame));

#ifndef PORT__PCSIMULATOR
    systemTimeTimer = new QmTimer(true); //TODO:
    systemTimeTimer->setInterval(1000);
    systemTimeTimer->start();
    systemTimeTimer->timeout.connect(sigc::mem_fun(this, &Service::updateSystemTime));
#endif


}

void Service::updateHeadset(Headset::Controller::Status status)
{
    bool open_ch_missing;
    Headset::Controller::SmartStatusDescription smart_status;

    switch(status)
    {
    case Headset::Controller::StatusAnalog:
        if(this->pGetHeadsetController()->getAnalogStatus(open_ch_missing))
        {
            if(open_ch_missing)
            {
                this->setNotification(Ui::NotificationMissingOpenVoiceChannels);
            }
        }
        break;
    case Headset::Controller::StatusSmartOk:
        if(this->pGetHeadsetController()->getSmartStatus(smart_status))
        {
            if(smart_status.channels_mismatch)
            {
                this->setNotification(Ui::NotificationMismatchVoiceChannelsTable);
            }
        }
        break;
    default:
        break;
    }

    indicator->UpdateHeadset(status);
    drawIndicator();
}

void Service::updateMultiradio(Multiradio::MainServiceInterface::Status status)
{
    indicator->UpdateMultiradio(status);
    drawIndicator();
}

void Service::updateBattery(int new_val)
{
    indicator->UpdateBattery(new_val);
    drawIndicator();
}

void Service::drawIndicator()
{
    if ( guiTree.getCurrentState().getType() == mainWindow )
        indicator->Draw();
}

void Service::FailedSms(int stage)
{
    switch(stage)
    {
    case -1:
    {
    	guiTree.append(messangeWindow, "Sucsess Sms", "Sms Sucsess");
    	msgBox( "Recieved packet ", "Sms Sucsess" );
    	break;
    }
    case 0:
    {
        guiTree.append(messangeWindow, "Failed Sms", sms_quit_fail1);
        msgBox( "Recieved packet ", sms_quit_fail1 );
        break;
    }
    case 1:
    {
        guiTree.append(messangeWindow, "Failed Sms", sms_quit_fail2);
        msgBox( "Recieved packet ", sms_quit_fail2);
        break;
    }
    case 3:
    {
        guiTree.append(messangeWindow, "Failed Sms", sms_quit_fail2);
        msgBox( "Failed Sms", sms_crc_fail);
        break;
    }
    default:
        //qmDebugMessage(QmDebug::Dump, "Sms error message: stage = %s", stage);
        break;
    }
}

Service::~Service() {
    QM_ASSERT(single_instance == true);
    single_instance = false;

    delete menu;
    delete msg_box;
    delete keyboard;
    delete chnext_bt;
    delete chprev_bt;
    delete chnext_bt;
    delete chprev_bt;
    delete main_scr;
    delete indicator;
}

void Service::setNotification(NotificationType type)
{
    switch(type)
    {
    case NotificationMissingVoiceChannelsTable:
        guiTree.append(messangeWindow, missing_ch_table_txt[getLanguage()]);
        break;
    case NotificationMissingOpenVoiceChannels:
        guiTree.append(messangeWindow, missing_open_ch_txt[getLanguage()]);
        break;
    case NotificationMismatchVoiceChannelsTable:
        guiTree.append(messangeWindow, ch_table_mismatch_txt[getLanguage()]);
        break;
    default:
        QM_ASSERT(0);
        break;
    }
    draw();
}


void Service::keyHandler(int key_id, QmMatrixKeyboard::PressType pr_type){
    QM_UNUSED(pr_type);
    switch(matrix_kb.key_id[key_id]){
    case matrixkbkeyEnter:
        keyPressed(keyEnter);
        break;
    case matrixkbkeyBack:
        keyPressed(keyBack);
        break;
    case matrixkbkeyUp:
        keyPressed(keyUp);
        break;
    case matrixkbkeyDown:
        keyPressed(keyDown);
        break;
    case matrixkbkeyLeft:
        keyPressed(keyLeft);
        break;
    case matrixkbkeyRight:
        keyPressed(keyRight);
        break;
    case matrixkbkey0:
        keyPressed(key0);
        break;
    case matrixkbkey1:
        keyPressed(key1);
        break;
    case matrixkbkey2:
        keyPressed(key2);
        break;
    case matrixkbkey3:
        keyPressed(key3);
        break;
    case matrixkbkey4:
        keyPressed(key4);
        break;
    case matrixkbkey5:
        keyPressed(key5);
        break;
    case matrixkbkey6:
        keyPressed(key6);
        break;
    case matrixkbkey7:
        keyPressed(key7);
        break;
    case matrixkbkey8:
        keyPressed(key8);
        break;
    case matrixkbkey9:
        keyPressed(key9);
        break;
    default:
        break;
    }
}

Headset::Controller * Service::pGetHeadsetController(){
    return headset_controller;
}

Multiradio::VoiceServiceInterface* Service::pGetVoiceService()
{
    return voice_service;
}

Multiradio::MainServiceInterface* Service::pGetMultitradioService()
{
    return multiradio_service;
}

Power::Battery * Service::pGetPowerBattery()
{
    return power_battery;
}

void Service::chNextHandler()
{
    if(chnext_bt->isPressed())
    {
        keyPressed(keyChNext);
    }
}

void Service::chPrevHandler()
{
    if(chprev_bt->isPressed())
    {
        keyPressed(keyChPrev);
    }
}

void Service::voiceChannelChanged()
{
    CState state = guiTree.getCurrentState();
    if ( state.getType() == mainWindow)
        drawMainWindow();
}

void Service::keyPressed(UI_Key key)
{
    CState state = guiTree.getCurrentState();

    switch( state.getType() )
    {
    case mainWindow:
    {
        if (main_scr->isEditing())
        {
            switch(key)
            {
            case keyBack:
                main_scr->mwFocus = -2;
                main_scr->setFocus(1-main_scr->mwFocus);
                main_scr->editing = false;
                main_scr->setFreq(main_scr->oFreq.c_str());
                break;
            case keyEnter:
                if (main_scr->mwFocus == 0)
                {
                    main_scr->oFreq.clear();
                    main_scr->oFreq.append(main_scr->nFreq.c_str());
                    int freq = atoi(main_scr->nFreq.c_str());
                    voice_service->TuneFrequency(freq);
                }
                // ? пїЅпїЅпїЅ
                switch ( main_scr->mainWindowModeId )
                {
                case 0:
                {}
                    // пїЅпїЅпїЅ
                case 1:
                {}
                    // пїЅпїЅпїЅ
                case 2:
                {}
                default:
                    break;
                }

                break;
            case keyLeft:
                if (main_scr->mwFocus == 1 && main_scr->mainWindowModeId > 0)
                    main_scr->mainWindowModeId--;
                break;
            case keyRight:
                if (main_scr->mwFocus == 1 && main_scr->mainWindowModeId < 2)
                    main_scr->mainWindowModeId++;
                break;
            default:
                if ( main_scr->mwFocus == 0 )
                    main_scr->editingFreq(key);
                break;
            }
        }
        else
        {
            switch(key)
            {
            case keyChNext:
                pGetVoiceService()->tuneNextChannel();
                break;
            case keyChPrev:
                pGetVoiceService()->tunePreviousChannel();
                break;
            case keyBack:
                main_scr->mwFocus = -2;
                main_scr->setFocus(1-main_scr->mwFocus);
                break;
            case keyUp:
                if (main_scr->mwFocus < 1)
                    main_scr->mwFocus++;
                main_scr->setFocus(1-main_scr->mwFocus);
                break;
            case keyDown:
                if (main_scr->mwFocus < 0)
                    main_scr->mwFocus++;
                if (main_scr->mwFocus > 0)
                    main_scr->mwFocus--;
                main_scr->setFocus(1-main_scr->mwFocus);
                break;
            case keyEnter:
                if (main_scr->mwFocus < 0)
                    guiTree.advance(0);
                if (main_scr->mwFocus >= 0)
                {
                    main_scr->editing = true;
                    if (main_scr->mwFocus == 0)
                        main_scr->nFreq.clear();
                }
                break;
                //            case keyLeft:
                //                if (main_scr->mwFocus == 1 && main_scr->mainWindowModeId > 0)
                //                    main_scr->mainWindowModeId--;
                //                break;
                //            case keyRight:
                //                if (main_scr->mwFocus == 1 && main_scr->mainWindowModeId < 2)
                //                    main_scr->mainWindowModeId++;
                //                break;
            case key0:
            {
                int p = 10;
                char sym[64];
                sprintf(sym,"%d",p);
                guiTree.append(messangeWindow, (char*)"Receive first packet", sym);
            }
                break;
            default:
                break;
            }
        }
        break;
    }
    case messangeWindow:
    {
        if ( key == keyEnter)
        {
            guiTree.delLastElement();
            if (msg_box != nullptr)
            {
                delete msg_box;
                msg_box = nullptr;
            }

            vect = nullptr;
            position = 1;
        }
        else
        {
            if (vect != nullptr)
            {
                if (key == keyUp && position > 1)
                { position--; }
                if (key == keyDown && position < vect[0])
                { position++; }
                //msg_box->setCmd(msg_box_vector.at(position));
            }
            else
                msg_box->keyPressed(key);
        }
        break;
    }
        // ? пїЅ ? пїЅ? пїЅ? пїЅСЋ
    case menuWindow:
    {
        if ( key == keyEnter)
        {
            guiTree.advance(menu->focus);
            menu->focus = 0;
        }
        if ( key == keyBack)
        {
            guiTree.backvard();
            menu->focus = 0;
        }
        if (key == keyUp)
        {
            if ( menu->focus > 0 )
                menu->focus--;
        }
        if (key == keyDown)
        {
            if ( state.nextState.size() != 0 )
            {
                if ( menu->focus < state.nextState.size()-1 )
                    menu->focus++;
            }
        }
        menu->keyPressed(key);
        break;
    }
    case endMenuWindow:
    {
        CEndState estate = (CEndState&)guiTree.getCurrentState();

        switch(estate.subType)
        {
        case GuiWindowsSubType::simpleCondComm:
        {
            //[0] - CMD, [1] - R_ADDR, [2] - retrans
            switch (menu->txCondCmdStage)
            {
            case 1:
            { // � ��������������/ ��� �������������
                if (key == keyUp || key == keyDown)
                { menu->useCmdRetrans = menu->useCmdRetrans ? false : true; }
                break;
            }
            case 2:
            { // ���� ������ ����������
                if ( key > 5 && key < 16)
                {
                    auto iter = estate.listItem.begin();
                    (*iter)++;(*iter)++;
                    if ( (*iter)->inputStr.size() < 2 )
                    {
                        (*iter)->inputStr.push_back((char)(42+key));
                        // check
                        int rc = atoi((*iter)->inputStr.c_str());
                        if ( rc > 31 )
                        { (*iter)->inputStr.clear(); }
                    }
                }
                break;
            }
            case 3:
            { // ���� ������ �������������
                if ( key > 5 && key < 16)
                {
                    auto iter = estate.listItem.begin();
                    (*iter)++;
                    if ( (*iter)->inputStr.size() < 2 )
                    {
                        (*iter)->inputStr.push_back((char)(42+key));
                        // check
                        int rc = atoi((*iter)->inputStr.c_str());
                        if ( rc > 31 )
                        { (*iter)->inputStr.clear(); }
                    }
                }
                break;
            }
            case 4:
            { // ���� �������� �������
                if ( key > 5 && key < 16)
                {
                    auto iter = estate.listItem.begin();

                    if ( (*iter)->inputStr.size() < 2 )
                    {
                        (*iter)->inputStr.push_back((char)(42+key));
                        // check
                        int rc = atoi((*iter)->inputStr.c_str());
                        if ( rc > 99 )
                        { (*iter)->inputStr.clear(); }
                    }
                }
                break;
            }
            default:
            {break;}
            }

            switch (key)
            {
            case keyEnter:
            {
                int size = 5;

                // next field
                if (menu->txCondCmdStage <= size )
                {
                    if (menu->txCondCmdStage == 1 && estate.listItem.size() == 2)
                    {
                        menu->txCondCmdStage++;
                        if (menu->useCmdRetrans == false)
                            menu->txCondCmdStage++;
                    }
                    if (menu->txCondCmdStage == 2 && menu->useCmdRetrans == false)
                    {
                        menu->txCondCmdStage++;
                    }
                    menu->txCondCmdStage++;
                }

                // send
                if ( menu->txCondCmdStage > size )
                {
#ifndef _DEBUG_
                    // [0] - cmd, [1] - raddr, [2] - retrans
                    // bool menu->useRETRANS
					int param[3] = {0,0,0}, i = 0;
                    for(auto &k: estate.listItem)
                    {
                        param[i] = atoi(k->inputStr.c_str());
                        i++;
                    }
                    	if (estate.listItem.size() == 2){
                    		//voice_service->clearBuff();
                            voice_service->TurnPSWFMode(0, 0, param[0],0);
                    	}
                    	else if (estate.listItem.size() == 3)
                            voice_service->TurnPSWFMode(1, param[0], param[2],param[1]);

//                        for(auto &k: estate.listItem)
//                        {
//                            k->inputStr.clear();
//                        }
#else
                    menu->txCondCmdStage = 1;
                    guiTree.resetCurrentState();
                    for(auto &k: estate.listItem)
                        k->inputStr.clear();
#endif
                }
                break;
            }
            case keyBack:
            {
                auto iter = estate.listItem.begin();

                if (menu->txCondCmdStage == 2)
                {
                    // R_ADR
                    (*iter)++;(*iter)++;
                    if ((*iter)->inputStr.size() > 0)
                        (*iter)->inputStr.pop_back();
                    else
                        menu->txCondCmdStage--;
                }
                else if(menu->txCondCmdStage == 3)
                {
                    // Retrans
                    (*iter)++;
                    if ((*iter)->inputStr.size() > 0)
                        (*iter)->inputStr.pop_back();
                    else
                    {
                        menu->txCondCmdStage--;
                        if (estate.listItem.size() == 2)
                            menu->txCondCmdStage--;
                    }
                }
                else if(menu->txCondCmdStage == 4)
                {
                    // CMD
                    if ((*iter)->inputStr.size() > 0)
                        (*iter)->inputStr.pop_back();
                    else
                    {
                        menu->txCondCmdStage--;
                        if (!menu->useCmdRetrans)
                        {
                            menu->txCondCmdStage--;
                            if (estate.listItem.size() == 2)
                                menu->txCondCmdStage--;
                        }
                    }
                }
                else
                {
                    if (menu->txCondCmdStage > 1)
                        menu->txCondCmdStage--;
                    else
                    {
                        menu->txCondCmdStage = 1;
                        guiTree.backvard();
                        for(auto &k: estate.listItem)
                            k->inputStr.clear();
                    }
                }
            }
            default:
                break;
            }
            break;
        }
        case GuiWindowsSubType::txGroupCondCmd:
        {
            std::list<SInputItemParameters*>::iterator iter = estate.listItem.begin();

            if ( menu->groupCondCommStage == 0 )
            {
                if (menu->focus == 1)
                    (*iter)++;
            }
            else if ( menu->groupCondCommStage == 1 && estate.listItem.size() == 4 )
            {
                if ( menu->focus == 0 )
                {
                    (*iter)++;(*iter)++;
                }
                if ( menu->focus == 1 )
                {
                    (*iter)++;(*iter)++;(*iter)++;
                }
            }
            else if ( menu->groupCondCommStage == 1 && estate.listItem.size() == 3 )
            {
                if ( menu->focus == 0 )
                {
                    (*iter)++;(*iter)++;(*iter)++;
                }
            }

            switch ( key )
            {
            case keyBack:
            {
                if ( menu->groupCondCommStage == 0 && menu->focus < 2 )
                {
                    if ( (*iter)->inputStr.size() > 0 )
                    {
                        (*iter)->inputStr.pop_back();
                    }
                    else
                    {
                        for (auto &k: estate.listItem)
                            k->inputStr.clear();

                        guiTree.backvard();
                        menu->focus = 0;
                    }
                }
                else if ( menu->groupCondCommStage == 1 )
                {
                    if ( estate.listItem.size() == 4 && menu->focus < 2 )
                    {
                        if ( (*iter)->inputStr.size() > 0 )
                        {
                            (*iter)->inputStr.pop_back();
                        }
                        else
                        {
                            menu->groupCondCommStage = 0;
                            menu->focus = 0;
                        }
                    }
                    else if ( estate.listItem.size() == 3 && menu->focus < 2 )
                    {
                        if ( (*iter)->inputStr.size() > 0 )
                        {
                            (*iter)->inputStr.pop_back();
                        }
                        else
                        {
                            menu->groupCondCommStage = 0;
                            menu->focus = 0;
                        }
                    }
                    else
                    {
                        menu->groupCondCommStage = 0;
                        menu->focus = 0;
                    }
                }
                else
                {
                    if ( menu->groupCondCommStage == 1 )
                    {
                        menu->groupCondCommStage = 0;
                    }

                }
                break;
            }
            case keyEnter:
            {
                if ( menu->groupCondCommStage == 0 )
                {
                    if ( menu->focus == 2 )
                    {
                        menu->focus = 0;
                        menu->groupCondCommStage = 1;
                    }
                    break;
                }
                if ( menu->groupCondCommStage == 1 )
                {
                    if ( menu->focus == 2 )
                    {
#ifndef PORT__PCSIMULATOR
                        menu->focus = 0;
                        menu->groupCondCommStage = 0;
                        int mas[4];
                        int i = 0;
                        const char * str;
                        for (auto &k: estate.listItem)
                        {
                        	mas[i] = atoi(k->inputStr.c_str());
                        	if (i == 3) str = k->inputStr.c_str();
                        	i++;
                        }
                        int r_adr = 0;//atoi(mas[0]);
                        int speed = 0;//atoi(mas[1]);
                        guc_command_vector.clear();
                        parsingGucCommand((uint8_t*)str);
                        voice_service->TurnGuc(r_adr,speed,guc_command_vector);
#else
                        for (auto &k: estate.listItem)
                            k->inputStr.clear();
                        guiTree.resetCurrentState();
#endif
                    }
                }
                break;
            }
            case keyUp:
            {
                if ( menu->focus > 0 )
                    menu->focus--;
                break;
            }
            case keyDown:
            {
                if ( estate.listItem.size() == 3 && menu->groupCondCommStage == 1 ){
                    if (menu->focus < 1)
                        menu->focus++;

                }else
                    if ( menu->focus < 2 )
                        menu->focus++;
                break;
            }
            default:
            {
                // set freq
                if ( menu->groupCondCommStage == 0 && (menu->focus == 0 || menu->focus == 1) )
                {
                    std::string* freq;

                    if ( menu->focus == 0 )
                        freq = &(*iter)->inputStr;

                    if ( menu->focus == 1 )
                        freq = &(*iter)->inputStr;

                    if (freq->size() < 8)
                    {
                        if ( key > 5 && key < 17)
                        {
                            if ( freq->size() > 0 || (freq->size() == 0 && key != key0) )
                            {
                                freq->push_back( (char)(42 + key) );
// check
                                if ( atoi( freq->c_str() ) > 25E6 )
                                    freq->clear();
                            }
                        }
                    }
                }

                if ( menu->groupCondCommStage == 1 )
                {
                    if ( estate.listItem.size() == 3 )
                    {
                        // set commands
                        if ( menu->groupCondCommStage == 1 && menu->focus == 0 )
                        {
                            std::string* commands;
                            commands = &(*iter)->inputStr;

                            if ( key > 5 && key < 17 && commands->size() < 20 )
                            {
                                if ( key != key0 )
                                {
                                    commands->push_back( (char)(42 + key) );
                                }
                                else
                                    menu->inputGroupCondCmd(estate);
                            }
                        }
                    }
                    else if ( estate.listItem.size() == 4 )
                    {
                        // set address
                        if ( menu->focus == 0 )
                        {
                            std::string* address;
                            address = &(*iter)->inputStr;

                            if ( key > 5 && key < 17)
                            {
                                address->push_back( (char)(42 + key) );
                                if ( atoi(address->c_str()) > 31) address->clear();
                            }
                        }

                        // set commands
                        if ( menu->focus == 1 )
                        {
                            std::string* commands;
                            commands = &(*iter)->inputStr;

                            if ( key > 5 && key < 17 && commands->size() < 100 )
                            {
                                if ( key != key0 )
                                    commands->push_back( (char)(42 + key) );
                                else
                                    menu->inputGroupCondCmd(estate);
                            }
                        }
                    }
                }

                break;
            }
            }
            break;
        }
        case GuiWindowsSubType::txPutOffVoice:
        {
            switch(menu->putOffVoiceStatus)
            {
            case 1:
            {
                if ( key > 5 && key < 16 && menu->channalNum.size() < 2 )
                {
                    menu->channalNum.push_back((char)(42+key));
                    // check
                    int rc = atoi(menu->channalNum.c_str());

                    if ( (rc < 1 || rc > 98) && (menu->channalNum.size() > 1))
                    { menu->channalNum.clear(); }
                }
                if (key == keyBack)
                {
                    if (menu->channalNum.size() > 0)
                        menu->channalNum.pop_back();
                    else
                    {
                        guiTree.backvard();
                        menu->focus = 0;
                    }
                }
#ifdef _DEBUG_
                if (key == keyEnter)
                {
                    if (menu->channalNum.size() > 0)
                        menu->putOffVoiceStatus++;
                }
#else
                if (key == keyEnter && menu->channalNum.size() > 0)
                {
                    headset_controller->startSmartRecord((uint8_t)atoi( menu->channalNum.c_str()));
                    menu->putOffVoiceStatus++;
                }
#endif
                break;
            }
            case 2:
            {
                if (key == keyBack)
                {
                    headset_controller->stopSmartRecord();
                    menu->putOffVoiceStatus--;
                }
#ifndef _DEBUG_
                if (key == keyEnter) // && STATUS OK
                {
                    headset_controller->stopSmartRecord();

                    if ( headset_controller->getSmartHSState() == SmartHSState_SMART_READY )
                        menu->putOffVoiceStatus++;
                    else if ( headset_controller->getSmartHSState() == SmartHSState_SMART_RECORD_TIMEOUT ||\
                              headset_controller->getSmartHSState() == SmartHSState_SMART_EMPTY_MESSAGE  ||\
                              headset_controller->getSmartHSState() == SmartHSState_SMART_ERROR          ||\
                              )
                    {
                        multiradio_service->stopAle();
                        menu->putOffVoiceStatus = 1;
                        menu->voiceAddr.clear();
                        menu->channalNum.clear();
                        menu->focus = 0;
                        guiTree.resetCurrentState();
                    }
                    // repeat
                    else if (headset_controller->getSmartHSState() == SmartHSState_SMART_NOT_CONNECTED ||\
                             headset_controller->getSmartHSState() == SmartHSState_SMART_BAD_CHANNEL ||\
                             headset_controller->getSmartHSState() == SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL ||\
                             headset_controller->getSmartHSState() == SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE ||\
                             headset_controller->getSmartHSState() == SmartHSState_SMART_RECORD_DOWNLOADING ||\
                             headset_controller->getSmartHSState() == SmartHSState_SMART_PLAYING ||\
                             headset_controller->getSmartHSState() == SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL ||\
                             headset_controller->getSmartHSState() == SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE ||\
                             headset_controller->getSmartHSState() == SmartHSState_SMART_RECORDING ||\
                             headset_controller->getSmartHSState() == SmartHSState_SMART_RECORD_UPLOADING ||\
                             )
                    {}
                }
#else
                if (key == keyEnter)
                {
                    menu->putOffVoiceStatus++;
                }
#endif
                break;
            }
            case 3:
            {// ввод адреса олучатель
                if ( key > 5 && key < 16 && menu->voiceAddr.size() < 2 )
                {
                    menu->voiceAddr.push_back((char)(42+key));
                    // check
                    int rc = atoi(menu->voiceAddr.c_str());

                    if ( (rc < 1 || rc > 31) && (menu->voiceAddr.size() > 1) )
                    { menu->voiceAddr.clear(); }
                }
                if (key == keyBack)
                {
                    if (menu->voiceAddr.size() > 0)
                        menu->voiceAddr.pop_back();
                    else
                    {
                        menu->putOffVoiceStatus--;
                    }
                }

#ifdef _DEBUG_
                if (key == keyEnter)
                {
                    if (menu->voiceAddr.size() > 0)
                        menu->putOffVoiceStatus++;
                }
#else
                if (key == keyEnter)
                {
                    menu->putOffVoiceStatus++;
                }
#endif
                break;
            }
            case 4:
            {// подтверждение
                if (key == keyBack)
                {
                    menu->putOffVoiceStatus--;
                }
#ifdef _DEBUG_
                if (key == keyEnter)
                {
                    if (menu->voiceAddr.size() < 1)
                        menu->voiceAddr.append("23\0");
                    menu->putOffVoiceStatus++;
                }
#else
                if (key == keyEnter)
                {
                    multiradio_service->startAleTxVoiceMail((uint8_t)atoi(menu->voiceAddr.c_str()));
                    menu->putOffVoiceStatus++;
                }
#endif
                break;
            }
            case 5:
            {// статус
                if (key == keyBack)
                {
                    menu->putOffVoiceStatus--;
#ifndef _DEBUG_
                    multiradio_service->stopAle();
#endif
                }
                if (key == keyEnter /*&& multiradio_service->getAleState() == */)
                {
#ifndef _DEBUG_
                    multiradio_service->stopAle();
#endif
                    menu->putOffVoiceStatus = 1;
                    menu->voiceAddr.clear();
                    menu->channalNum.clear();
#ifndef _DEBUG_
                    menu->focus = 0;
                    guiTree.resetCurrentState();
#endif
                }
                break;
            }
            default:
            {
                break;
            }
            }
            break;
        }
        case GuiWindowsSubType::message:
        {
            switch (menu->smsTxStage)
            {
            case 1:
            {
                switch (key)
                {
                case keyBack:
                {
                    guiTree.backvard();
                    break;
                }
                case keyLeft:
                case keyRight:
                {
                    menu->useSmsRetrans = menu->useSmsRetrans ? false : true;
                    break;
                }
                case keyEnter:
                {
                    menu->smsTxStage++;
                    if (!menu->useSmsRetrans)
                        menu->smsTxStage++;
                    break;
                }
                default:{break;}
                }
                break;
            }
            case 2:
            {
                auto iter = estate.listItem.begin(); (*iter)++; (*iter)++;

                switch (key)
                {
                case keyBack:
                {

                    if ((*iter)->inputStr.size() > 0)
                        (*iter)->inputStr.pop_back();
                    else
                    menu->smsTxStage--;
                    break;
                }
                case keyEnter:
                {
                    if ((*iter)->inputStr.size() > 0)
                    menu->smsTxStage++;
                    break;
                }
                default:
                {
                    menu->inputSmsAddr( &(*iter)->inputStr, key );
                    break;
                }
                }
                break;
            }
            case 3:
            {
                auto iter = estate.listItem.begin();

                switch (key)
                {
                case keyBack:
                {
                    if ((*iter)->inputStr.size() > 0)
                        (*iter)->inputStr.pop_back();
                    else
                    {
                        menu->smsTxStage--;
                        if (!menu->useSmsRetrans)
                            menu->smsTxStage--;
                    }
                    break;
                }
                case keyEnter:
                {
                    if ((*iter)->inputStr.size() > 0)
                        menu->smsTxStage++;
                    break;
                }
                default:
                {
                    menu->inputSmsAddr( &(*iter)->inputStr, key );
                    break;
                }
                }
                break;
            }
            case 4:
            {
                auto iter = estate.listItem.begin(); (*iter)++;

                switch (key)
                {
                case keyBack:
                {
                    if ((*iter)->inputStr.size() > 0)
                        (*iter)->inputStr.pop_back();
                    else
                        menu->smsTxStage--;
                    break;
                }
                case keyEnter:
                {
                    if ((*iter)->inputStr.size() > 0)
                        menu->smsTxStage++;
                    break;
                }
                default:
                {
                    if ((*iter)->inputStr.size() < 100)
                        menu->inputSmsMessage( &(*iter)->inputStr, key );
                    break;
                }
                }
                break;
            }
            case 5:
            {
                switch (key)
                {
                case keyBack:
                {
                    menu->smsTxStage--;
                    break;
                }
                case keyEnter:
                {
                    // call
                    // [0] - dstAddr, [1]- message, [3] - retrAddr
                    auto iter = estate.listItem.begin();
                    auto dstAddr = (*iter)->inputStr;
                    (*iter)++;
                    auto msg = (*iter)->inputStr;
                    (*iter)++; (*iter)++;
                    auto retrAddr = (*iter)->inputStr;

                    if (navigator != 0){
                        Navigation::Coord_Date date = navigator->getCoordDate();

                        char ch[4]; memcpy(ch, date.data, 4);

                        if (atoi(ch) > 0)
                        {
                            if (atoi(retrAddr.c_str()) > 0)
                                voice_service->TurnSMSMode(atoi(dstAddr.c_str()), (char*)msg.c_str());
                            else
                                voice_service->TurnSMSMode(atoi(dstAddr.c_str()), (char*)msg.c_str());

                            for(auto &k: estate.listItem)
                                k->inputStr.clear();
                        }
                    }

                    menu->smsTxStage = 1;
                    guiTree.resetCurrentState();

                    break;
                }
                default:{break;}
                }
                break;
            }
            default:
            {break;}
            }

            break;
        }
        case GuiWindowsSubType::recvVoice:
        {
            switch ( key )
            {
            case keyBack:
            {
                guiTree.backvard();
                menu->focus = 0;
                break;
            }
            case keyEnter:
            {break;}
            default:
                break;
            }
            break;
        }
        case GuiWindowsSubType::recvCondCmd:
        {
            if ( menu->rxCondCmdStatus == 1 && (key == keyUp || key == keyDown))
            {
                menu->useTicket = menu->useTicket ? false : true;
            }

            if ( key == keyBack)
            {
                if (menu->rxCondCmdStatus == 2)
                    menu->rxCondCmdStatus--;
                else
                    guiTree.backvard();
            }
            if ( key == keyEnter)
            {
                if (menu->rxCondCmdStatus == 1)
                {
                    menu->rxCondCmdStatus++;
                }
                else if( menu->rxCondCmdStatus == 2 )
                {
#ifdef _DEBUG_
                    guiTree.resetCurrentState();
#else
                    if (menu->useTicket)
                        voice_service->TurnPSWFMode(0,0,0,0); // 1 param - request /no request
                    else
                        voice_service->TurnPSWFMode(0,0,0,0);
#endif
                    menu->rxCondCmdStatus = 1;
                }
                else
                {}
            }
            break;
        }
        case GuiWindowsSubType::recvSms:
        {
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->focus = 0;
            }
            if ( key == keyEnter)
            {
                if (menu->recvStage == 0)
                {
                    menu->recvStage = 1;
#ifndef PORT__PCSIMULATOR
                voice_service->TurnSMSMode();
#endif
                break;
                }
                if (menu->recvStage == 1)
                {
                    menu->recvStage = 0;
                    guiTree.resetCurrentState();
                }

            }
            break;
        }
        case GuiWindowsSubType::recvGroupCondCmd:
        {
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->focus = 0;
            }
            if ( key == keyEnter)
            {
#ifndef PORT__PCSIMULATOR
            	voice_service->TurnGuc();
               // voice_service->TurnPSWFMode(0,0,0);
#else
                guiTree.resetCurrentState();
#endif
            }
            break;
        }
        case GuiWindowsSubType::rxPutOffVoice:
        {
            switch(menu->putOffVoiceStatus)
            {
            case 1:
            {
                if (key == keyBack)
                {
                    menu->focus = 0;
                    guiTree.backvard();
#ifndef _DEBUG_
                    multiradio_service->stopAle();
#endif
                }
                if (key == keyEnter)
                {
#ifndef _DEBUG_
                    multiradio_service->startAleRx();
#endif
                    menu->putOffVoiceStatus++;
                }
                break;
            }
            case 2:
            {
                if (key == keyBack)
                {
#ifndef _DEBUG_
                    multiradio_service->stopAle();
#endif
                    menu->putOffVoiceStatus--;
                }
                if (key == keyEnter)
                {
                    // multiradio_service->getStatus();
                    menu->putOffVoiceStatus++;
                }
                break;
            }
            case 3:
            {
                if (key == keyBack)
                {
                    menu->putOffVoiceStatus--;
                }
                if (key == keyEnter)
                {
                    menu->putOffVoiceStatus++;
                }
                break;
            }
            case 4:
            {
                // выбрать канал воспроизведения
                if ( key > 5 && key < 16 && menu->channalNum.size() < 2 )
                {
                    menu->channalNum.push_back((char)(42+key));
                    // check
                    int rc = atoi(menu->channalNum.c_str());

                    if ( (rc < 1 || rc > 98) && (menu->channalNum.size() > 1) )
                    { menu->channalNum.clear(); }
                }
                if (key == keyBack)
                {
                    if (menu->channalNum.size() > 0)
                        menu->channalNum.pop_back();
                    else
                        menu->putOffVoiceStatus--;
                }
                if (key == keyEnter)
                {
                    if (menu->channalNum.size()>0)
                    {
#ifndef _DEBUG_
                        headset_controller->startSmartPlay((uint8_t)atoi(menu->channalNum.c_str()));
#endif
                        menu->putOffVoiceStatus++;
                    }
                }
                break;
            }
            case 5:
            {
                if (key == keyBack)
                {
#ifndef _DEBUG_
                    headset_controller->stopSmartPlay();
#endif
                    menu->putOffVoiceStatus--;
                }
                if (key == keyEnter)
                {
#ifndef _DEBUG_
                    headset_controller->stopSmartPlay();
#endif
                    menu->putOffVoiceStatus = 1;
                    menu->voiceAddr.clear();
                    menu->channalNum.clear();
#ifndef _DEBUG_
                    menu->focus = 0;
                    guiTree.resetCurrentState();
#endif

                }
                break;
            }
            default:
            {break;}
            }
            break;
        }
        case GuiWindowsSubType::volume:
        {
            if ( key == keyRight || key == keyUp )
            {
                menu->incrVolume();
                uint8_t level = menu->getVolume();
                voice_service->TuneAudioLevel(level);

            }
            if ( key == keyLeft || key == keyDown )
            {
                menu->decrVolume();
                uint8_t level = menu->getVolume();
                voice_service->TuneAudioLevel(level);
            }
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->focus = 0;
            }
            break;
        }
        case GuiWindowsSubType::scan:
        {
            if ( key == keyRight || key == keyUp )
            {
                menu->scanStatus = menu->scanStatus ? false : true;
                menu->inclStatus = menu->inclStatus ? false : true;
            }
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->focus = 0;
            }
            break;
        }
        case GuiWindowsSubType::suppress:
        {
            if ( key == keyRight || key == keyLeft )
            {
                menu->supressStatus = menu->supressStatus ? false : true;
                menu->inclStatus = menu->inclStatus ? false : true;
            }
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->focus = 0;
            }
            if (key == keyEnter)
            {
                int value = 0;
                if (menu->supressStatus == 1) value =  6;
                voice_service->tuneSquelch(value);
            }
            break;
        }
        case GuiWindowsSubType::aruarmaus:
        {
            if (key == keyUp  )
            {
                if ( menu->focus > 0 )
                    menu->focus--;
            }
            if (key == keyDown)
            {
                if ( menu->focus < 2 )
                    menu->focus++;
            }
            if ( key == keyLeft || key == keyRight )
            {
                if ( menu->focus >= 0 && menu->focus < 3)
                {
                    menu->aruArmAsuStatus[menu->focus] = menu->aruArmAsuStatus[menu->focus] ? false : true;
                }              
#ifndef PORT__PCSIMULATOR
                uint8_t vol = menu->getAruArmAsu();
                voice_service->TurnAGCMode(vol, menu->focus);
#endif
            }

            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->focus = 0;
            }
            break;
        }
        case GuiWindowsSubType::gpsCoord:
        {
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->focus = 0;
            }
            else
            {
#if !defined(PORT__PCSIMULATOR)
            setCoordDate(navigator->getCoordDate());
#endif
            }
            break;
        }
        case GuiWindowsSubType::gpsSync:
        {
            switch ( key )
            {
            case keyBack:
            {
                guiTree.backvard();
                menu->focus = 0;
                break;
            }
            case keyRight:
            case keyLeft:
            {
                gpsSynchronization = gpsSynchronization ? false : true;
                break;
            }
            default:
                break;
            }
            break;
        }
        case GuiWindowsSubType::setDate:
        {
            if ( key == keyBack )
            {
                auto &st = ((CEndState&)guiTree.getCurrentState()).listItem.front()->inputStr;
                if (st.size() > 0)
                {
                    st.pop_back();
                }
                else
                {
                    guiTree.backvard();
                    menu->focus = 0;
                }
            }
            else if ( key >= key0 && key <= key9 )
            {
                auto &st = ((CEndState&)guiTree.getCurrentState()).listItem.front()->inputStr;
                if ( st.size() < 8 )
                    st.push_back(key+42);

                if (st.size() > 1 && st.size() < 3 )
                {
                    // 1 <= ?? <= 31
                }
                if (st.size() > 3 && st.size() < 5 )
                {
                    // 1 <= ?? <= 12
                }
                if (st.size() > 6 )
                {
                    // 16 <= ?? <= 99
                }

                if ( st.size() == 2  || st.size() == 5)
                {
                    st.push_back('.');
                }
            }
            break;
        }
        case GuiWindowsSubType::setTime:
        {
            if ( key == keyBack )
            {
                auto &st = ((CEndState&)guiTree.getCurrentState()).listItem.front()->inputStr;
                if (st.size() > 0)
                {
                    st.pop_back();
                }
                else
                {
                    guiTree.backvard();
                    menu->focus = 0;
                }
            }
            else if ( key >= key0 && key <= key9 )
            {
                auto &st = ((CEndState&)guiTree.getCurrentState()).listItem.front()->inputStr;
                if ( st.size() < 8 )
                    st.push_back(key+42);

                if (st.size() > 1 && st.size() < 3 )
                {
                    // 0 <= ?? <= 23
                    auto hh = st.substr(0, 2);
                    if ( atoi(hh.c_str()) > 23 )
                        st.clear();
                }
                if (st.size() > 3 && st.size() < 5 )
                {
                    // 0 <= ?? <= 59
                    auto mm = st.substr(3, 2);
                    if ( atoi(mm.c_str()) > 59 )
                    {
                        st.pop_back(); st.pop_back();
                    }
                }
                if (st.size() > 6 )
                {
                    // 0 <= ?? <= 59
                    auto cc = st.substr(6, 2);
                    if ( atoi(cc.c_str()) > 59 )
                    {
                        st.pop_back(); st.pop_back();
                    }
                }

                if ( st.size() == 2  || st.size() == 5)
                {
                    st.push_back(':');
                }
            }
            break;
        }
        case GuiWindowsSubType::setFreq:
        case GuiWindowsSubType::setSpeed:
        {
            switch ( key )
            {
            case keyEnter:
            {  }
                break;
            case keyBack:
            {
                int i = 0;
                for (auto &k: estate.listItem)
                {
                    if (menu->focus == i)
                    {
                        if (k->inputStr.size() > 0)
                        {
                            k->inputStr.pop_back();
                        }
                        else
                        {
                            guiTree.backvard();
                            menu->focus = 0;
                            break;
                        }
                    }
                    i++;
                }
                if ( menu->focus == estate.listItem.size() )
                {
                    guiTree.backvard();
                    menu->focus = 0;
                }

            }
                break;
            default:
                if ( key > 5 && key < 16)
                {
                    menu->setSttParam(estate, key);
                }
                else if ( key == 1)
                {
                    int i = 0;
                    for (auto &k: estate.listItem)
                    {
                        if (menu->focus == i)
                        {
                            if (k->inputStr.size() > 0)
                            {
                                k->inputStr.pop_back();
                            }
                            else
                            {
                                guiTree.backvard();
                                menu->focus = 0;
                                break;
                            }
                        }
                        i++;
                    }
                }
                break;
            }
            break;
        }
        case GuiWindowsSubType::editRnKey:
        {
            // выбрать канал воспроизведения
            if ( key > 5 && key < 16 && menu->RN_KEY.size() < 2 )
            {
                menu->RN_KEY.push_back((char)(42+key));
                // check
                int rc = atoi(menu->RN_KEY.c_str());

                if ( rc < 1 || rc > 98 )
                { menu->RN_KEY.clear(); }
            }
            if (key == keyBack)
            {
                if (menu->RN_KEY.size() > 0)
                    menu->RN_KEY.pop_back();
                else
                {
                    uint8_t t; storageFs->getFhssKey(t);
                    char ch[3]; sprintf(ch, "%d", t); ch[2] = '\0';
                    menu->RN_KEY.append(ch);
                    menu->focus = 4;
                    guiTree.backvard();
                }
            }
            if (key == keyEnter)
            {
                storageFs->setFhssKey((uint8_t)atoi(menu->RN_KEY.c_str()));
                menu->focus = 4;
                guiTree.backvard();

            }
            break;
        }
        default:
            break;
        }
    }
        break;
    default:
        break;
    }

    draw();
}


int Service::getLanguage()
{
    return 0;
}

void Service::FirstPacketPSWFRecieved(int packet)
{
    if ( packet >= 0 && packet < 100 )
    {
        char sym[64];
        sprintf(sym,"%d",packet);
        guiTree.append(messangeWindow, "Recieved packet ", sym);
        msgBox( "Recieved packet ", (int)packet );
    }
    else if ( packet > 99)
    {
        guiTree.append(messangeWindow, "Recieved packet: Fatal error\t");
        msgBox( "Recieved packet: Fatal error\t" );
    }
    else
    {
        guiTree.append(messangeWindow, "Recieved packet: Unknow error\t");
        msgBox( "Recieved packet: Unknow error\t" );
    }
}

void Service::msgBox(const char *title)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(159), (GYT)(127)};
    if(msg_box == nullptr)
    {
        msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, align007);
    }
    msg_box->Draw();
}

void Service::msgBox(const char *title, const char *text)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(159), (GYT)(127)};

    if(msg_box == nullptr)
    {
        msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, (char*)text, align007);
    }
    msg_box->Draw();
}

void Service::msgBox(const char *title, const int condCmd)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(159), (GYT)(127)};

    if(msg_box == nullptr)
    {
        msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, (int)condCmd, align007);
    }
    msg_box->Draw();
}


void Service::msgBox(const char *title, const int condCmd, const int size, const int pos)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(159), (GYT)(127)};

    if(msg_box == nullptr)
    {
        msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, (int)condCmd, (int) size, (int) pos, align007);
    }
    else
    {
    	msg_box->setCmd(condCmd);
    	msg_box->position = pos;
    }
    msg_box->Draws();
}

void Service::drawMainWindow()
{
    main_scr->setModeText(mode_txt[main_scr->mainWindowModeId]);

    Multiradio::VoiceServiceInterface *voice_service = pGetVoiceService();

    main_scr->Draw(voice_service->getCurrentChannelStatus(),
                   voice_service->getCurrentChannelNumber(),
                   voice_service->getCurrentChannelType());


    //main_scr->oFreq.clear();
    char mas[11];

#ifdef _DEBUG_
    sprintf(mas,"%d",1);
#else
    sprintf(mas,"%d",voice_service->getCurrentChannelFrequency());
#endif

    //std::string freq(mas);
    //main_scr->oFreq.append(freq);
    //main_scr->setFreq(freq.c_str());

    bool gpsStatus = false;

    if (navigator != 0){
        Navigation::Coord_Date date = navigator->getCoordDate();
        char ch[10];
        memcpy(&ch,&date.data,10);
        if (atoi((const char*)ch) != 0)
        {
            gpsStatus = true;
        }
    }

    indicator->Draw(pGetMultitradioService()->getStatus(),
                    pGetHeadsetController()->getStatus(),
                    pGetPowerBattery()->getChargeLevel(),
                    gpsStatus);
}

void Service::drawMenu()
{
    Alignment align = {alignHCenter,alignTop};
    int focusItem;

    if( menu == nullptr )
    {
        menu = new CGuiMenu(&ui_menu_msg_box_area, guiTree.getCurrentState().getName(), align);
    }

    if ( guiTree.getCurrentState().getType() == menuWindow )
    {
        CState st = guiTree.getCurrentState();
        std::list<std::string> t;

        int removal = 0; QM_UNUSED(removal);
        focusItem = menu->focus;
        if (menu->focus > MAIN_MENU_MAX_LIST_SIZE)
        {
            removal = menu->focus - MAIN_MENU_MAX_LIST_SIZE;
            focusItem = MAIN_MENU_MAX_LIST_SIZE;
        }
        //
        // пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ
        //        for(auto i = removal; i < std::min((removal + MAIN_MENU_MAX_LIST_SIZE), (int)st.nextState.size()); i++)

        for (auto &k: st.nextState)
        {
            t.push_back( std::string(k->getName()) );
        }

        menu->initItems(t, st.getName(), focusItem);
        menu->Draw();
        t.clear();
    }
    else
    {
        CEndState st = (CEndState&)guiTree.getCurrentState();
        menu->setTitle(st.getName());

        switch( st.subType )
        {
        case GuiWindowsSubType::simpleCondComm:
        case GuiWindowsSubType::duplCondCmd:
        {
            menu->initCondCommDialog(st);
            break;
        }
        case GuiWindowsSubType::txGroupCondCmd:
        {
            menu->/*initTxGroupCondComm*/initGroupCondCmd(st);
            break;
        }
        case GuiWindowsSubType::txPutOffVoice:
        {
            int status = 0;;
            if (menu->putOffVoiceStatus == 2)
            {
                status = headset_controller->getSmartHSState();
            }
            else if (menu->putOffVoiceStatus == 5)
            {
                status = multiradio_service->getAleState();
                menu->vmProgress = multiradio_service->getAleVmProgress();
            }

            menu->initTxPutOffVoiceDialog(status);
            break;
        }
        case GuiWindowsSubType::message:
        {
            std::string titleStr, tmpStr; std::string *fieldStr;
            switch(menu->smsTxStage)
            {
            case 1:
            {
                titleStr.append(ticketStr[1]);

                if (menu->useSmsRetrans)
                    tmpStr.append(useScanMenu[0]);
                else
                    tmpStr.append(useScanMenu[1]);

                fieldStr = &tmpStr;

                break;
            }
            case 2:
            {
                auto iter = st.listItem.begin();
                (*iter)++; (*iter)++;

                titleStr.append(condCommStr[1]);
                fieldStr = &(*iter)->inputStr; // address retr
                break;}
            case 3:
            {
                auto iter = st.listItem.begin();

                titleStr.append(condCommStr[0]);
                fieldStr = &(*iter)->inputStr; // address dst
                break;
            }
            case 4:
            {
                auto iter = st.listItem.begin();
                (*iter)++;

                titleStr.append(condCommStr[4]);
                fieldStr = &(*iter)->inputStr; // message
                break;
            }
            case 5:
            {
                tmpStr.append(startAleTxVoiceMailStr);
                fieldStr = &tmpStr;
                break;
            }
            default:
            {break;}
            }
            menu->initTxSmsDialog( titleStr, *fieldStr );
            break;
        }
        case GuiWindowsSubType::recvCondCmd:
        {
            menu->initRxCondCmdDialog();
            break;
        }
        case GuiWindowsSubType::recvGroupCondCmd:
        case GuiWindowsSubType::recvVoice:
        case GuiWindowsSubType::recvSms:
        {
            menu->initRxSmsDialog();
            break;
        }
        case GuiWindowsSubType::rxPutOffVoice:
        {
            int status = 0;
            if (menu->putOffVoiceStatus == 5)
            {
                status = headset_controller->getSmartHSState();
            }
            else if (menu->putOffVoiceStatus == 2)
            {
                status   = multiradio_service->getAleState();
                menu->vmProgress = multiradio_service->getAleVmProgress();
            }

            menu->initRxPutOffVoiceDialog(status);
            break;
        }
        case GuiWindowsSubType::recvSilence:
        {
            guiTree.resetCurrentState();
            drawMainWindow();
            break;
        }
        case GuiWindowsSubType::gpsCoord:
        {
#if !defined(PORT__PCSIMULATOR)
            setCoordDate(navigator->getCoordDate());
#endif
            menu->initGpsCoordinateDialog( menu->coord_lat, menu->coord_log );
            break;
        }
        case GuiWindowsSubType::gpsSync:
        {
            menu->inclStatus =  gpsSynchronization;
            menu->initIncludeDialog();
            break;
        }
        case GuiWindowsSubType::setDate:
        {
            menu->setTitle(dataAndTime[0]);
            std::string str; str.append(st.listItem.front()->inputStr); //str.append("00.00.00");
            menu->initSetDateOrTimeDialog( str );
            break;
        }
        case GuiWindowsSubType::setTime:
        {
            menu->setTitle(dataAndTime[1]);
            std::string str; str.append(st.listItem.front()->inputStr); //str.append("00:00:00");
            menu->initSetDateOrTimeDialog( str );
            break;
        }
        case GuiWindowsSubType::setFreq:
        {
            std::string str; str.append(st.listItem.front()->inputStr); str.append(" ").append(freq_hz);
            menu->initSetParametersDialog( str );
            break;
        }
        case GuiWindowsSubType::setSpeed:
        {
            std::string str; str.append(st.listItem.front()->inputStr); if ( str.size() >= 5 ){ str.push_back('\n');} str.append(" ").append(speed_bit);
            menu->initSetParametersDialog( str );
            break;
        }
        case GuiWindowsSubType::twoState:
        {
            menu->initTwoStateDialog();
            break;
        }
        case GuiWindowsSubType::scan:
        {
            menu->inclStatus = menu->scanStatus;
            menu->initIncludeDialog();
            break;
        }
        case GuiWindowsSubType::suppress:
        {
            menu->inclStatus = menu->supressStatus;
            menu->initIncludeDialog();
            break;
        }
        case GuiWindowsSubType::aruarmaus:
        {
            menu->initAruarmDialog();
            break;
        }
        case GuiWindowsSubType::volume:
        {
            menu->initVolumeDialog();
            break;
        }
        case GuiWindowsSubType::editRnKey:
        {
            menu->initEditRnKeyDialog();
            break;
        }
        default:
            break;
        }
    }
}

void Service::draw()
{
    CState currentState;
    guiTree.getLastElement(currentState);

    switch(currentState.getType())
    {
    case mainWindow:{
        drawMainWindow();
        break;
    }
    case messangeWindow:
    {
        int cmd = atoi(currentState.getText());
        if (vect != nullptr)
                		msgBox(currentState.getName(), vect[position], vect[0], position);
        else
        if ( cmd >= 0 && cmd < 100)
            if (vect != nullptr)
                msgBox(currentState.getName(), vect[position], vect[0], position);
            else
                msgBox( currentState.getName(), cmd );
        	else
        		msgBox( currentState.getName(), currentState.getText() );
    break;
    }
    case menuWindow:
        drawMenu();
        break;
    case endMenuWindow:
        drawMenu();
        break;
    default:
        break;
    }
}

int Service::getFreq()
{
    return isFreq;
}

void Service::setFreq(int isFreq)
{
    Service::isFreq = isFreq;
}

void Service::parsingGucCommand(uint8_t *str)
{
    int index = 0;
    char number[3] = {'\0','\0','\0'};
    int cnt = 0;

    int len = strlen((const char*)str);
    for(int i = 0; i<=len;i++){
        if ((str[i] == ' ') || (len == i))
        {
            if (i - index == 2)
                number[2] = '\0';
            if (i - index == 1)
                number[1] = '\0';

            memcpy(number,&str[index],i - index);
            guc_command_vector.push_back(atoi(number));
            ++cnt;
            for(int j = 0; j<3;j++) number[j] = '\0';
            index = i+1;
        }
    }
}

void Service::setCoordDate(Navigation::Coord_Date date)
{
    menu->coord_lat.clear();
    menu->coord_log.clear();
    menu->date.clear();
    menu->time.clear();

    menu->coord_lat.append((char *)date.latitude);

    date.longitude[11] = '\0';

    uint8_t *abc;
    if (date.longitude[0] == '0') {
    	abc = &(date.longitude[1]);
    	menu->coord_log.append((char*)abc);
    } else {
    	menu->coord_log.append((char*)date.longitude);
    }

    std::string str;
    str.push_back((char)date.data[0]);
    str.push_back((char)date.data[1]);
    str.push_back('.');
    str.push_back((char)date.data[2]);
    str.push_back((char)date.data[3]);

    str.push_back((char)' ');
    str.push_back((char)date.time[0]);
    str.push_back((char)date.time[1]);
    str.push_back((char)':');
    str.push_back((char)date.time[2]);
    str.push_back((char)date.time[3]);

    str.push_back((char)':');
    str.push_back((char)date.time[4]);
    str.push_back((char)date.time[5]);
    qmDebugMessage(QmDebug::Dump, "DATE TIME %s :", str.c_str());
    indicator->date_time->SetText((char *)str.c_str());
    drawIndicator();
    str.clear();
}

void Service::gucFrame()
{
    const char *sym = "Recieved packet for station\0";
    vect = voice_service->getGucCommand();
    char ch[3]; sprintf(ch, "%d", vect[position]); ch[2] = '\0';
    guiTree.append(messangeWindow, sym, ch);
    msgBox( "Recieved Guc\0", vect[position], vect[0], position);
}



void Service::updateSystemTime()
{
    if ( true/*gpsSynchronization*/ )
    {

        setCoordDate(navigator->getCoordDate());
    }
    else
    {
        Navigation::Coord_Date data;
        memset( &data.latitude,  0, 11 );
        memset( &data.longitude, 0, 11 );

        data.data[0] = 2;
        data.data[1] = 6;
        data.data[2] = 0;
        data.data[3] = 4;

        data.time[0] = 1;
        data.time[1] = 2;
        data.time[2] = 1;
        data.time[3] = 1;

        setCoordDate(data );
    }

    systemTimeTimer->start();
}

void Service::smsMessage()
{
    char sym[50];//TODO:
    for(int i = 0; i<50;++i) sym[i] = '0';

    memcpy(sym, voice_service->getSmsContent(), 50);
    sym[49] = '\0';

    guiTree.append(messangeWindow, "Recieved SMS ", sym);
    msgBox( "Recieved SMS", sym );
}

void Service::updateAleVmProgress(uint8_t t)
{
    QM_UNUSED(t);

    CState currentState;
    guiTree.getLastElement(currentState);

    if (currentState.getType() == endMenuWindow)
    {
        GuiWindowsSubType subType = ((CEndState&)guiTree.getCurrentState()).subType;
        if ( (subType == txPutOffVoice && (menu->putOffVoiceStatus == 5)) || (subType == rxPutOffVoice && (menu->putOffVoiceStatus == 2)))
            drawMenu();
    }
}

void Service::updateAleState(Multiradio::MainServiceInterface::AleState state)
{
    QM_UNUSED(state);

    CState currentState;
    guiTree.getLastElement(currentState);

    if (currentState.getType() == endMenuWindow)
    {
        GuiWindowsSubType subType = ((CEndState&)guiTree.getCurrentState()).subType;
        if ( (subType == txPutOffVoice && (menu->putOffVoiceStatus == 5)) || (subType == rxPutOffVoice && (menu->putOffVoiceStatus == 2)))
            drawMenu();
    }
}

void Service::updateHSState(Headset::Controller::SmartHSState state)
{
    QM_UNUSED(state);

    CState currentState;
    guiTree.getLastElement(currentState);

    if (currentState.getType() == endMenuWindow)
    {
        GuiWindowsSubType subType = ((CEndState&)guiTree.getCurrentState()).subType;
        if ( (subType == txPutOffVoice && (menu->putOffVoiceStatus == 2)) || (subType == rxPutOffVoice && (menu->putOffVoiceStatus == 5)))
            drawMenu();
    }
}

}/* namespace Ui */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(service, LevelDefault)
#include "qmdebug_domains_end.h"
