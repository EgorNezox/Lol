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

namespace Ui {

MoonsGeometry ui_common_dialog_area={ 0,24,GDISPW-1,GDISPH-1 };

bool Service::single_instance = false; // зависимость от единственного дисплея в системе

Service::Service(matrix_keyboard_t matrixkb_desc, aux_keyboard_t auxkb_desc,
		Headset::Controller *headset_controller,
		Multiradio::MainServiceInterface *mr_main_service, Multiradio::VoiceServiceInterface *mr_voice_service,
		Power::Battery *power_battery) {
	QM_UNUSED(headset_controller);
	QM_UNUSED(mr_main_service);
	QM_UNUSED(power_battery);
	QM_ASSERT(single_instance == false);
	single_instance = true;
	//...
	this->voice_service=mr_voice_service;
	ginit();
	matrix_kb=matrixkb_desc;
	keyboard= new QmMatrixKeyboard(matrix_kb.resource);
	keyboard->keyAction.connect(sigc::mem_fun(this, &Service::keyHandler));
	aux_kb=auxkb_desc;
	chnext_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChNext]);
	chprev_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChPrev]);
	chnext_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chNextHandler));
	chprev_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chPrevHandler));
	main_scr=new GUI_Dialog_MainScr(&ui_common_dialog_area, this);
	main_scr->Draw();
}

Service::~Service() {
	QM_ASSERT(single_instance == true);
	single_instance = false;
	//...
}

void Service::setNotification(NotificationType type) {
	QM_UNUSED(type);
	//...
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

Multiradio::VoiceServiceInterface* Service::pGetVoiceService(){
	return voice_service;
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


void Service::keyPressed(UI_Key key){
	main_scr->keyHandler(key);
}

} /* namespace Ui */
