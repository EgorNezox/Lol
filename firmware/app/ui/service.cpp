/**
 ******************************************************************************
 * @file    service.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include <stdlib.h>
#include "qm.h"
#include "qmdebug.h"
#include "dialogs.h"
#include "service.h"
#include "texts.h"
#include "messages/messagepswf.h"
#include <thread>
#include <navigation/navigator.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

MoonsGeometry ui_common_dialog_area = { 0,24,GDISPW-1,GDISPH-1 };
MoonsGeometry ui_msg_box_area       = { 20,29,GDISPW-21,GDISPH-11 };
MoonsGeometry ui_menu_msg_box_area  = { 1,1,GDISPW-2,GDISPH-2 };
MoonsGeometry ui_indicator_area     = { 0,0,GDISPW-1,23 };



using namespace MessagesPSWF;

namespace Ui {



bool Service::single_instance = false; // пїЅ В·пїЅ В°пїЅ Р†пїЅ С‘РЎРѓпїЅ С‘пїЅ С�пїЅ С•РЎРѓРЎвЂљРЎРЉ пїЅ С•РЎвЂљ пїЅ ВµпїЅ Т‘пїЅ С‘пїЅ Р…РЎРѓРЎвЂљпїЅ Р†пїЅ ВµпїЅ Р…пїЅ Р…пїЅ С•пїЅ С–пїЅ С• пїЅ Т‘пїЅ С‘РЎРѓпїЅ С—пїЅ В»пїЅ ВµРЎРЏ пїЅ Р† РЎРѓпїЅ С‘РЎРѓРЎвЂљпїЅ ВµпїЅ С�пїЅ Вµ

Service::Service( matrix_keyboard_t                  matrixkb_desc,
                  aux_keyboard_t                     auxkb_desc,
                  Headset::Controller               *headset_controller,
                  Multiradio::MainServiceInterface  *mr_main_service,
                  Multiradio::VoiceServiceInterface *mr_voice_service,
                  Power::Battery                    *power_battery,
                  Navigation::Navigator             *navigator )
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

    ginit();
    voice_service->currentChannelChanged.connect(sigc::mem_fun(this, &Service::voiceChannelChanged));

    keyboard= new QmMatrixKeyboard(matrix_kb.resource);
    keyboard->keyAction.connect(sigc::mem_fun(this, &Service::keyHandler));
    chnext_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChNext]);
    chprev_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChPrev]);
    chnext_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chNextHandler));
    chprev_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chPrevHandler));

    main_scr  = new GUI_Dialog_MainScr(&ui_common_dialog_area);
    indicator = new GUI_Indicator     (&ui_indicator_area);

    menu = nullptr;
    msg_box = nullptr;

    // пїЅ С‘РЎРѓпїЅ С—РЎР‚пїЅ В°пїЅ Р†пїЅ С‘РЎвЂљРЎРЉ пїЅ Р…пїЅ В° РЎРѓпїЅ ВµРЎР‚пїЅ Р†пїЅ С‘РЎРѓ
    this->headset_controller->statusChanged.connect(sigc::mem_fun(this, &Service::updateBattery));
    this->multiradio_service->statusChanged.connect(sigc::mem_fun(this, &Service::updateMultiradio));
    this->power_battery->chargeLevelChanged.connect(sigc::mem_fun(this, &Service::updateBattery));

    guiTree.append(messangeWindow, (char*)test_Pass, voice_service->ReturnSwfStatus());
    msgBox( guiTree.getCurrentState().getName(), guiTree.getCurrentState().getText() );

    voice_service->PswfRead.connect(sigc::mem_fun(this,&Service::getPSWF));
    command_rx_30 = 0;

    voice_service->firstPacket.connect(sigc::mem_fun(this,&Service::FirstPacketPSWFRecieved));
    voice_service->smsMess.connect(sigc::mem_fun(this,&Service::smsMessage));
    voice_service->smsFailed.connect(sigc::mem_fun(this,&Service::FailedSms));
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
    char sym[64];

    if (stage == 0){
        guiTree.append(messangeWindow, "Failed Sms", sms_quit_fail1);
        msgBox( "Recieved packet ", sms_quit_fail1 );
    }

    if (stage == 1){
        guiTree.append(messangeWindow, "Failed Sms", sms_quit_fail2);
        msgBox( "Recieved packet ", sms_quit_fail2);
    }

    if (stage == 3)
    {
    	 guiTree.append(messangeWindow, "Failed Sms", sms_quit_fail2);
    	 msgBox( "Failed Sms", sms_crc_fail);
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
    // Р“Р»Р°РІРЅС‹Р№ СЌРєСЂР°РЅ
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
                // С„РёРєСЃР°С†РёСЏ РёР·РјРµРЅРµРЅРёР№
                if (main_scr->mwFocus == 0)
                {
                    main_scr->oFreq.clear();
                    main_scr->oFreq.append(main_scr->nFreq.c_str());
                    int freq = atoi(main_scr->nFreq.c_str());
                    voice_service->TuneFrequency(freq);
                }
                // Речь
                switch ( main_scr->mainWindowModeId )
                {
                case 0:
                {}
                    // АУС
                case 1:
                {}
                    // ГУК
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
        }
        else
            msg_box->keyPressed(key);
        break;
    }
        // РІ РјРµРЅСЋ
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

        //        if ( key == keyEnter)
        //        {
        //            if (menu->focus == estate.listItem.size())
        //            {
        //                guiTree.resetCurrentState();
        //            }
        //            menu->focus = 0;
        //        }

        switch(estate.subType)
        {
        case GuiWindowsSubType::simpleCondComm:  // СѓСЃР»РѕРІРЅС‹Рµ РєРѕРјР°РЅРґС‹
        {
            switch (key)
            {
            case keyUp:
                if ( menu->focus > 0 )
                    menu->focus--;
                break;
            case keyDown:
            {
                if ( menu->focus < estate.listItem.size() )
                    menu->focus++;
            }
                break;
            case keyEnter:
            {
                bool flag = false;
                if ( estate.listItem.size() == 2 )
                {
                    if (estate.listItem.front()->inputStr.size() != 0 &&
                            estate.listItem.back()->inputStr.size() != 0 )
                    { flag = true; }
                }
                else
                {
                    if ( estate.listItem.front()->inputStr.size() != 0 )
                    { flag = true; }
                }

                if ( menu->focus == estate.listItem.size() && flag )
                {
                    /* callback */
                    int param[2]; // 0 -R_ADR, 1 - COM_N
                    int i = 0;

                    for (auto &k: estate.listItem)
                    {
                        param[i] = atoi(k->inputStr.c_str());
                        i++;
                    }
                    voice_service->TurnPSWFMode(1,param[0],param[1]);
                }
                break;
            }
            default:
                if ( key > 5 && key < 16)
                {
                    menu->setCondCommParam(estate, key);
                }
                else if ( key == keyBack)
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
            }
            break;
        }
        case GuiWindowsSubType::duplCondComm:
        {
            switch (key)
            {
            case keyUp:
                if ( menu->focus > 0 )
                    menu->focus--;
                break;
            case keyDown:
            {
                if ( menu->focus < estate.listItem.size() )
                    menu->focus++;
            }
                break;
            case keyEnter:
                if ( menu->focus < estate.listItem.size() )
                {
                    //TODO: temp fix))

                    int param[2]; // 0 -R_ADR, 1 - COM_N
                    int i = 0;

                    for (auto &k: estate.listItem)
                    {
                        param[i] = atoi(k->inputStr.c_str());
                        i++;
                    }
                    voice_service->TurnPSWFMode(0,param[0],param[1]);
                }
                break;
            default:
                if ( key > 5 && key < 16)
                {
                    menu->setCondCommParam(estate, key);
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
        case GuiWindowsSubType::message:
        {
            switch ( key )
            {
            case keyUp:
            {
                if ( menu->focus > 0 )
                    menu->focus--;
            }
                break;
            case keyDown:
            {
                if ( menu->focus < estate.listItem.size() )
                    menu->focus++;
            }
                break;
            case keyBack:
            {
                int i = 0;
                CEndState elem = (CEndState&)guiTree.getCurrentState();
                for ( auto &k: elem.listItem )
                {
                    if ( menu->focus == i)
                    {
                        if ( k->inputStr.size() > 0 )
                            k->inputStr.pop_back();
                        else
                        {
                            guiTree.backvard();
                            menu->focus = 0;
                        }
                    }
                    i++;
                }
                break;
            }
            case keyEnter:
            {
                CEndState elem = (CEndState&)guiTree.getCurrentState();
                if (menu->focus == 2)
                {
                    if (elem.listItem.front()->inputStr.size() != 0 && elem.listItem.back()->inputStr.size() != 0)
                    {

                        int r_adr;
                        char mes[100];
                        int cnt = 0;

                        menu->focus = 0;
                        guiTree.resetCurrentState();
                        for ( auto &k: elem.listItem)
                        {
                            if (cnt == 0) r_adr = atoi(k->inputStr.c_str());
                            else strcpy(mes,k->inputStr.c_str());
                            k->inputStr.clear();
                            cnt++;
                        }

                        voice_service->TurnSMSMode(r_adr,mes);
                    }
                }
                break;
            }
            default:
                CEndState elem = (CEndState&)guiTree.getCurrentState();
                if ( menu->focus == 0)
                {
                    if ( elem.listItem.front()->inputStr.size() < 2 )
                        menu->inputSmsAddr( (CEndState&)guiTree.getCurrentState(), key );
                }
                else if ( menu->focus == 1 )
                {
                    if ( elem.listItem.back()->inputStr.size() < 74 )
                        menu->inputSmsMessage( (CEndState&)guiTree.getCurrentState(), key );
                }
                else
                {}
                break;
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
                /* call */
                guiTree.resetCurrentState();
                voice_service->TurnSMSMode();
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
                menu->scanStatus = true;
                menu->inclStatus = true;
            }
            if ( key == keyLeft  || key == keyDown )
            {
                menu->scanStatus = false;
                menu->inclStatus = false;
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
            if ( key == keyRight || key == keyUp )
            {
                menu->supressStatus = true;
                menu->inclStatus = true;
            }
            if ( key == keyLeft  || key == keyDown )
            {
                menu->supressStatus = false;
                menu->inclStatus = false;
            }
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->focus = 0;
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
            if ( key == keyLeft )
            {
                if ( menu->focus == 0 )
                {
                    menu->decrAruArmAsu(estate.subType);
                }
                else if ( menu->focus == 1)
                {
                    menu->decrAruArmAsu(estate.subType);
                }
                else
                {
                    menu->decrAruArmAsu(estate.subType);
                }
                uint8_t vol = menu->getAruArmAsu();
                voice_service->TurnAGCMode(vol, menu->focus);
            }
            if ( key == keyRight )
            {
                if ( menu->focus == 0 )
                {
                    menu->incrAruArmAsu(estate.subType);
                }
                else if ( menu->focus == 1 )
                {
                    menu->incrAruArmAsu(estate.subType);
                }
                else
                {
                    menu->incrAruArmAsu(estate.subType);
                }
                uint8_t vol = menu->getAruArmAsu();
                voice_service->TurnAGCMode(vol, menu->focus);
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
            break;
        }
        case GuiWindowsSubType::gpsSync:
        {
            break;
        }
        case GuiWindowsSubType::setDate:
        case GuiWindowsSubType::setTime:
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
        msgBox( "Recieved packet ", sym );
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

void Service::drawMainWindow()
{
    main_scr->setModeText(mode_txt[main_scr->mainWindowModeId]);

    Multiradio::VoiceServiceInterface *voice_service = pGetVoiceService();

    main_scr->Draw(voice_service->getCurrentChannelStatus(),
                   voice_service->getCurrentChannelNumber(),
                   voice_service->getCurrentChannelType());

    indicator->Draw(pGetMultitradioService()->getStatus(),
                    pGetHeadsetController()->getStatus(),
                    pGetPowerBattery()->getChargeLevel()
                    );
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

        int removal = 0;
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
        case GuiWindowsSubType::duplCondComm:
            menu->initCondCommDialog(st);
            break;
        case GuiWindowsSubType::message:
            menu->initTxSmsDialog( st.getName(), st.listItem.front()->inputStr, st.listItem.back()->inputStr );
            break;
        case GuiWindowsSubType::recvSms:
            menu->initRxSmsDialog();
            break;
        case GuiWindowsSubType::gpsCoord:
#if !defined(PORT__PCSIMULATOR)
            setCoordDate(navigator->getCoordDate());
#endif
            menu->initGpsCoordinateDialog();
            break;
        case GuiWindowsSubType::gpsSync:
            break;
        case GuiWindowsSubType::setDate:
        {
            std::string str; str.append(st.listItem.front()->inputStr);// str.append(freq_hz);
            menu->initSetParametersDialog( str );
            break;
        }
        case GuiWindowsSubType::setTime:
        {
            std::string str; str.append(st.listItem.front()->inputStr);// str.append(freq_hz);
            menu->initSetParametersDialog( str );
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
            menu->initTwoStateDialog();
        case GuiWindowsSubType::scan:
            menu->inclStatus = menu->scanStatus;
            menu->initIncludeDialog();
            break;
        case GuiWindowsSubType::suppress:
            menu->inclStatus = menu->supressStatus;
            menu->initIncludeDialog();
            break;
        case GuiWindowsSubType::aruarmaus:
            menu->initAruarmDialog();
            break;
        case GuiWindowsSubType::volume:
            menu->initVolumeDialog();
            break;
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
        msgBox( currentState.getName(), currentState.getText() );
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

void Service::setCoordDate(Navigation::Coord_Date date)
{
    menu->coord_lat.clear();
    menu->coord_log.clear();
    menu->date.clear();
    menu->time.clear();

    menu->coord_lat.append((char *)date.latitude);
    menu->coord_log.append((char *)date.longitude);

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

    indicator->date_time->SetText((char *)str.c_str());
    str.clear();
}

void Service::getPSWF()
{
    if (command_rx_30 < 30)
    {
        int *pointer = voice_service->ReturnDataPSWF();
        BasePswfCadr.push_back(pointer);
        int freq = pointer[1];
        voice_service->TuneFrequency(freq);
        command_rx_30++;
    }
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

}/* namespace Ui */
