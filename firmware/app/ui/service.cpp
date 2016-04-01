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

MoonsGeometry ui_common_dialog_area = { 0,24,GDISPW-1,GDISPH-1 };
MoonsGeometry ui_msg_box_area       = { 20,29,GDISPW-21,GDISPH-11 };
MoonsGeometry ui_menu_msg_box_area  = { 5,5,GDISPW-5,GDISPH-5 };
MoonsGeometry ui_indicator_area     = { 0,0,GDISPW-1,23 };



using namespace MessagesPSWF;

namespace Ui {



bool Service::single_instance = false; // � ·� °� І� ёСЃ� ё� ј� ѕСЃС‚СЊ � ѕС‚ � µ� ґ� ё� ЅСЃС‚� І� µ� Ѕ� Ѕ� ѕ� і� ѕ � ґ� ёСЃ� ї� »� µСЏ � І СЃ� ёСЃС‚� µ� ј� µ

Service::Service( matrix_keyboard_t                  matrixkb_desc,
                  aux_keyboard_t                     auxkb_desc,
                  Headset::Controller               *headset_controller,
                  Multiradio::MainServiceInterface  *mr_main_service,
                  Multiradio::VoiceServiceInterface *mr_voice_service,
                  Power::Battery                    *power_battery
                  )
{
    QM_ASSERT(single_instance == false);
    single_instance = true;

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

    mainWindowModeId = 0;
    //drawMainWindow();

    menu = nullptr;
    msg_box = nullptr;

    // � ёСЃ� їСЂ� °� І� ёС‚СЊ � Ѕ� ° СЃ� µСЂ� І� ёСЃ
    this->headset_controller->statusChanged.connect(sigc::mem_fun(this, &Service::updateBattery));
    this->multiradio_service->statusChanged.connect(sigc::mem_fun(this, &Service::updateMultiradio));
    this->power_battery->chargeLevelChanged.connect(sigc::mem_fun(this, &Service::updateBattery));

    guiTree.append(messangeWindow, (char*)test_Pass);
    msgBox(guiTree.getCurrentState().getName() );

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
    // Главный экран
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
                // фиксация изменений
                if (main_scr->mwFocus == 0)
                {
                    main_scr->oFreq.clear();
                    main_scr->oFreq.append(main_scr->nFreq.c_str());
                    int freq = atoi(main_scr->nFreq.c_str());
                    voice_service->TuneFrequency(freq);
                }
                if (main_scr->mwFocus == 1)
                {
                    //
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
            case keyLeft:
                if (main_scr->mwFocus == 1 && main_scr->mainWindowModeId > 0)
                    main_scr->mainWindowModeId--;
                break;
            case keyRight:
                if (main_scr->mwFocus == 1 && main_scr->mainWindowModeId < 2)
                    main_scr->mainWindowModeId++;
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
        // в меню
    case menuWindow:
    {
        if ( key == keyEnter)
        {
            int rc;
            rc = guiTree.advance(menu->focus);
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

        if ( key == keyEnter)
        {
            if (menu->focus == estate.listItem.size())
            {
                guiTree.resetCurrentState();
            }
            menu->focus = 0;
        }
        if ( key == keyBack)
        {
            if (menu->focus == 2)
            {
                guiTree.backvard();
                menu->focus = 0;
            }
        }

        switch(estate.subType)
        {
        case GuiWindowsSubType::simpleCondComm:
        {
            switch (key){
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
                { /* callback */ }
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

            // simplo
            if (estate.listItem.size() == 2)
            {}
            else if (estate.listItem.size() == 1)
            {}
            else {}
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
                { /* callback */ }
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

            // duplo
            if (estate.listItem.size() == 2)
            {}
            else if (estate.listItem.size() == 1)
            {}
            else {}
        }
            break;
        case GuiWindowsSubType::volume:
        {
            if (key == keyUp  )
            {
                menu->incrVolume();
                uint8_t level = menu->getVolume();
                voice_service->TuneAudioLevel(level);

            }
            if (key == keyDown)
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
            break;
        case GuiWindowsSubType::aruarm:
        {
            if (key == keyUp  )
            {
                menu->incrAruArm();
                uint8_t level = menu->getAruArm();
                voice_service->TurnAGCMode(level);

            }
            if (key == keyDown)
            {
                menu->decrAruArm();
                uint8_t level = menu->getAruArm();
                voice_service->TurnAGCMode(level);
            }
            break;
        }
        case GuiWindowsSubType::gpsCoord:
            break;
        case GuiWindowsSubType::gpsSync:
            break;
        case GuiWindowsSubType::setDate:
        case GuiWindowsSubType::setTime:
        case GuiWindowsSubType::setFreq:
        case GuiWindowsSubType::setSpeed:
            switch ( key )
            {
            case keyEnter:
            {  }
                break;
            case keyBack:
            {
                guiTree.backvard();
                menu->focus = 0;
            }
                break;
            default:
                break;
            }
            break;
        case GuiWindowsSubType::suppress:
            break;
        default:
            break;
        }
    }
        break;
    default:
        break;
    }

    if (false)
    {
        //        goToCallBack();
    }
    draw();
}


int Service::getLanguage()
{
    return 0;
}

void Service::msgBox(const char *text)
{
    Alignment align = {alignHCenter,alignTop};
    MoonsGeometry area = {0, 0, (GXT)(159), (GYT)(127)};
    if(msg_box == nullptr)
    {
        msg_box = new GUI_Dialog_MsgBox(&area, (char*)text, align);
    }
    msg_box->Draw();
}

void Service::drawMainWindow()
{
    main_scr->setModeText(mode_txt[mainWindowModeId]);

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
    const char* text = "";
    int focusItem;

    if(menu == nullptr)
    {
        menu = new CGuiMenu(&ui_menu_msg_box_area, guiTree.getCurrentState().getName(), text, align);
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
        // ���������
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
        case simpleCondComm:
        case duplCondComm:
            menu->initCondCommDialog(st);
            break;
        case recv:
            //menu->initTwoStateDialog();
            break;
        case data:
            //menu->initTwoStateDialog();
            break;
        case settings:
            //menu->initTwoStateDialog();
            break;
        case gpsCoord:
            menu->initGpsCoordinateDialog();
            break;
        case gpsSync:
            break;
        case setDate:
        case setTime:
        case GuiWindowsSubType::setFreq:
        case setSpeed:
            break;
        case twoState:
            menu->initTwoStateDialog();
        case aruarm:
            menu->initAruarmDialog();
            break;
        case volume:
            menu->initVolumeDialog();
            break;
//        case date:
//            break;
        default:
            //menu->initTwoStateDialog();
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
    case mainWindow:
        drawMainWindow();
        break;
    case messangeWindow:
        msgBox( currentState.getName() );
        break;
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

void Service::setCoordDate(Navigation::Coord_Date*(*func)())
{
   Navigation::Coord_Date* date = (Navigation::Coord_Date*)func;

   menu->coord_lat.append((char *)date->latitude);
   menu->coord_log.append((char *)date->longitude);
   menu->date.append((char *)date->data);
   menu->time.append((char *)date->time);
}


}/* namespace Ui */
