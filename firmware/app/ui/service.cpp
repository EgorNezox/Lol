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
MoonsGeometry ui_indicator_area={ 0,0,GDISPW-1,23 };

namespace Ui {



bool Service::single_instance = false; // зависимость от единственного дисплея в системе

Service::Service(matrix_keyboard_t matrixkb_desc, aux_keyboard_t auxkb_desc,
		Headset::Controller *headset_controller,
		Multiradio::MainServiceInterface *mr_main_service, Multiradio::VoiceServiceInterface *mr_voice_service,
		Power::Battery *power_battery) {

	QM_ASSERT(single_instance == false);
	single_instance = true;
	//...
	notify_dialog=false;
	msg_box=NULL;

	this->matrix_kb=matrixkb_desc;
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
	main_scr=new GUI_Dialog_MainScr(&ui_common_dialog_area, this);
	main_scr->Draw();
	indicator=new GUI_Indicator(&ui_indicator_area,this);
	indicator->Draw();
	this->headset_controller->statusChanged.connect(sigc::mem_fun(indicator,&GUI_Indicator::UpdateHeadset));
	this->multiradio_service->statusChanged.connect(sigc::mem_fun(indicator,&GUI_Indicator::UpdateMultiradio));
	this->power_battery->chargeLevelChanged.connect(sigc::mem_fun(indicator,&GUI_Indicator::UpdateBattery));
}

Service::~Service() {
	QM_ASSERT(single_instance == true);
	single_instance = false;
	//...
}

void Service::setNotification(NotificationType type) {
	switch(type){
		case NotificationMissingVoiceChannelsTable:
			msgBox(missing_ch_table_txt[getLanguage()]);
			break;
		case NotificationMissingOpenVoiceChannels:
			msgBox(missing_open_ch_txt[getLanguage()]);
			break;
		case NotificationMismatchVoiceChannelsTable:
			msgBox(ch_table_mismatch_txt[getLanguage()]);
			break;
		default:
			QM_ASSERT(0);
			break;
	}
}


void Service::keyHandler(int key_id, QmMatrixKeyboard::PressType pr_type){
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
	if(!notify_dialog){
		main_scr->Draw();
	}
}

void Service::keyPressed(UI_Key key){
	if(notify_dialog){
		msg_box->keyHandler(key);
	}
	else{
		main_scr->keyHandler(key);
	}
}

int Service::getLanguage(){
	return 0;
}

void Service::msgBox(char *text){
	Alignment align={alignHCenter,alignTop};
	if(msg_box==NULL){
		notify_dialog=true;
		msg_box=new GUI_Dialog_MsgBox(&ui_msg_box_area,text,align,this);
		msg_box->Draw();
	}
}

void Service::clearNotification(){
	msg_box=NULL;
	notify_dialog=false;
	main_scr->Draw();
}

} /* namespace Ui */
