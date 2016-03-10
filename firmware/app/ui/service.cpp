/**
 ******************************************************************************
 * @file    service.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#include "qm.h"
#include "qmdebug.h"
#include "dialogs.h"
#include "service.h"
#include "texts.h"

MoonsGeometry ui_common_dialog_area={ 0,24,GDISPW-1,GDISPH-1 };
MoonsGeometry ui_msg_box_area={ 20,29,GDISPW-21,GDISPH-11 };
MoonsGeometry ui_menu_msg_box_area = { 5,5,GDISPW-5,GDISPH-5 };
MoonsGeometry ui_indicator_area={ 0,0,GDISPW-1,23 };

namespace Ui {



bool Service::single_instance = false; // Р·Р°РІРёСЃРёРјРѕСЃС‚СЊ РѕС‚ РµРґРёРЅСЃС‚РІРµРЅРЅРѕРіРѕ РґРёСЃРїР»РµСЏ РІ СЃРёСЃС‚РµРјРµ

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
	msg_box=NULL;

    this->matrix_kb = matrixkb_desc;
	this->aux_kb=auxkb_desc;
	this->multiradio_service=mr_main_service;
	this->voice_service=mr_voice_service;
	this->power_battery=power_battery;
	this->headset_controller=headset_controller;

	ginit();
	voice_service->currentChannelChanged.connect(sigc::mem_fun(this, &Service::voiceChannelChanged));
	keyboard= new QmMatrixKeyboard(matrix_kb.resource);
	keyboard->keyAction.connect(sigc::mem_fun(this, &Service::keyHandler));
	chnext_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChNext]);
	chprev_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChPrev]);
	chnext_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chNextHandler));
	chprev_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chPrevHandler));

    main_scr = new GUI_Dialog_MainScr(&ui_common_dialog_area);
	main_scr->Draw();

    indicator = new GUI_Indicator(&ui_indicator_area);
	indicator->Draw();

    menu = nullptr;
    msg_box = nullptr;

//    this->headset_controller->statusChanged.connect(sigc::mem_fun(indicator, &GUI_Indicator::UpdateHeadset));
//    this->multiradio_service->statusChanged.connect(sigc::mem_fun(indicator, &GUI_Indicator::UpdateMultiradio));
    this->power_battery->chargeLevelChanged.connect(sigc::mem_fun(indicator, &GUI_Indicator::UpdateBattery));
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

void Service::setNotification(NotificationType type) {
    CGuiTree& instance = CGuiTree::getInstance();
	switch(type){
        case NotificationMissingVoiceChannelsTable:
            instance.append(messangeWindow, missing_ch_table_txt[getLanguage()]);
			break;
        case NotificationMissingOpenVoiceChannels:
            instance.append(messangeWindow, missing_open_ch_txt[getLanguage()]);
            break;
        case NotificationMismatchVoiceChannelsTable:
            instance.append(messangeWindow, ch_table_mismatch_txt[getLanguage()]);
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

Multiradio::VoiceServiceInterface* Service::pGetVoiceService(){
	return voice_service;
}

Multiradio::MainServiceInterface* Service::pGetMultitradioService(){
	return multiradio_service;
}

Power::Battery * Service::pGetPowerBattery(){
	return power_battery;
}

void Service::chNextHandler(){
	if(chnext_bt->isPressed()){
		keyPressed(keyChNext);
	}
}

void Service::chPrevHandler(){
	if(chprev_bt->isPressed()){
		keyPressed(keyChPrev);
	}
}

void Service::voiceChannelChanged(){
//        main_scr->Draw();
}

void Service::keyPressed(UI_Key key){

    CGuiTree& instance = CGuiTree::getInstance();

    CState currentState;
    int rc = instance.getLastElement( currentState );

    if ( rc != -1)
    {
        switch( currentState.getType() )
        {
        case mainWindow:
        {
            if ( key == keyEnter)
            {
                openMenu();
                instance.append(menuWindow, "Menu");
            }
            break;
        }
        case messangeWindow:
        {
            if ( key == keyEnter)
            {
                instance.delLastElement();
            }
            break;
        }
        // в меню
        case menuWindow:
        {
            // переходим вниз по дереву & запоминаем состояние
            if ( key == keyEnter)
            {
                //instance.append();
            }
            // переходим вверх по дереву & удаляем из стзка последнее состояние
            if ( key == keyBack)
            {
                instance.delLastElement();
                // currentState = carrentState.prevCarrenState;
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
                if ( menu->focus < 3)
                    menu->focus++;
            }
            break;
        }
        default:
            break;
        }
    }
    else
    {
        instance.append(menuWindow, "Menu");
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
    MoonsGeometry area = {0, 0, (GXT)(160), (GYT)(120)};
    if(msg_box == nullptr)
    {
        msg_box = new GUI_Dialog_MsgBox(&area, (char*)text, align);
	}
    msg_box->Draw();
}

void Service::drawMainWindow()
{
    main_scr->Draw();
    indicator->Draw();
}

void Service::openMenu()
{
    Alignment align = {alignHCenter,alignTop};
    const char* title = "MENU";
    const char* text = "texttexttexttexttext";
    if(menu == nullptr)
    {
        menu = new CGuiMenu(&ui_menu_msg_box_area, title, text, align);

    }
    menu->Draw();
}


void Service::draw()
{
    CGuiTree& instance = CGuiTree::getInstance();

    CState currentState;
    int rc = instance.getLastElement(currentState);

    if (rc == 1)
    {
        switch(currentState.getType())
        {
        case mainWindow:
            drawMainWindow();
            break;
        case messangeWindow:
            msgBox( currentState.getName() );
            break;
        case menuWindow:
            openMenu();
            break;
        default:
            break;
        }
    }
    else
    {
        drawMainWindow();
    }
}

}/* namespace Ui */
