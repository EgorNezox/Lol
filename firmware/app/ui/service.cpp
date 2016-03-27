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

MoonsGeometry ui_common_dialog_area = { 0,24,GDISPW-1,GDISPH-1 };
MoonsGeometry ui_msg_box_area       = { 20,29,GDISPW-21,GDISPH-11 };
MoonsGeometry ui_menu_msg_box_area  = { 5,5,GDISPW-5,GDISPH-5 };
MoonsGeometry ui_indicator_area     = { 0,0,GDISPW-1,23 };



using namespace MessagesPSWF;

namespace Ui {



bool Service::single_instance = false; // зависимость от единственного дисплея в системе

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
    //...

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
    drawMainWindow();

    menu = nullptr;
    msg_box = nullptr;

    // исправить на сервис
    this->headset_controller->statusChanged.connect(sigc::mem_fun(this, &Service::updateBattery));
    this->multiradio_service->statusChanged.connect(sigc::mem_fun(this, &Service::updateMultiradio));
    this->power_battery->chargeLevelChanged.connect(sigc::mem_fun(this, &Service::updateBattery));
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
    if ( guiTree.getCurrentState().getType() == mainWindow)
        indicator->Draw();
}

Service::~Service() {
    QM_ASSERT(single_instance == true);
    single_instance = false;
    //...

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
    MessagesPSWF::MessagePswf *pswf;// указываем ссылку на класс отправки ППРЧ
    CState state = guiTree.getCurrentState();

//    if (getFreq() == 1)
//     {
//        if ((key >=6) && (key<=15))
//        {
//            mas_freq[mas_cnt] = (char)48+(key-6);
//            mas_cnt++;
//        }
//            draw();
//            guiTree.append(messangeWindow,mas_freq);


//            if (key == keyChNext)
//            {
//                QString str(mas_freq);
//                int freq = str.toInt();
//                voice_service ->TuneFrequency(freq);
//               // guiTree.append(messangeWindow,"Установка частоты выполнена!");
//                setFreq(0);
//                for(int i = 0; i<10;i++)
//                    mas_freq[i] = 0;
//                mas_cnt = 0;

//            }


//            draw();
//            //guiTree.delLastElement();

//            if (msg_box != nullptr)
//            {
//                delete msg_box;
//                msg_box = nullptr;
//            }

//     }
     if ((status_rx == true) && (key != keyUp))
     {
          if ( (key >=6) && (key <=15))
          mas_freq[mas_cnt] = (char) 48 + (key-6);
          mas_cnt++;
          main_scr->setFreq(mas_freq);

     }

     else

     if (main_scr->mwFocus == 0)
     {
         for(int i=0;i<mas_cnt;i++)
         mas_freq[i] = 0;

         status_rx = false;

     }

     if (main_scr->mwFocus == 1)
     {
         status_rx = true;

         main_scr->clearFreq();

     }

    switch( state.getType() )
    {
    // Главный экран
    case mainWindow:
    {
        switch( key )
        {
        case keyEnter:
            if (status_rx)
            {

                status_rx = false;
                main_scr->setFreq(mas_freq);
                int freq = atoi(mas_freq);
                voice_service ->TuneFrequency(freq);
                mas_cnt=0;
                draw();

            }
            else
               guiTree.advance(0);
            break;
        case keyChNext:
            pGetVoiceService()->tuneNextChannel();
            break;
        case keyChPrev:
            pGetVoiceService()->tunePreviousChannel();
            break;
        case keyLeft:
            if (mainWindowModeId > 0)
                mainWindowModeId--;
            break;
        case keyRight:
            if (mainWindowModeId < 2)
                mainWindowModeId++;
            break;
        case key0:
           setFreq(1);
          guiTree.append(messangeWindow,"Режим установки частоты");
          //pswf->MessageSendPswf(MessagePswf::UartDeviceAddress::TransmissionToDsp,
                                //MessagePswf::PswfMessageIndicator::TransmitPackage,
                               //0.3,0.3,0,0,0,0.3,0);
            break;
        //case key1:
            //setFreq(1);
            //break;
        default:
             main_scr->keyPressed(key);
            break;
        }
        break;
    }
    case messangeWindow:
    {
        if ( key == keyEnter)
        {
            guiTree.delLastElement();
            delete msg_box;
            msg_box = nullptr;
        }
        break;
    }
    // в меню
    case menuWindow:
    {
    	// переходим вниз по дереву & запоминаем состояние
        if ( key == keyEnter)
        {
            int rc = guiTree.advance(menu->focus);
            menu->focus = 0;

            // ввод параметров
            if (rc == -1)
            {
                //
            }
        }
        // переходим вверх по дереву & удаляем из стзка последнее состояние
        if ( key == keyBack)
        {
            int rc = guiTree.backvard();
            menu->focus = 0;
        }
        // движемся по списку вверх
        if (key == keyUp)
        {
            if ( menu->focus > 0 )
                menu->focus--;
        }
        // движемся по списку вниз
        if (key == keyDown)
        {
            if ( menu->focus < state.nextState.size()-1 )
                menu->focus++;
        }
        break;
    }
    default:
        break;
    }

    draw();
    guiTree.delLastElement();

    if (msg_box != nullptr)
    {
        delete msg_box;
        msg_box = nullptr;
    }
}

int Service::getLanguage()
{
    return 0;
}

void Service::msgBox(const char *text)
{
    Alignment align = {alignHCenter,alignTop};
    MoonsGeometry area = {0, 0, (GXT)(160), (GYT)(120)};
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
    CState st = guiTree.getCurrentState();
    Alignment align = {alignHCenter,alignTop};
    const char* text = "";
    int focusItem;

    if(menu == nullptr)
    {
        menu = new CGuiMenu(&ui_menu_msg_box_area, st.getName(), text, align);
    }
    std::vector<std::string> t;
    if (st.nextState.size() > 0)
    {
        int removal = 0;
        focusItem = menu->focus;
        if (menu->focus > MAIN_MENU_MAX_LIST_SIZE)
        {
            removal = menu->focus - MAIN_MENU_MAX_LIST_SIZE;
            focusItem = MAIN_MENU_MAX_LIST_SIZE;
        }

        for(auto i = removal; i < std::min((removal + MAIN_MENU_MAX_LIST_SIZE), (int)st.nextState.size()); i++)
        {
            t.push_back( std::string(st.nextState[i]->getName()) );
        }
        menu->initItems(t, st.getName(), focusItem);

    }
    else
    {
        menu->setTitle(st.getName());
        menu->initDialog();
    }
    menu->Draw();
    t.clear();
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


}/* namespace Ui */
