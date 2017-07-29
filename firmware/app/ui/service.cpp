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
#include <iostream>
#include <string.h>
#include <string>
#include "../../../system/reset.h"

#define VM_STATE 1
#define VM_PROGRESS 1
#define SMS_PROGRESS 1
#define TIME_ON_GPS_MARKER 0

#define PARAMS_DRAW 1

MoonsGeometry ui_common_dialog_area = { 0,24,GDISPW-1,GDISPH-1 };
MoonsGeometry ui_msg_box_area       = { 20,29,GDISPW-21,GDISPH-11 };
MoonsGeometry ui_menu_msg_box_area  = { 1,1,GDISPW-2,GDISPH-2 };

#if PARAMS_DRAW
    MoonsGeometry ui_indicator_area     = { 0,0,110,23};
#else
    MoonsGeometry ui_indicator_area     = { 0,0,GDISPW-1,23 };
#endif

namespace Ui {

bool Service::single_instance = false;

Service::Service( matrix_keyboard_t                  matrixkb_desc,
                  aux_keyboard_t                     auxkb_desc,
                  Headset::Controller               *headset_controller,
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
    this->voice_service      = mr_voice_service;
    this->power_battery      = power_battery;
    this->headset_controller = headset_controller;
    this->storageFs          = fs;

    ginit();
    loadSheldure();

    voice_service->currentChannelChanged.connect(sigc::mem_fun(this, &Service::voiceChannelChanged));
    voice_service->smsCounterChanged.connect(sigc::mem_fun(this,&Service::onSmsCounterChange));

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
    if (storageFs > 0)
    	menu->setFS(storageFs);

    bool useMode = false;
    if (storageFs > 0)
    	storageFs->getVoiceMode(&useMode);
    menu->useMode = (bool)useMode;
    voice_service->setVoiceMode((Multiradio::VoiceServiceInterface::VoiceMode)!menu->useMode);

    this->voice_service->statusChanged.connect(sigc::mem_fun(this, &Service::updateMultiradio));

    this->power_battery->voltageChanged.connect(sigc::mem_fun(this, &Service::batteryVoltageChanged));
    this->power_battery->chargeLevelChanged.connect(sigc::mem_fun(this, &Service::batteryChargeChanged));

    //this->power_battery->voltageReceived.connect(sigc::mem_fun(this,&Service::onRecievingBatteryVoltage));

    this->voice_service->aleStateChanged.connect(sigc::mem_fun(this, &Service::updateAleState));
    this->voice_service->aleVmProgressUpdated.connect(sigc::mem_fun(this, &Service::updateAleVmProgress));
    this->headset_controller->statusChanged.connect(sigc::mem_fun(this, &Service::updateHeadset));
    this->headset_controller->smartHSStateChanged.connect(sigc::mem_fun(this, &Service::updateHSState));

    voice_service->command_tx30.connect(sigc::mem_fun(this, &Service::TxCondCmdPackage));

    command_rx_30 = 0;


    this->headset_controller->BOOM.connect(sigc::mem_fun(this, &Service::resetLogicDSPforGarniture));

    voice_service->firstPacket.connect(sigc::mem_fun(this,&Service::FirstPacketPSWFRecieved));
    voice_service->smsMess.connect(sigc::mem_fun(this,&Service::smsMessage));
    voice_service->smsFailed.connect(sigc::mem_fun(this,&Service::FailedSms));
    voice_service->respGuc.connect(sigc::mem_fun(this,&Service::gucFrame));
    voice_service->atuMalfunction.connect(sigc::mem_fun(this, &Service::showAtuMalfunction));
    voice_service->dspHardwareFailed.connect(sigc::mem_fun(this, &Service::showDspHardwareFailure));
    voice_service->messageGucTxQuit.connect(sigc::mem_fun(this, &Service::msgGucTXQuit));
    voice_service->gucCrcFailed.connect(sigc::mem_fun(this,&Service::errorGucCrc));
    voice_service->gucCoord.connect(sigc::mem_fun(this,&Service::GucCoord));
    voice_service->startRxQuitSignal.connect(sigc::mem_fun(this, &Service::startRxQuit));
    voice_service->stationModeIsCompleted.connect(sigc::mem_fun(this,&Service::onCompletedStationMode));
    voice_service->dspStarted.connect(sigc::mem_fun(this,&Service::onDspStarted));

    voice_service->waveInfoRecieved.connect(sigc::mem_fun(this,&Service::onWaveInfoRecieved));

    voice_service->rxModeSetting.connect(sigc::mem_fun(this,&Service::onRxModeSetting));
    voice_service->txModeSetting.connect(sigc::mem_fun(this,&Service::onTxModeSetting));

    voice_service->settingAleFreq.connect(sigc::mem_fun(this,&Service::onSettingAleFreq));

    voice_service->startCondReceiving.connect(sigc::mem_fun(this,&Service::onStartCondReceiving));
    voice_service->virtualCounterChanged.connect(sigc::mem_fun(this,&Service::onVirtualCounterChanged));

    voice_service->qwitCounterChanged.connect(sigc::mem_fun(this,&Service::onQwitCounterChanged));

    voice_service->transmitAsk.connect(sigc::mem_fun(this,&Service::onTransmitAsk));

    valueRxSms = 0;


    pswf_status = false;
 #if defined (PORT__TARGET_DEVICE_REV1)
 #endif

#ifndef PORT__PCSIMULATOR
    navigator->PswfSignal.connect(sigc::mem_fun(this,&Service::setPswfStatus));

#if TIME_ON_GPS_MARKER
    navigator->syncPulse.connect(sigc::mem_fun(this,&Service::updateSystemTime));
#else
    systemTimeTimer = new QmTimer(true); // TODO:
    systemTimeTimer->setInterval(1000);
    systemTimeTimer->setSingleShot(false);
    systemTimeTimer->start();
    systemTimeTimer->timeout.connect(sigc::mem_fun(this, &Service::updateSystemTime));
#endif

#endif

    menu->supressStatus = 0;

    synchModeTimer.setSingleShot(true);
    synchModeTimer.timeout.connect(sigc::mem_fun(this, &Service::readSynchMode));
    synchModeTimer.start(1000);

    testMsgTimer.setSingleShot(true);
    testMsgTimer.timeout.connect(sigc::mem_fun(this, &Service::onTestMsgTimer));
    testMsgTimer.start(1500);

    currentSpeed = voice_service->getCurrentChannelSpeed();

    draw();
}

void Service::resetLogicDSPforGarniture()
{
     voice_service->resetDSPLogic();
}

void Service::onTestMsgTimer()
{
    isStartTestMsg = false;
    draw();
}

void Service::readSynchMode()
{
    if (storageFs > 0){
       storageFs->getGpsSynchroMode((uint8_t*)&gpsSynchronization);
       voice_service->setVirtualMode(!gpsSynchronization);
    }
}

void Service::startRxQuit()
{
	isGucAnswerWaiting = true;
	draw();
}

void Service::setPswfStatus(bool var)
{
		pswf_status = var;
}

void Service::showAtuMalfunction()
{
    msgBox(atumalfunction_title_str, atumalfunction_text_str);
}

void Service::showDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code)
{
	std::string title, text;
	if ((subdevice_code == 7) && (error_code == 5)) {
		title = dsphardwarefailure_7_5_title_str;
		text = dsphardwarefailure_7_5_text_str;
	} else {
		title = dsphardwarefailure_unknown_title_str;
		char text_buffer[50];
		sprintf(text_buffer , dsphardwarefailure_unknown_text_str, subdevice_code, error_code);
        text = text_buffer;
	}
	msgBox(title.c_str(), text.c_str());
}

void Service::errorGucCrc()
{
    msgBox( "Error ", "Crc error\0");
}

void Service::GucCoord(){
    uint8_t *mes;
    mes = voice_service->requestGucCoord();
    char str[9];
    //	for(int i = 0; i<=8 ;i++) {
    //		int a = static_cast<int>(mes[i]);
    //		sprintf(str,"%d",a);
    //	}
    //	str[9] = '\0';
    //	msgBox( "Coord", str);
    //	guiTree.append(messangeWindow,str, "0\0");
}

void Service::updateHeadset(Headset::Controller::Status status)
{
    checkHeadsetStatus();

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

    drawIndicator();
}

void Service::updateMultiradio(Multiradio::VoiceServiceInterface::Status status)
{
    multiradioStatus = status;
    if (multiradioStatus == Multiradio::VoiceServiceInterface::Status::StatusVoiceTx)
    	voice_service->setSwrTimerState(true);
    else
    	voice_service->setSwrTimerState(false);
    drawIndicator();
    drawWaveInfo();
}

void Service::setFreqLabelValue(int value)
{
    voice_service->saveFreq(value);
}

void Service::batteryChargeChanged(int newCharge)
{
	updateBattery();
}

void Service::batteryVoltageChanged(int newVoltage)
{
	voice_service->sendBatteryVoltage(newVoltage);
}

void Service::updateBattery()
{
    drawIndicator();
}

void Service::drawIndicator()
{
    if (!isStartTestMsg){
    static uint8_t gpsStatus = 0; //none
	if ( guiTree.getCurrentState().getType() == mainWindow && msg_box == nullptr){
		if (navigator != 0)
		{
			Navigation::Coord_Date date = navigator->getCoordDate();
            uint8_t gpsStatusNew = date.status;
            if (gpsStatusNew && !isValidGpsTime && isDspStarted)
				updateSessionTimeSchedule();
            if (gpsStatus == 0 && gpsStatusNew == 1)
                gpsStatus = 2; //unlock
            else if (gpsStatus == 2 && gpsStatusNew == 0)
                gpsStatus = 1; //lock
            else if (gpsStatus == 1 && gpsStatusNew == 1)
                gpsStatus = 2; //unlock
		}
		indicator->UpdateGpsStatus(gpsStatus);
        indicator->UpdateBattery(pGetPowerBattery()->getChargeLevel());
        indicator->UpdateHeadset(pGetHeadsetController()->getStatus());
        indicator->UpdateMultiradio(pGetVoiceService()->getStatus());
        indicator->Draw();


		MoonsGeometry objArea = {  0, 0, 159, 127 };
		MoonsGeometry batArea   = {  70, 29,  90, 41 };

		int charge = pGetPowerBattery()->getChargeLevel();
		//charge = 100;

		char var[4] = {0,0,0,0};
		sprintf(var,"%03i",charge);
		std::string chargeStr(var);

		GUI_Obj obj(&objArea);
		GUI_EL_Label  batLabel  (&GUI_EL_TEMP_LabelTitle,    &batArea,   (char*)chargeStr.c_str(), (GUI_Obj *)&obj);

		MoonsGeometry windowArea = {  70, 29, 90, 41 };
		GUI_EL_Window window     (&GUI_EL_TEMP_WindowGeneral, &windowArea,                         (GUI_Obj *)&obj);

		window.Draw();
		batLabel.Draw();
	}
    }
}

void Service::FailedSms(int stage)
{
    switch(stage){
		case -1: { msgBox( rxtxFiledSmsStr[0], EndSms);         break; }
		case  0: { msgBox( rxtxFiledSmsStr[0], sms_quit_fail1); break; }
		case  1: { msgBox( rxtxFiledSmsStr[0], sms_quit_fail2); break; }
		case  2: { msgBox( rxtxFiledSmsStr[0], EndSms2);        break; }
		case  3: { msgBox( rxtxFiledSmsStr[1], sms_crc_fail);   break; }
    }
    failFlag = true;
    menu->virtCounter = 0;
}

void Service::setColorScheme(uint32_t back,uint32_t front)
{
         GENERAL_TEXT_COLOR =				front;
         GENERAL_FORE_COLOR =				front;
         GENERAL_BACK_COLOR =				back;
         MENU_ITEM_INACTIVE_BACK_COLOR =	back;
         MENU_ITEM_INACTIVE_TEXT_COLOR =	front;
         MENU_ITEM_ACTIVE_BACK_COLOR =	    front;
         MENU_ITEM_ACTIVE_TEXT_COLOR =	    back;
         SPBOX_ACTIVE_BACK_COLOR =		    front;
         SPBOX_ACTIVE_TEXT_COLOR =		    back;
         SPBOX_INACTIVE_BACK_COLOR =		back;
         SPBOX_INACTIVE_TEXT_COLOR =		front;
         BATTERY_HIGH_COLOR =				front;
         BATTERY_MID_COLOR =				front;
         BATTERY_LOW_COLOR =				front;
}

Service::~Service() {
    QM_ASSERT(single_instance == true);
    single_instance = false;

    delete menu;
    delete msg_box;
    delete keyboard;
    delete chnext_bt;
    delete chprev_bt;
    delete main_scr;
    delete indicator;
    fileMsg.clear();
}

void Service::setNotification(NotificationType type)
{
    switch(type)
    {
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
    QM_UNUSED(pr_type);
    keyPressed((UI_Key)matrix_kb.key_id[key_id]);
}

Headset::Controller * Service::pGetHeadsetController(){
    return headset_controller;
}

Multiradio::VoiceServiceInterface* Service::pGetVoiceService()
{
    return voice_service;
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

void Service::checkHeadsetStatus()
{
    //  0 - skzi open
    //  1 - polev open
    //  2 - skzi close

    uint8_t headsetType = 0;
    bool chMiss = false;
    if (pGetHeadsetController()->getAnalogStatus(chMiss)){
      headsetType = 1;
      voice_service->sendHeadsetType(headsetType);
    }
    else{
      headsetType = (uint8_t)voice_service->getCurrentChannelType(); // 1 - open 2 - close
      if (headsetType)// not invalid
      {
          if (headsetType == 1)
              headsetType = 0;
          voice_service->sendHeadsetType(headsetType);
      }
    }
}

void Service::voiceChannelChanged()
{
    checkHeadsetStatus();
    char mas[9];
    sprintf(mas,"%d",voice_service->getCurrentChannelFrequency());
    mas[8] = '\0';
    main_scr->oFreq.clear(); main_scr->oFreq.append(mas);
    main_scr->setFreq(mas);
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
                if (main_scr->nFreq.size() > 0)
                {
                    main_scr->nFreq.pop_back();
                    main_scr->setFreq(main_scr->nFreq.c_str());
                }
                else
                {
                    main_scr->mwFocus = -2;
                    main_scr->setFocus(1-main_scr->mwFocus);
                    main_scr->editing = false;
                    main_scr->setFreq(main_scr->oFreq.c_str());
                }
                break;
            case keyEnter:
                if (main_scr->mwFocus == 0)
                {
                    main_scr->mwFocus = -2;
                    main_scr->setFocus(1-main_scr->mwFocus);
                    main_scr->editing = false;
                    main_scr->oFreq.clear();
                    main_scr->oFreq.append(main_scr->nFreq.c_str());
                    int freq = atoi(main_scr->nFreq.c_str());
                    voice_service->tuneFrequency(freq, true);
                }

                break;
            case keyLeft:
                if (main_scr->mwFocus == 1 && main_scr->mainWindowModeId > 0)
                {
                    main_scr->mainWindowModeId--;
                    this->voice_service->setVoiceMode(Multiradio::VoiceServiceInterface::VoiceMode(main_scr->mainWindowModeId));
                }
                break;
            case keyRight:
                if (main_scr->mwFocus == 1 && main_scr->mainWindowModeId < 1)
                {
                    main_scr->mainWindowModeId++;
                    this->voice_service->setVoiceMode(Multiradio::VoiceServiceInterface::VoiceMode(main_scr->mainWindowModeId));
                }
                break;
            default:
                if ( main_scr->mwFocus == 0 )
                    main_scr->editingFreq(key);
                break;
            }
        }
        else if (main_scr->channelEditing)
        {
        	if (key >= key0 && key <= key9) // 1 - 98
        	{
        		if (channelNumberSyms == 0)
        		{
        			channelNumberEditing += key - 6;
        			if (key > key0 )
        				channelNumberSyms = 1;
        		}
        		else if (channelNumberSyms == 1)
        		{
        			if (not (channelNumberEditing == 9 && key == key9) )
        			{
						channelNumberEditing *= 10;
						channelNumberEditing += key - 6;
						channelNumberSyms = 2;
        			}
        		}
        	}
        	if (key == keyEnter)
        	{
        		if (channelNumberEditing > 0 && channelNumberEditing < 99)
        		{
        			headset_controller->setChannel(channelNumberEditing);
        			channelNumberEditing = 0;
        			channelNumberSyms = 0;
        		    main_scr->channelEditing = false;
        			main_scr->mwFocus = -2;
        			main_scr->setFocus(1-main_scr->mwFocus);
        		}
        	}
        	if (key == keyBack)
        	{
        		if (channelNumberSyms == 2)
        		{
       				channelNumberEditing /= 10;
        			channelNumberSyms = 1;
        		}
        		else if (channelNumberSyms == 1)
        		{
        			channelNumberEditing = 0;
        			channelNumberSyms = 0;
        		}
        		else if (channelNumberSyms == 0)
        		{
        			channelNumberEditing = 0;
        		    main_scr->channelEditing = false;
        			main_scr->mwFocus = -2;
        			main_scr->setFocus(1-main_scr->mwFocus);
        		}
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
                if (main_scr->mwFocus > -2)
                    main_scr->mwFocus--;
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
                if (main_scr->mwFocus == -2)
                    guiTree.advance(0);
                if (main_scr->mwFocus == -1)
                {
                    if (this->voice_service->getVoiceMode() == Multiradio::VoiceServiceInterface::VoiceModeManual)
                    main_scr->channelEditing = true;
                    oldChannelNumber = voice_service->getCurrentChannelNumber();
                }
                if (main_scr->mwFocus >= 0)
                {
                    if (this->voice_service->getVoiceMode() == Multiradio::VoiceServiceInterface::VoiceModeManual)
                        main_scr->editing = true;

                    if (main_scr->mwFocus == 0)
                        main_scr->nFreq.clear();
                }
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
            if (isDrawCondCmd)
        	    isDrawCondCmd = false;
            //draw();
            if (msg_box != nullptr)
            {
                delete msg_box;
                msg_box = nullptr;
            }

            vect = nullptr;
            position = 1;
            if (isCondModeQwitTx || isGucModeQwitTx)
            {
            	//guiTree.backvard();
            }
            else
            {
            	guiTree.resetCurrentState();
            }
        }
        else
        {
            if (vect != nullptr)
            {
            	if (position == 0)
            		position = 1;
                if (key == keyUp && position > 1)
                	position--;
                if (key == keyDown && position < vect[0])
                	position++;

                msg_box->setCmd(vect[position]);
                msg_box->keyPressed(key);
            }
        }
        break;
    }
        // ? пїЅ ? пїЅ? пїЅ? пїЅСЋ
    case menuWindow:
    {
        if ( key == keyEnter)
        {
            guiTree.advance(menu->focus);
            menu->isNeedClearWindow  = true;
            menu->oldFocus = menu->focus;
            menu->focus = 0;
            menu->offset = 0;

            auto type = guiTree.getCurrentState().getType();
            if ( type == GuiWindowTypes::endMenuWindow )
            {
                CEndState estate = (CEndState&)guiTree.getCurrentState();
                if ( estate.subType == GuiWindowsSubType::setSpeed )
                {
                    currentSpeed = /*Multiradio::voice_channel_speed_t(4);*/voice_service->getCurrentChannelSpeed();
                }
                else if (estate.subType == GuiWindowsSubType::voiceMode)
                {
                    if (voice_service->getVoiceMode() == Multiradio::VoiceServiceInterface::VoiceModeAuto)
                        menu->useMode = true;
                    else
                        menu->useMode = false;
                }
                else if (estate.subType == GuiWindowsSubType::channelEmissionType)
                {
                    if ( voice_service->getCurrentChannelEmissionType() == Multiradio::voiceemissionFM)
                        menu->ch_emiss_type = true;
                    else
                        menu->ch_emiss_type = false;
                }
            }

        }
        if ( key == keyBack)
        {
            guiTree.backvard();
              menu->isNeedClearWindow  = true;
              menu->oldFocus = menu->focus;
              menu->focus = 0;
              menu->offset = 0;
        }
        if (key == keyUp)
        {
            if ( menu->focus > 0 ){
             menu->oldFocus = menu->focus;
                menu->focus--;
            }
        }
        if (key == keyDown)
        {
            if ( state.nextState.size() != 0 )
            {
                if ( menu->focus < state.nextState.size()-1 )
                {
                    menu->oldFocus = menu->focus;
                    menu->focus++;
                }
            }
        }

        menu->oldOffset = menu->offset;

        if (menu->focus + 1 == menu->offset)
        	menu->offset = menu->focus;
        else if (menu->focus - 3 == menu->offset)
        	menu->offset++;

        menu->keyPressed(key);
        break;
    }
    case endMenuWindow:
    {
        CEndState estate = (CEndState&)guiTree.getCurrentState();

        switch(estate.subType)
        {
        case GuiWindowsSubType::condCommand:
        {
            //[0] - CMD, [1] - R_ADDR, [2] - retrans
            switch (menu->txCondCmdStage)
            {
            case 0:
            {
                if (key == keyLeft)
                {
                	if (menu->condCmdModeSelect > 0)
                		menu->condCmdModeSelect--;
                	else
                	{
                        guiTree.backvard();
                        onCompletedStationMode();
                	}
                }
                if (key == keyRight)
                {
                	if (menu->condCmdModeSelect < 2)
                		menu->condCmdModeSelect++;
                }
                break;
            }
            case 1:
            {
                if ( estate.subType == condCommand && estate.listItem.size() == 3)
                {
                    if (key == keyLeft || key == keyRight)
                    { menu->useCmdRetrans = menu->useCmdRetrans ? false : true; }
                }
                else
                {
                    menu->useCmdRetrans = false;
                }
                break;
            }
            case 2:
            case 3:
            case 4:
            {
                if ( key > 5 && key < 16)
                {
                    auto iter = estate.listItem.begin();
                    if (menu->txCondCmdStage != 4)
                    {
                    	(*iter)++; if (menu->txCondCmdStage == 3) (*iter)++;
                    }
                    if ( (*iter)->inputStr.size() < 2 )
                    {
                        (*iter)->inputStr.push_back((char)(42+key));
                        // check
                        int rc = atoi((*iter)->inputStr.c_str());
                        uint8_t num = menu->txCondCmdStage == 4 ? 99 : 31;
                        if ( rc > num )
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
                int size = 6;

                // next field
                if (menu->txCondCmdStage <= size )
                {
                    // select mode

                    // group
                    if (menu->txCondCmdStage == 0 && menu->condCmdModeSelect == 0)
                    {
                        menu->txCondCmdStage = 4;
                        auto iter = estate.listItem.begin();
                        for (uint8_t i = 0; i < 2; i++)
                        {
                            (*iter)++;
                            (*iter)->inputStr.clear();
                            (*iter)->inputStr.push_back((char)(42+key0));
                            (*iter)->inputStr.push_back((char)(42+key0));
                        }
                        break;
                    }

                    // indiv
                    if (menu->txCondCmdStage == 0 && menu->condCmdModeSelect == 1)
                    {
                        menu->txCondCmdStage = 1;
                        break;
                    }

                    // ticket
                    if (menu->txCondCmdStage == 0 && menu->condCmdModeSelect == 2)
                    {
                        menu->txCondCmdStage = 3;
                        auto iter = estate.listItem.begin();
                        (*iter)++;
                        (*iter)->inputStr.clear();
                        (*iter)->inputStr.push_back((char)(42+key0));
                        (*iter)->inputStr.push_back((char)(42+key0));
                        break;
                    }

                    // use retrans ?
                    if (menu->txCondCmdStage == 1 && menu->condCmdModeSelect == 1)
                    {
                        auto iter = estate.listItem.begin();
                        (*iter)++;
                        (*iter)->inputStr.clear();

                        if ( menu->useCmdRetrans )
                        {
                            menu->txCondCmdStage++;
                        }
                        else
                        {
                            (*iter)->inputStr.push_back((char)(42+key0));
                            (*iter)->inputStr.push_back((char)(42+key0));
                            menu->txCondCmdStage = 3;
                        }
                        break;
                    }

                    auto iter = estate .listItem.begin();

                    switch (menu->txCondCmdStage){
                        case 2: (*iter)++;            if (!((*iter)->inputStr == ""))    menu->txCondCmdStage++; break;
                        case 3: (*iter)++; (*iter)++; if ((*iter)->inputStr.size() != 0) menu->txCondCmdStage++; break;
                        case 4: if ((*iter)->inputStr.size() != 0) menu->txCondCmdStage++; break;
                        case 5: menu->txCondCmdStage++; break;
                        case 6:
                        {
                        	menu->txCondCmdStage = 0;
                            guiTree.backvard();
                            onCompletedStationMode();
                        	break;
                        }
                    }
                }

                // send
                if ( menu->txCondCmdStage == 6 )
                {
#ifndef _DEBUG_

                    // [0] - cmd, [1] - raddr, [2] - retrans
                    // condCmdModeSelect, 1 - individ, 2 - quit
                    int param[3] = {0,0,0}, i = 0;
                    for(auto &k: estate.listItem){
                        param[i] = atoi(k->inputStr.c_str());
                        i++;
                    }
                    setAsk = false;

                    if ((storageFs > 0) && (param[0] != 0))
                    {
                        char sym[4];
                        sprintf(sym,"%d",param[0]);
                        if (param[0] < 10) sym[1] = 0;
                        sym[2] = 0;
                        fileMsg.clear();
                        fileMsg.push_back((uint8_t)sym[0]);
                        fileMsg.push_back((uint8_t)sym[1]);
                        fileMsg.push_back((uint8_t)sym[2]);

                        GUI_Painter::ClearViewPort();
                        showMessage(waitingStr, flashProcessingStr, promptArea);
                        storageFs->writeMessage(DataStorage::FS::FT_CND, DataStorage::FS::TFT_TX, &fileMsg);
                        draw();
                    }

                    menu->virtCounter = 0;

                    if (menu->condCmdModeSelect == 0)
                        voice_service->TurnPSWFMode(0, param[0], 0,0); // групповой вызов
                    if (menu->condCmdModeSelect == 1)
                        voice_service->TurnPSWFMode(0, param[0], param[2],param[1]); // индивидуальный вызов
                    if (menu->condCmdModeSelect == 2){
                        param[2] +=32;
                        voice_service->TurnPSWFMode(1,param[0],param[2],0); // с квитанцией
                        setAsk = true;
                    }

                    for(auto &k: estate.listItem)
                        k->inputStr.clear();


#else
                    menu->txCondCmdStage = 0;
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

                switch(menu->txCondCmdStage)
                {
					case 0:
					{
						guiTree.backvard();
						onCompletedStationMode();
						break;
					}
					case 1:
					{
						 menu->txCondCmdStage--;
						 break;
					}
					case 2:
					{
		                   (*iter)++;
		                    if ((*iter)->inputStr.size() > 0)
		                        (*iter)->inputStr.pop_back();
		                    else
		                        menu->txCondCmdStage = 1;

		                    break;
					}
					case 3:
					{
	                    (*iter)++;(*iter)++;
	                    if ((*iter)->inputStr.size() > 0)
	                        (*iter)->inputStr.pop_back();
	                    else
	                    {
	                        if ( menu->condCmdModeSelect == 2)
	                            menu->txCondCmdStage = 0;

	                        if (menu->useCmdRetrans)
	                            menu->txCondCmdStage--;
	                        else
	                            menu->txCondCmdStage = 0;
	                    }
	                    break;
					}
					case 4:
					{
	                    // CMD
	                    if ((*iter)->inputStr.size() > 0)
	                        (*iter)->inputStr.pop_back();
	                    else
	                    {
	                        if ( menu->condCmdModeSelect == 0 )
	                        {
	                            for(auto &k: estate.listItem)
	                            {
	                                k->inputStr.clear();
	                            }
	                            menu->txCondCmdStage = 0;
	                        }
	                        else
	                        {
	                            menu->txCondCmdStage--;
	                        }
	                    }
	                    break;
					}
					case 5:
					{
	                    menu->txCondCmdStage--;
	                    break;
					}
					case 6:
					{
						break;
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
            //static bool isDefaultFreq = false; //freq from main screen
            switch (menu->groupCondCommStage)
            {
                case 0:     // use coordinate ?
                {
                    if ( key == keyBack )
                    {
                        for (auto &k: estate.listItem)
                            k->inputStr.clear();
                        guiTree.backvard();
                        menu->focus = 0;
                        if (pGetHeadsetController()->getStatus() ==  Headset::Controller::Status::StatusSmartOk){
                        	setFreq();
                        }
                        isTurnGuc = false;
                        onCompletedStationMode(true);
                    }

                    if ( key == keyEnter )
                    {
                        menu->groupCondCommStage += 2;
                        //isDefaultFreq = true;
                    }

                    if ( key == keyLeft || key == keyRight )
                    {
                        menu->useSndCoord = not menu->useSndCoord;
                    }
                }
                break;
                case 2:     // group vs. indiv.
                {
                    if ( key == keyBack )
                    {
                        menu->groupCondCommStage -= 2;
                    }

                    if ( key == keyEnter )
                    {
                        menu->groupCondCommStage++;     // одному
                        if(menu->sndMode)
                            menu->groupCondCommStage++; // всем
                    }

                    if ( key == keyLeft || key == keyRight )
                    {
                        menu->sndMode = !menu->sndMode;
                    }
                }
                break;
                case 3:     // set address
                {
                    std::string* address;
                    (*iter)++;
                    address = &(*iter)->inputStr;

                    if ( key == keyBack )
                    {
                        if(address->size() > 0)
                        {
                            address->pop_back();
                        }else
                            menu->groupCondCommStage--;
                    }

                    if ( key == keyEnter )
                    {
                        if(address->size() > 0)
                            menu->groupCondCommStage++;
                    }

                    if ( key >= key0 && key <= key9 )
                    {
                        address->push_back( (char)(key + 42) );
                        if ( atoi(address->c_str()) > 31)
                            address->clear();
                    }

                }
                break;
                case 4:     // set command
                {
                    std::string* commands;
                    (*iter)++; (*iter)++;
                    commands = &(*iter)->inputStr;

                    if ( key == keyBack )
                    {
                        if(commands->size() > 0)
                        {
                            if(commands->size() %3 == 1)
                                menu->cmdCount--;

                            if( commands->size() > 2 && commands->back() == ' ' )
                            {
                                commands->pop_back();
                            }
                            commands->pop_back();
                        }else
                        {
                            menu->cmdCount = 0;
                            menu->groupCondCommStage--;     // одному
                                if(menu->sndMode)
                                    menu->groupCondCommStage--; // всем
                        }
                    }
                    if ( key == keyEnter )
                    {
                        if(commands->size() > 0)
                            menu->groupCondCommStage++;
                    }
                    if ( key == keyUp )
                    {
                        if(menu->cmdScrollIndex > 0) menu->cmdScrollIndex--;
                    }

                    if ( key == keyDown )
                    {
                        menu->cmdScrollIndex++;
                    }

                    if ( key >= key0 && key <= key9 )
                    {
                        if (commands->size() < 299 && menu->cmdCount < 101 )
                        {
                            commands->push_back( (char)(key + 42) );
                            if(commands->size() > 0 && commands->size() %3 == 1)
                                menu->cmdCount++;

                            if(commands->size() > 0 && (commands->size()+1) %3 == 0)
                            {
                                commands->push_back((char) ' ');
                            }
                                menu->cmdScrollIndex += 20;
                        }
                    }
                }
                break;
                case 5:     // start
                {
                    if ( key == keyBack )
                    {
                        menu->groupCondCommStage--;
                    }
                    if ( key == keyEnter )
                    {
                    	menu->groupCondCommStage++;
#ifndef PORT__PCSIMULATOR
                        int mas[4];
                        int i = 0;
                        const char * str;
                        for (auto &k: estate.listItem)
                        {
                            mas[i] = atoi(k->inputStr.c_str());
                            if (i == 2) str = k->inputStr.c_str();
                            i++;
                        }
                        int freqs;
                        int r_adr = mas[1];
                        freqs = mas[0];
                        int speed = 0;//atoi(mas[1]);
                        guc_command_vector.clear();

                        parsingGucCommand((uint8_t*)str);
                        std::string dataStr(str);
                        uint8_t size = dataStr.size();
                        fileMsg.clear();
                        fileMsg.resize(size + 1);
                        fileMsg[size] = 0;
                        memcpy(fileMsg.data(), &dataStr[0], size);

                        if (storageFs > 0)
                        {
                        	GUI_Painter::ClearViewPort();
                            showMessage(waitingStr, flashProcessingStr, promptArea);
                            storageFs->writeMessage(DataStorage::FS::FT_GRP, DataStorage::FS::TFT_TX, &fileMsg );
                            draw();
                        }

                        voice_service->saveFreq(freqs);
                        voice_service->TurnGuc(r_adr,speed,guc_command_vector,menu->useSndCoord);
                        isTurnGuc = true;

#else
                        for (auto &k: estate.listItem)
                            k->inputStr.clear();
                        menu->groupCondCommStage = 0;
                        guiTree.resetCurrentState();
#endif

//                        std::list<SInputItemParameters*>::iterator iterClr = estate.listItem.begin();
//                        std::string* strClr;
//                        strClr = &(*iterClr)->inputStr;
//                        strClr->clear(); //clear freq

//                        iterClr = estate.listItem.begin();
//                        (*iterClr)++;
//                        strClr = &(*iterClr)->inputStr;
//                        strClr->clear(); //clear address

//                        iterClr = estate.listItem.begin();
//                        (*iterClr)++;
//                        strClr = &(*iterClr)->inputStr;
//                        strClr->clear(); //clear cmd's
//                        menu->cmdCount = 0;
//                        menu->cmdScrollIndex = 0;
                    }
                    break;
                }
                case 6:     // ...
                {
                	if ( key == keyBack )
                	{
                		menu->groupCondCommStage--;
                	}
                	if ( key == keyEnter )
                	{
                		menu->groupCondCommStage = 0;
                		guiTree.resetCurrentState();
                		isTurnGuc = false;
                        onCompletedStationMode(true);
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
                    menu->inVoiceMail = true;
                }
                if (key == keyBack)
                {
                    if (menu->channalNum.size() > 0)
                        menu->channalNum.pop_back();
                    else
                    {
                        guiTree.backvard();
                        menu->offset = 1;
                        menu->focus = 2;
                        menu->inVoiceMail = false;
                        menu->toVoiceMail = false;
                        onCompletedStationMode();
                    }
                }
#ifdef _DEBUG_
                if (key == keyEnter)
                {
                    if (menu->channalNum.size() > 0){
                        menu->putOffVoiceStatus++;
                        menu->inVoiceMail = true;
                    }
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
                    voice_service->stopAle();
                    menu->putOffVoiceStatus--;
                    headset_controller->stopSmartRecord();
                }
#ifndef _DEBUG_
                if (key == keyEnter) // && STATUS OK
                {
                    headset_controller->stopSmartRecord();
                    Headset::Controller::SmartHSState smartState = headset_controller->getSmartHSState();

                    if ( smartState == headset_controller->SmartHSState_SMART_READY )
                        menu->putOffVoiceStatus++;
                    else if ( smartState == headset_controller->SmartHSState_SMART_RECORD_TIMEOUT ||\
                              smartState == headset_controller->SmartHSState_SMART_EMPTY_MESSAGE  ||\
                              smartState == headset_controller->SmartHSState_SMART_ERROR
                              )
                    {
                        voice_service->stopAle();
                        menu->putOffVoiceStatus = 1;
                        menu->voiceAddr.clear();
                        menu->channalNum.clear();
                        menu->focus = 0;
                        //onCompletedStationMode();
                        guiTree.resetCurrentState();
                    }
                    // repeat
//                    else if (smartState == headset_controller->SmartHSState_SMART_NOT_CONNECTED ||\
//                             smartState == headset_controller->SmartHSState_SMART_BAD_CHANNEL ||\
//                             smartState == headset_controller->SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL ||\
//                             smartState == headset_controller->SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE ||\
//                             smartState == headset_controller->SmartHSState_SMART_RECORD_DOWNLOADING ||\
//                             smartState == headset_controller->SmartHSState_SMART_PLAYING ||\
//                             smartState == headset_controller->SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL ||\
//                             smartState == headset_controller->SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE ||\
//                             smartState == headset_controller->SmartHSState_SMART_RECORDING ||\
//                             smartState == headset_controller->SmartHSState_SMART_RECORD_UPLOADING
//                             )
//                    {}
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
            {// ���� ������ ����������
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
                    updateAleState(AleState_IDLE);
                    Multiradio::voice_message_t message = headset_controller->getRecordedSmartMessage();
                    if (storageFs > 0)
                    {
                    	GUI_Painter::ClearViewPort();
                        showMessage(waitingStr, flashProcessingStr, promptArea);
                        storageFs->writeMessage(DataStorage::FS::FT_VM, DataStorage::FS::TFT_TX, &message);
                        menu->toVoiceMail = false;
                        draw();
                    }
                    voice_service->startAleTx((uint8_t)atoi(menu->voiceAddr.c_str()),message);
                    //Запись во флеш


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
                    voice_service->stopAle();
                    //onCompletedStationMode();
#endif
                }
                if (key == keyEnter /*&& voice_service->getAleState() == */)
                {
#ifndef _DEBUG_
                    voice_service->stopAle();
#endif
                    menu->putOffVoiceStatus = 1;
                    menu->voiceAddr.clear();
                    menu->channalNum.clear();
#ifndef _DEBUG_
                    menu->focus = 0;
                    guiTree.resetCurrentState();
                    menu->inVoiceMail = false;
                    menu->toVoiceMail = false;
                    onCompletedStationMode();
#endif
                    guiTree.resetCurrentState();
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
        case GuiWindowsSubType::txSmsMessage:
        {
            if (!isSmsMessageRec){
            switch (menu->smsTxStage)
            {
            case 1:
            {
                switch (key)
                {
                case keyBack:
                {
                    guiTree.backvard();
                    menu->offset = 0;
                    menu->focus = 1;
                    onCompletedStationMode();
					menu->virtCounter = 0;
                    break;
                }
                case keyLeft:
                case keyRight:
                {
                    menu->useSmsRetrans = !menu->useSmsRetrans;
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
                    if ((*iter)->inputStr.size() > 0){
                        (*iter)->inputStr.pop_back();
                        menu->smsScrollIndex += 20;
                    }
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
                case keyUp:
                {
                    if(menu->smsScrollIndex > 0)
                        menu->smsScrollIndex--;
                    break;
                }
                case keyDown:
                {
                        menu->smsScrollIndex++;
                    break;
                }
                default:
                {
                    if ((*iter)->inputStr.size() < 101){
                        menu->inputSmsMessage( &(*iter)->inputStr, key );
                        menu->smsScrollIndex++;
                    }
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

                    if ( menu->smsStage == 0xF0 )
                    {
                        menu->smsStage = 0;
                        menu->smsTxStage = 1;
                        guiTree.resetCurrentState();
    					menu->virtCounter = 0;
                    }
                    else if (menu->smsStage == 0x0F){   /*menu->smsStage = 0;*/  }
                    else
                    {
                        // call
                        // [0] - dstAddr, [1]- message, [3] - retrAddr
                        auto iter = estate.listItem.begin();
                        auto dstAddr = (*iter)->inputStr;
                        (*iter)++;
                        auto msg = (*iter)->inputStr;
                        (*iter)++; (*iter)++;
                        auto retrAddr = (*iter)->inputStr;
                        int param[3] = {0,0,0};
                        int i = 0;
                        for(auto &k: estate.listItem)
                        {
                            param[i] = atoi(k->inputStr.c_str());
                            i++;
                        }

                        if (navigator != 0){
                            Navigation::Coord_Date date = navigator->getCoordDate();

                            char ch[4]; memcpy(ch, date.data, 4);

                            if (atoi(ch) > 0)
                            {
                                if (storageFs > 0){
                                    fileMsg.clear();
                                    fileMsg.resize(msg.size());
                                    memcpy(fileMsg.data(), &msg[0], msg.size());
                                    GUI_Painter::ClearViewPort();
                                    showMessage(waitingStr, flashProcessingStr, promptArea);
                                    storageFs->writeMessage(DataStorage::FS::FT_SMS, DataStorage::FS::TFT_TX, &fileMsg);
                                    menu->virtCounter = 0;
                                    draw();
                                }

                                voice_service->defaultSMSTrans();
                                failFlag = false;

            					menu->virtCounter = 0;
                                if (param[2] > 0)
                                    voice_service->TurnSMSMode(param[2], (char*)msg.c_str(),atoi(dstAddr.c_str())); //retr,msg,radr
                                else
                                    voice_service->TurnSMSMode(atoi(dstAddr.c_str()), (char*)msg.c_str(),0);

                                for(auto &k: estate.listItem)
                                    k->inputStr.clear();
                                menu->smsTxStage++;
                                //guiTree.resetCurrentState();

                            }
                        }
                    }

                    break;
                }
                default:{break;}
                }
                break;
            }
            case 6:
            {
            case keyEnter:
            {
                menu->smsTxStage = 1;
                guiTree.resetCurrentState();
                onCompletedStationMode();
				menu->virtCounter = 0;
            }
            }
            default:
            {break;}
            }

            break;
            }
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
            if ( key == keyBack)
            {
            	switch (menu->recvStage)
            	{
					case 0:
					case 3:
					{
						isStartCond = false;
						guiTree.resetCurrentState();
						//guiTree.backvard();
						onCompletedStationMode();
						menu->virtCounter = 0;
						menu->recvStage = 0;
						break;
					}
					case 1:
					case 2:
					{
						menu->recvStage--;
						break;
					}
            	}
            }

            if ( key == keyEnter)
            {
                {
#ifdef _DEBUG_
                    guiTree.resetCurrentState();
#else
                    if (menu->recvStage != 3)
                    	menu->recvStage++;
                    if (menu->recvStage == 1)
                    {
                        failFlag = false;
    					menu->virtCounter = 0;
                        voice_service->TurnPSWFMode(0,0,0,0); // 1 param - request /no request
                    }
                    if (menu->recvStage == 2)
                    {
                    	menu->recvStage = 0;
                    	//guiTree.backvard();
                    	guiTree.resetCurrentState();
                    	onCompletedStationMode();
                    	isStartCond = false;
    					menu->virtCounter = 0;
                    }
#endif
                }
            }
            break;
        }
        case GuiWindowsSubType::rxSmsMessage:
        {
        	if ( key == keyBack)
        	{
        		if (cntSmsRx > 0)
        			--cntSmsRx;
        		if (cntSmsRx == 0)
        		{
        			cntSmsRx = -1;
					guiTree.backvard();
					menu->focus = 0;
					isSmsMessageRec = false;
					menu->smsStage = 0;
					menu->smsTxStage = 1;
					onCompletedStationMode();
					menu->virtCounter = 0;
					break;
        		}
        	}
        	if ( key == keyEnter)
        	{
               ++cntSmsRx;
        	}
        	if ( key == keyBack || key == keyEnter){
        	  if (cntSmsRx == 1)
        	  {
        		menu->virtCounter = 0;
                menu->initRxSmsDialog(startStr, cntSmsRx);
        		isSmsMessageRec = false;
        	  }
        	  if (cntSmsRx == 2)
        	  {
				#ifndef PORT__PCSIMULATOR
        		voice_service->TurnSMSMode();
				#endif
        		if (valueRxSms == 0)
        		{
        			if ( voice_service->getVirtualMode() )
        			{
        				menu->virtCounter = 0;
        				std::string str;
        	    		char syn[4] = {0,0,0,0};
        	    		sprintf(syn, "%d", menu->virtCounter);
        	    		str.append("\t\t").append(syncWaitingStr).append("\n\t ").append(syn).append(" / 120");
                		menu->virtCounter = 0;
        				menu->initRxSmsDialog(str.c_str());
        			}
        			else
        				menu->initRxSmsDialog(receiveStatusStr[1]);
        		}
        		else
        		{
        			menu->VoiceDialogClearWindow();
        			menu->RxSmsStatusPost(valueRxSms);
        		}
        	  }
              if (cntSmsRx == 3)
        	  {
                  //smsMessage(13);
                  guiTree.resetCurrentState();
                  isSmsMessageRec = false;
                  menu->smsStage = 0;
                  cntSmsRx = -1;
                  menu->smsTxStage = 1;
                  onCompletedStationMode();
                  menu->virtCounter = 0;
        	  }
        	}

        	if ( key == keyUp)
        	{
                if (menu->smsScrollIndex > 0)
                    menu->smsScrollIndex--;

        		if (cntSmsRx >= 2 && isSmsMessageRec)
        		{
        			showReceivedSms();
        		}

        	}
        	if ( key == keyDown)
        	{
                menu->smsScrollIndex++;

        		if (cntSmsRx >= 2 && isSmsMessageRec)
        		{
        			showReceivedSms();
        		}
        	}
        	break;
        }
        case GuiWindowsSubType::recvGroupCondCmd:
        {
        	if ( key == keyBack)
        	{
        		switch (cntGucRx)
        		{
					case 0:
					case 4:
					{
	        			cntGucRx = -1;
						guiTree.backvard();
	                    menu->offset = 1;
	                    menu->focus = 2;
	                    onCompletedStationMode(false);
						break;
					}
					case 1:
					case 2:
					case 3:
					{
						--cntGucRx;
						break;
					}
        		}
        	}
            if ( key == keyEnter)
            {
            	++cntGucRx;
            }
            if ( key == keyBack || key == keyEnter)
            {

        		switch (cntGucRx)
        		{
					case 1:
					{
						menu->initRxSmsDialog(STARTS);
						break;
					}
					case 2:
					{
	                    menu->initRxSmsDialog(receiveStatusStr[1]);
	            		//setFreq();
						#ifndef PORT__PCSIMULATOR
						voice_service->saveFreq(getFreq());
						voice_service->TurnGuc();
						#else
		                guiTree.resetCurrentState();
						#endif
						break;
					}
					case 3:
					{
	                    cntGucRx = -1;
	            		guiTree.resetCurrentState();
	                    onCompletedStationMode(false);
	                    break;
					}
					case 4:
					{
						menu->initRxSmsDialog(txQwit,10);
						break;
					}
        		}
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
                    menu->offset = 1;
                    menu->focus = 3;
                    guiTree.backvard();
                    menu->inVoiceMail = false;
                    menu->toVoiceMail = false;
#ifndef _DEBUG_
                    voice_service->stopAle();
                    onCompletedStationMode();
#endif
                }
                if (key == keyEnter)
                {
#ifndef _DEBUG_
                    updateAleState(AleState_IDLE);
                    voice_service->startAleRx();
#endif
                    menu->inVoiceMail = true;
                    menu->putOffVoiceStatus++;
                }
                break;
            }
            case 2:
            {
                if (key == keyBack)
                {
#ifndef _DEBUG_
                    voice_service->stopAle();
#endif
                    menu->voiceAddr.clear();
                    menu->putOffVoiceStatus--;
                }
                if (key == keyEnter)
                {
                 //    uint8_t rxAddr = voice_service->getAleRxAddress();
//                    char ch[3]; sprintf(ch, "%d", rxAddr); ch[2] = '\0';
//                    menu->voiceAddr.append(ch);
//                    menu->putOffVoiceStatus++;

//                    if (rxAddr > 0)
//                    {
//                        char ch[3]; sprintf(ch, "%d", rxAddr); ch[2] = '\0';
//                        menu->voiceAddr.append(ch);
//                        menu->putOffVoiceStatus++;
//                        voice_service->stopAle();
//                        Multiradio::voice_message_t message = voice_service->getAleRxVmMessage();
//                        if (storageFs > 0)
//                        {
//                            showMessage(waitingStr, flashProcessingStr, promptArea);
//                            storageFs->writeMessage(DataStorage::FS::FT_VM, DataStorage::FS::TFT_RX, &message);
//                            draw();
//                        }
//                    }

//                    if (rxAddr == 0)
//                    {
//                        voice_service->stopAle();
//                        menu->putOffVoiceStatus = 1;
//                        menu->voiceAddr.clear();
//                        menu->channalNum.clear();
//                        menu->offset = 1;
//                        menu->focus = 3;
//                        guiTree.backvard();
//                        menu->inVoiceMail = false;
//                        menu->toVoiceMail = false;
//                    }
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
                    	headset_controller->setSmartMessageToPlay(voice_service->getAleRxVmMessage());
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
                    menu->inVoiceMail = false;
                    menu->toVoiceMail = false;
#endif

                }
                break;
            }
            }
            break;
        }
        case GuiWindowsSubType::volume:
        {
            if ( key == keyRight || key == keyUp )
            {
                if (!isVolumeEdit)
                    volumeLevel = menu->getVolume();
                menu->incrVolume();
                uint8_t level = menu->getVolume();
                voice_service->TuneAudioLevel(level);
                isVolumeEdit = true;
            }
            if ( key == keyLeft || key == keyDown )
            {
                if (!isVolumeEdit)
                    volumeLevel = menu->getVolume();
                menu->decrVolume();
                uint8_t level = menu->getVolume();
                voice_service->TuneAudioLevel(level);
                isVolumeEdit = true;
            }
            if ( key == keyBack)
            {
                if (isVolumeEdit){
                    menu->setVolume(volumeLevel);
                    voice_service->TuneAudioLevel(volumeLevel);
                }
                guiTree.backvard();
                menu->offset = 2;
                menu->focus = 3;
                isVolumeEdit = false;
            }
            if ( key == keyEnter)
            {
                if (isVolumeEdit){
                    volumeLevel = menu->getVolume();
                    menu->setVolume(volumeLevel);
                    voice_service->TuneAudioLevel(volumeLevel);
                }
                guiTree.backvard();
                menu->offset = 2;
                menu->focus = 3;
                isVolumeEdit = false;
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
                onCompletedStationMode();
                menu->offset = 1;
                menu->focus = 2;
            }
            if (key == keyEnter)
            {
            	onCompletedStationMode();
            	guiTree.resetCurrentState();
            }
            break;
        }
        case GuiWindowsSubType::suppress:
        {
            static int8_t oldSuppress = -1;

            if ( key == keyRight || key == keyLeft )
            {
                if (oldSuppress == -1)
                    oldSuppress = menu->supressStatus;

                if (menu->supressStatus < 24 && key == keyRight)
                    ++menu->supressStatus;
                if (menu->supressStatus > 6 && key == keyLeft)
                    --menu->supressStatus;
                if (menu->supressStatus > 24 || menu->supressStatus <6)
                    menu->supressStatus = 6;

                //value =  menu->supressStatus;
                voice_service->tuneSquelch(menu->supressStatus);
            }
            if (key == keyDown)
            {
                if (oldSuppress == -1)
                    oldSuppress = menu->supressStatus;
                menu->supressStatus = 0;
                voice_service->tuneSquelch(menu->supressStatus);
            }

            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->offset = 3;
                menu->focus = 4;
                if (oldSuppress != -1){
                    menu->supressStatus = oldSuppress;
                    voice_service->tuneSquelch(menu->supressStatus);
                }
                oldSuppress = -1;

            }
            if (key == keyEnter)
            {
                //voice_service->tuneSquelch(menu->supressStatus);
                guiTree.backvard();
                menu->offset = 3;
                menu->focus = 4;
                oldSuppress = -1;
            }
            break;
        }
        case GuiWindowsSubType::display:
        {
            if (key == keyLeft )
            {
                if (menu->displayBrightness > 0)
                menu->displayBrightness--;
            }
            if ( key == keyRight)
            {
                if (menu->displayBrightness < 2)
                menu->displayBrightness++;
            }
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->offset = 4;
                menu->focus = 5;
            }
            if (key == keyEnter)
            {
                guiTree.backvard();
                menu->offset = 3;
                menu->focus = 4;
                if (menu->displayBrightness == 2)
                    setColorScheme(G_BLACK,G_WHITE);
                if (menu->displayBrightness == 1)
                    setColorScheme(G_BLACK,G_LLIGHTGREY);
                if (menu->displayBrightness == 0)
                    setColorScheme(G_BLACK,G_LIGHTGREY);
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

            if ( key == keyBack || key == keyEnter)
            {
                guiTree.backvard();
                menu->offset = 1;
                menu->focus = 2;
            }
            break;
        }
        case GuiWindowsSubType::gpsCoord:
        {
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->offset = 2;
                menu->focus = 3;
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
            case keyEnter:
            case keyBack:
            {
                if (isChangeGpsSynch){
                	voice_service->setVirtualMode(!gpsSynchronization);
                	if (storageFs > 0)
                		storageFs->setGpsSynchroMode((uint8_t)gpsSynchronization);
                	isChangeGpsSynch = false;
                	updateSessionTimeSchedule();
                }
                guiTree.backvard();
                menu->focus = 0;
                break;
            }
            case keyRight:
            case keyLeft:
            {
                gpsSynchronization = !gpsSynchronization;
                isChangeGpsSynch = true;
                break;
            }
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
                    if (st.size() == 2 || st.size() == 5)
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
                    auto day = st.substr(0, 2);
                    if ( atoi(day.c_str()) > 31 || atoi(day.c_str()) == 0 ){
                        st.pop_back(); st.pop_back();
                    }
                }
                if (st.size() > 4 && st.size() < 6 )
                {
                    // 1 <= ?? <= 12
                    auto month = st.substr(3, 2);
                    if ( atoi(month.c_str()) > 12 || atoi(month.c_str()) == 0 ){
                        st.pop_back(); st.pop_back();
                    }
                }
                if (st.size() == 8 )
                {
                    // 16 <= ?? <= 99
                    auto year = st.substr(6, 2);
                    if ( atoi(year.c_str()) < 16){
                        st.pop_back(); st.pop_back();
                    }
                }

                if ( st.size() == 2  || st.size() == 5)
                {
                    st.push_back('.');
                }
            }
            if (key == keyEnter)
            {
            	auto &st = ((CEndState&)guiTree.getCurrentState()).listItem.front()->inputStr;
            	voice_service->setVirtualDate(st);
            	guiTree.backvard();
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
                    if (st.size() == 2 || st.size() == 5)
                       st.pop_back();
                }
                else
                {
                    guiTree.backvard();
                    menu->focus = 0;
                    updateSessionTimeSchedule();
                }
            }
            if (key == keyEnter)
            {
            	auto &st = ((CEndState&)guiTree.getCurrentState()).listItem.front()->inputStr;
            	voice_service->setVirtualTime(st);
            	guiTree.backvard();
                updateSessionTimeSchedule();
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
                if (st.size() > 3 && st.size() < 6 )
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
        {
            switch ( key )
            {
            case keyEnter:
            {
                auto iter = estate.listItem.begin();
                main_scr->oFreq.clear();
                main_scr->oFreq.append( (*iter)->inputStr );
                int freq = atoi(main_scr->oFreq.c_str());
                voice_service->tuneFrequency(freq, true);

                guiTree.resetCurrentState();
                menu->focus = 0;
            }
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
        case GuiWindowsSubType::setSpeed:
        {
            switch ( key )
            {
            case keyEnter:
            {
                voice_service->setCurrentChannelSpeed(currentSpeed);
                break;
            }
            case keyBack:
            {
                guiTree.backvard();
                menu->offset = 0;
                menu->focus = 1;
                break;
            }
            case keyUp:
            {
                if ( currentSpeed < Multiradio::voice_channel_speed_t(4) )
                {
                    int i = currentSpeed;
                    currentSpeed = Multiradio::voice_channel_speed_t(++i);
                }
                break;
            }
            case keyDown:
            {
                if ( currentSpeed > Multiradio::voice_channel_speed_t(1) )
                {
                    int i = currentSpeed;
                    currentSpeed = Multiradio::voice_channel_speed_t(--i);
                }
                break;
            }
            default:
                break;
            }
            break;
        }
        case GuiWindowsSubType::editRnKey:
        {
            // выбрать канал воспроизведения
            if ( key > 5 && key < 16 && menu->RN_KEY.size() < 3 )
            {
                menu->RN_KEY.push_back((char)(42+key));
                // check
                int rc = atoi(menu->RN_KEY.c_str());

                if ( rc < 1 || rc > 999)
                { menu->RN_KEY.clear(); }
            }
            if (key == keyBack)
            {
                if (menu->RN_KEY.size() > 0 && menu->RN_KEY != "0")
                    menu->RN_KEY.pop_back();
                else
                {
                    uint16_t t; storageFs->getFhssKey(t);
                    char ch[4]; sprintf(ch, "%d", t); ch[3] = '\0';
                    menu->RN_KEY = ch;
                    menu->focus = 4;
                    menu->offset = 3;
                    guiTree.backvard();
                }
            }
            if (key == keyEnter)
            {
                storageFs->setFhssKey((uint16_t)atoi(menu->RN_KEY.c_str()));
                voice_service->setRnKey(atoi(menu->RN_KEY.c_str()));
                menu->focus = 4;
                menu->offset = 3;
                guiTree.backvard();

            }
            break;
        }
        case GuiWindowsSubType::voiceMode:
        {
            if ( key == keyEnter)
            {
                if (menu->useMode)
                    voice_service->setVoiceMode(Multiradio::VoiceServiceInterface::VoiceMode::VoiceModeAuto);
                else
                    voice_service->setVoiceMode(Multiradio::VoiceServiceInterface::VoiceMode::VoiceModeManual);
                storageFs->setVoiceMode(menu->useMode);
                guiTree.backvard();
                menu->focus = 6;
                menu->offset = 5;
            }
            if ( key == keyBack)
            {
                guiTree.backvard();
                menu->focus = 6;
                menu->offset = 5;
            }
            if (key == keyUp || key == keyDown)
            {
                menu->useMode = !menu->useMode;
            }

            break;
        }
        case GuiWindowsSubType::channelEmissionType:
        {
            if ( key == keyEnter || key == keyBack)
            {
                if (menu->ch_emiss_type)
                    voice_service->tuneEmissionType(Multiradio::voice_emission_t::voiceemissionFM);
                else
                    voice_service->tuneEmissionType(Multiradio::voice_emission_t::voiceemissionUSB);

                guiTree.backvard();
                menu->focus = 5;
                menu->offset = 4;
            }
            if (key == keyUp || key == keyDown)
            {
                menu->ch_emiss_type = !menu->ch_emiss_type;
            }
            break;
        }
        case GuiWindowsSubType::filetree:
        {
            if ( key == keyEnter)
            {
                if (menu->filesStage == 0){

                    menu->fileType = (DataStorage::FS::FileType)menu->filesStageFocus[0];
                     if (storageFs > 0)
                          storageFs->getFileNamesByType(&menu->tFiles[menu->fileType], menu->fileType);
                }

                if (menu->filesStage == 1){

                	DataStorage::FS::TransitionFileType ft;
                    if (storageFs > 0) ft = storageFs->getTransmitType(menu->fileType, menu->filesStageFocus[1]);

                    switch (menu->fileType)
                    {
                    case DataStorage::FS::FT_SMS:
                    case DataStorage::FS::FT_CND:
                    case DataStorage::FS::FT_GRP:
                        if (menu->tFiles[menu->fileType].size() > 0)
                        menu->fileMessage = loadMessage(menu->fileType, ft, storageFs->getFileNumber(menu->fileType, menu->filesStageFocus[1]));
                        break;

                    case DataStorage::FS::FT_VM:
                        if (menu->tFiles[menu->fileType].size() > 0)
                        menu->fileMessage = loadVoiceMail(storageFs->getFileNumber(menu->fileType, menu->filesStageFocus[1]), ft);
                        break;
                    }
                }

                switch (menu->filesStage){
                case 0:
                    menu->filesStage++; break;
                case 1:
                    if (menu->tFiles[menu->fileType].size() > 0)
                        menu->filesStage++;
                    break;
                }
            }
            if ( key == keyBack)
            {
                if (menu->filesStage > 0)
                   menu->filesStage--;
                else
                {
                   guiTree.backvard();
                   menu->focus = 7;
                   menu->offset = 5;
                }
            }
            if (key == keyUp)
            {
                switch(menu->filesStage){
#if no_speah_hack
                case 0:
                    if (menu->filesStageFocus[menu->filesStage] > 1)
                        menu->filesStageFocus[menu->filesStage]--;
                    break;
                case 1:
                    if (menu->filesStageFocus[menu->filesStage] > 0)
                        menu->filesStageFocus[menu->filesStage]--;
                    break;
#else
                case 0:
                case 1:
                    if (menu->filesStageFocus[menu->filesStage] > 0)
                        menu->filesStageFocus[menu->filesStage]--;
                    break;
#endif
                case 2:
                    if (menu->textAreaScrollIndex > 0)
                    menu->textAreaScrollIndex--;
                    break;
                }
            }
            if (key == keyDown)
            {
                switch (menu->filesStage){
                case 0:
                    if (menu->filesStageFocus[menu->filesStage] < 4)
                        menu->filesStageFocus[menu->filesStage]++;
                    break;
                case 1:
                    if (menu->filesStageFocus[menu->filesStage] < menu->tFiles[menu->fileType].size()-1)
                        menu->filesStageFocus[menu->filesStage]++;
                    break;
                case 2:
                    menu->textAreaScrollIndex++;
                    break;
                }
            }
            if (key == keyRight)
            {
                if (menu->filesStage == 0)
                switch (menu->filesStageFocus[menu->filesStage]){
                    case 0:
                        #if smsFlashTest
                            flashTestOn = true;
                            smsMessage(smsflashTest_size);
                        #endif
                    break;

                    case 2:
                        #if cndFlashTest
                            FirstPacketPSWFRecieved(42);
                        #endif
                    break;

                    case 3:
                        #if grpFlashTest
                            gucFrame(0);
                        #endif
                    break;
                }
            }
            break;
        }
        case GuiWindowsSubType::sheldure:
        {
            static uint8_t sheldureStagePrev = 0;
            static bool isNew = false;

            switch (menu->sheldureStage) {

            case 0: // session list
                sheldureStagePrev = 0;
                if ( key == keyBack )
                {
                    tempSheldureSession.clear();
                    guiTree.backvard();
                    menu->offset = 4;
                    menu->focus = 6;
                    break;
                }
                if ( key == keyEnter )
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] + 1 == sheldure_data.size() && sheldure.size() < 50){
                        menu->sheldureStage = 1;
                        isNew = true;
                        tempSheldureSession.clear();
                    }
                    else{
                        isNew = false;
                        tempSheldureSession.copyFrom(&sheldure[menu->sheldureStageFocus[menu->sheldureStage]]);
                        menu->sheldureStageFocus[1] = tempSheldureSession.type;
                        menu->sheldureTimeStr = tempSheldureSession.time;
                        menu->sheldureFreqStr = tempSheldureSession.freq;
                        menu->sheldureStage = 4;
                    }
                }
                if ( key == keyUp)
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] > 0)
                      menu->sheldureStageFocus[menu->sheldureStage]--;
                }
                if ( key == keyDown)
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] < (sheldure.size() - (sheldure.size() == 50)) )
                      menu->sheldureStageFocus[menu->sheldureStage]++;
                }
             break;
            case 1: //type msg
                if ( key == keyBack )
                {
                    if (sheldureStagePrev == 0 || sheldureStagePrev == 4) // list or edit
                        menu->sheldureStage = sheldureStagePrev;
                }
                if ( key == keyEnter )
                {                  
                    if (!isNew){
                        tempSheldureSession.time = sheldure[menu->sheldureStageFocus[0]].time;
                    }
                    tempSheldureSession.type = (DataStorage::FS::FileType)menu->sheldureStageFocus[menu->sheldureStage];
                    menu->sheldureTimeStr = tempSheldureSession.time;
                    menu->sheldureStage = 2; // time
                }
                if ( key == keyUp)
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] > 0){
                      menu->sheldureStageFocus[menu->sheldureStage]--;
                      tempSheldureSession.type = (DataStorage::FS::FileType)menu->sheldureStageFocus[menu->sheldureStage];
                    }
                }
                if ( key == keyDown)
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] < 4){
                      menu->sheldureStageFocus[menu->sheldureStage]++;
                      tempSheldureSession.type = (DataStorage::FS::FileType)menu->sheldureStageFocus[menu->sheldureStage];
                    }
                }
             break;
            case 2: // time

            if ( key == keyBack )
            {
                if (tempSheldureSession.time.size() > 0){
                    tempSheldureSession.time.pop_back();
                }
                else
                    menu->sheldureStage = 1;
            }
            if ( key == keyEnter )
            {
                if (tempSheldureSession.time.size() == 5)
                menu->sheldureStage = 3;
            }
            if ( key >= key0 && key <= key9 )
            {
                if ( tempSheldureSession.time.size() == 2)
                    tempSheldureSession.time.push_back(':');

                if ( tempSheldureSession.time.size() < 5)
                    tempSheldureSession.time.push_back(key + 42);

                if (tempSheldureSession.time.size() > 1 && tempSheldureSession.time.size() < 3 )
                {
                    // 0 <= ?? <= 23
                    auto hh = tempSheldureSession.time.substr(0, 2);
                    if ( atoi(hh.c_str()) > 23 )
                        tempSheldureSession.time.clear();
                }
                if (tempSheldureSession.time.size() > 3 && tempSheldureSession.time.size() < 6 )
                {
                    // 0 <= ?? <= 59
                    auto mm = tempSheldureSession.time.substr(3, 2);
                    if ( atoi(mm.c_str()) > 59 )
                    {
                        tempSheldureSession.time.pop_back(); tempSheldureSession.time.pop_back();
                    }
                }

              //  if ( tempSheldureSession.time.size() == 2)
              //      tempSheldureSession.time.push_back(':');
            }
            menu->sheldureTimeStr = tempSheldureSession.time;
            if (!isNew)
                tempSheldureSession.freq = sheldure[menu->sheldureStageFocus[0]].freq;
            menu->sheldureFreqStr = tempSheldureSession.freq;
            menu->sheldureFreqStr.append(" ").append(freq_hz);
            break;
            case 3: // freq
                if ( key == keyBack )
                {
                    if (tempSheldureSession.freq.size() > 0){
                        tempSheldureSession.freq.pop_back();
                    }
                    else
                        menu->sheldureStage = 2; // time
                }
                if ( key == keyEnter )
                {
                    if (tempSheldureSession.freq.size() > 4 && tempSheldureSession.freq.size() < 9){
                        if (isNew){
                            sheldure.push_back(tempSheldureSession);
                            tempSheldureSession.clear();
                        } else {
                            sheldure[menu->sheldureStageFocus[0]].copyFrom(&tempSheldureSession);
                            tempSheldureSession.clear();
                        }
                        uploadSheldure();
                        sheldureToStringList();
                        menu->sheldureStage = 0;
                    }
                }
                if ( key >= key0 && key <= key9 )
                {
                    if (tempSheldureSession.freq.size() < 8 )
                        tempSheldureSession.freq.push_back(key + 42);
                }
                menu->sheldureFreqStr = tempSheldureSession.freq;
                menu->sheldureFreqStr.append(" ").append(freq_hz);
            break;

            case 4:// editing
                sheldureStagePrev = 4;
                if ( key == keyUp)
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] > 0)
                      menu->sheldureStageFocus[menu->sheldureStage]--;
                }
                if ( key == keyDown)
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] < 1)
                      menu->sheldureStageFocus[menu->sheldureStage]++;
                }
                if ( key == keyBack )
                {
                    menu->sheldureStage = 0;
                    menu->sheldureStageFocus[4] = 0;
                }
                if ( key == keyEnter )
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] == 0) // type
                    	menu->sheldureStage = 1;
                    else if (menu->sheldureStageFocus[menu->sheldureStage] == 1) // delete
                    	menu->sheldureStage = 5;
                    menu->sheldureStageFocus[4] = 0;
                }
            break;

            case 5: // delete question
                if ( key == keyUp)
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] > 0)
                      menu->sheldureStageFocus[menu->sheldureStage]--;
                }
                if ( key == keyDown)
                {
                    if (menu->sheldureStageFocus[menu->sheldureStage] < 1)
                      menu->sheldureStageFocus[menu->sheldureStage]++;
                }
                if ( key == keyBack )
                {
                    menu->sheldureStage = 4;
                    menu->sheldureStageFocus[5] = 0;
                }
                if ( key == keyEnter )
                {
                    if(menu->sheldureStageFocus[menu->sheldureStage] == 0) // no
                        menu->sheldureStage = 4;
                    else {  // delete
                        menu->sheldureStage = 0; // session list
                        sheldure.erase(sheldure.begin() + menu->sheldureStageFocus[0]);
                        if (menu->sheldureStageFocus[0] > 0)
                            menu->sheldureStageFocus[0]--;
                        uploadSheldure();
                        sheldureToStringList();
                        menu->sheldureStageFocus[5] = 0;
                    }
                }
            break;


            } // switch exit

            break;
        }

        // default menu
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

void Service::onSmsCounterChange(int param)
{
    qmDebugMessage(QmDebug::Warning, "______sms counter: %d ", param);
    menu->virtCounter = 0;
    if (cntSmsRx != 2)
    {
        menu->smsTxStage = 6;
        if ((param > 0 && param < 84) && (!failFlag))
        	drawMenu();
        else
        	menu->smsTxStage = 1;
    }
    else
    {
#if SMS_PROGRESS

    	if (param > 0 && param < 84)
    	{
    		CState currentState;
    		guiTree.getLastElement(currentState);

    		valueRxSms = param;

    		if (currentState.getType() == endMenuWindow)
    		{
    			GuiWindowsSubType subType = ((CEndState&)guiTree.getCurrentState()).subType;
    			if ((subType == rxSmsMessage) && (cntSmsRx == 2))
    				drawMenu();
    		}

    	}
    	else
    	{
    		menu->smsTxStage = 6;
    	}
#endif
    }
}

void Service::FirstPacketPSWFRecieved(int packet, uint8_t address, bool isRec)
{
    if ( packet >= 0 && packet < 100 )
    {
//    	guiTree.resetCurrentState();
//    	drawMainWindow();
        char sym[3];
        sprintf(sym,"%d",packet);

        if (storageFs > 0 && isRec)
        {
            if (packet < 10) sym[1] = 0;
            sym[2] = 0;
            fileMsg.clear();
            fileMsg.push_back((uint8_t)sym[0]);
            fileMsg.push_back((uint8_t)sym[1]);
            fileMsg.push_back((uint8_t)sym[2]);

            GUI_Painter::ClearViewPort();
            showMessage(waitingStr, flashProcessingStr, promptArea);
            storageFs->writeMessage(DataStorage::FS::FT_CND, DataStorage::FS::TFT_RX, &fileMsg);
            draw();
        }

         //guiTree.append(messangeWindow, "Принятый пакет ", sym);
         condCmdValue = packet;
         isDrawCondCmd = true;

		 char addressStr[4] = {0,0,0,0};
		 sprintf(addressStr, "%d", address);

         if (isRec)
         {
			 if (setAsk)
			 {
				 guiTree.resetCurrentState();
				 menu->txCondCmdStage = 0;
				 isWaitAnswer = false;

				 std::string str(cmdRec);
				 str.append(" ").append(fromStr).append(" ").append(addressStr);
				 msgBox( str.c_str(), (int)packet );
				 setAsk = false;
			 }
			 else
			 {
				 std::string str(recPacket);
				 str.append(" ").append(fromStr).append(" ").append(addressStr);
				 msgBox( str.c_str(), (int)packet );
				 isCondModeQwitTx = true;
			 }
         }
         else
         {
			 std::string str(notReiableRecPacket);
			 str.append(" ").append(fromStr).append(" ").append(addressStr);
        	 msgBox(  str.c_str(), (int)packet );
         }
    }
     else
     {
        onCompletedStationMode();
     if ( packet == 100)
     {
		 guiTree.resetCurrentState();
		 menu->txCondCmdStage = 0;
		 isWaitAnswer = false;
         msgBox( gucQuitTextFail );
         setAsk = false;
     }
    else if ( packet > 100)
        msgBox( rxCondErrorStr[0] );
    else
        msgBox( rxCondErrorStr[1] );
    }
    if (isCondModeQwitTx)
    	menu->recvStage = 3;
    else
    	menu->recvStage = 0;
    isStartCond = false;
     //setFreq();
}

void Service::msgBox(const char *title)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(159), (GYT)(127)};

    if (msg_box != nullptr)
        delete msg_box;
    msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, align007);

    guiTree.append(messangeWindow, "");
    if (!isStartTestMsg)
        msg_box->Draw();
}

void Service::msgBox(const char *title, const char *text)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(159), (GYT)(127)};

    if (msg_box != nullptr)
        delete msg_box;
    msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, (char*)text, align007);

    guiTree.append(messangeWindow, "");
    if (!isStartTestMsg)
        msg_box->Draw();
}

void Service::msgBox(const char *title, const int condCmd)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(159), (GYT)(127)};

    if (msg_box != nullptr)
        delete msg_box;
    msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, (int)condCmd, align007);

    guiTree.append(messangeWindow, "");
    msg_box->setCmd(condCmd);
    if (!isStartTestMsg)
        msg_box->Draw();
    isDrawCondCmd = false;
}


void Service::msgBox(const char *title, const int condCmd, const int size, const int pos, uint8_t* coord = 0)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(159), (GYT)(127)};

    if (msg_box != nullptr)
        delete msg_box;
    msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, (int)condCmd, (int) size, (int) pos, align007);

    msg_box->setCmd(condCmd);
    msg_box->position = pos;

    guiTree.append(messangeWindow, "");
    if (!isStartTestMsg)
        msg_box->DrawWithCoord(coord);
}

void Service::drawMainWindow()
{
    if (!isStartTestMsg){
    Multiradio::VoiceServiceInterface *voice_service = pGetVoiceService();

    Multiradio::voice_emission_t emission_type = voice_service->getCurrentChannelEmissionType();

    std::string str;
    switch (emission_type)
    {
    case Multiradio::voice_emission_t::voiceemissionFM:
        str.append(ch_em_type_str[0]);
        break;
    case Multiradio::voice_emission_t::voiceemissionUSB:
        str.append(ch_em_type_str[1]);
        break;
    default:
        str.append((char*)"--\0");
        break;
    }

    main_scr->setModeText(str.c_str());

    auto status = voice_service->getStatus();

    bool valid_freq = true;
    if ( status == Multiradio::VoiceServiceInterface::StatusNotReady || status == Multiradio::VoiceServiceInterface::StatusIdle )
        valid_freq = false;

    int ch_num;
    Multiradio::voice_channel_t channelType;

    if (main_scr->channelEditing && channelNumberSyms)
    {
    	ch_num = channelNumberEditing;
    	channelType = headset_controller->getChannelType(channelNumberEditing);
    }
    else
    {
    	ch_num = voice_service->getCurrentChannelNumber();
    	channelType = voice_service->getCurrentChannelType();
    }

    main_scr->Draw(voice_service->getCurrentChannelStatus(),
    		       ch_num,
				   channelType,
                   valid_freq
                   );

    drawIndicator();
    drawWaveInfo();

    }
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
        t.clear();
    }
    else
    {
        CEndState st = (CEndState&)guiTree.getCurrentState();
        menu->setTitle(st.getName());

        switch( st.subType )
        {
        case GuiWindowsSubType::condCommand: //txCond
        {
            menu->initCondCommDialog(st, voice_service->getVirtualMode(), isWaitAnswer);
            break;
        }
        case GuiWindowsSubType::txGroupCondCmd:
        {
            menu->initGroupCondCmd(st, isGucAnswerWaiting);
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
                status = voice_service->getAleState();
                menu->vmProgress = voice_service->getAleVmProgress();

            }

            if (menu->putOffVoiceStatus == 1) voice_service->resetDSPLogic();

            bool isDraw = true;
            if (menu->putOffVoiceStatus == 2 && (status == 8 || status == 9))
                isDraw = false;

            if (isDraw)
                menu->initTxPutOffVoiceDialogTest(status);

            break;
        }
        case GuiWindowsSubType::txSmsMessage:
        {
            if (!isSmsMessageRec){
            std::string titleStr, fieldStr;

        	auto iter = st.listItem.begin();
            uint8_t num = 0;
            if (menu->smsTxStage >= 2 && menu->smsTxStage <= 4)
            {
                switch(menu->smsTxStage)
                 {
                 	case 2: num = 1; break;
                 	case 4: num = 4; break;
                 }
            }

            switch(menu->smsTxStage)
            {
            case 1:
            {
                titleStr.append(ticketStr[1]);
                fieldStr.append(useScanMenu[menu->useSmsRetrans]);
                break;
            }
            case 3:            // address dst
            case 4: (*iter)++; // message
            case 2: (*iter)++; // address retr
            {
                titleStr.append(condCommStr[num]);
                fieldStr.append((*iter)->inputStr); // address retr
                break;
            }
            case 5:
            {
                fieldStr.clear();
                fieldStr.append(startStr);
                break;
            }
            case 6:
            {

            	uint8_t counter = voice_service->getSmsCounter();
            	bool isSynch = voice_service->getVirtualMode() && !counter;

            	fieldStr.clear();

            	if (isSynch)
            	{
            		char syn[4] = {0,0,0,0};
            		sprintf(syn, "%d", menu->virtCounter);
            		//if (menu->virtCounter)
            		fieldStr.append("\t").append(syncWaitingStr).append("\n ").append(syn).append(" / 120");
//            		else
//            			fieldStr.append(receiveStatusStr[1]);
            	}
            	else
            	{
					char pac[2];
					sprintf(pac,"%i", counter);

					fieldStr.append(pac);
					fieldStr.append("/82");
            	}
                break;
            }
            default:
            { break; }
            }

            menu->initTxSmsDialog( titleStr, fieldStr );
            break;
            }
        }
        case GuiWindowsSubType::recvCondCmd:
        {
        	bool isSynch = voice_service->getVirtualMode();
            menu->initRxCondCmdDialog(isSynch, isStartCond);
            break;
        }
        case GuiWindowsSubType::recvGroupCondCmd:
        {
            if (cntGucRx == -1){
        		cntGucRx = 0;
        		keyPressed(keyEnter);
        	}
            break;
        }
        case GuiWindowsSubType::recvVoice: break;
        case GuiWindowsSubType::rxSmsMessage:
        {
        	if (cntSmsRx == -1){
        		cntSmsRx = 0;
        		valueRxSms = 0;
        		keyPressed(keyEnter);
        	}
            if (cntSmsRx == 2)
            {
                if (valueRxSms > 0 && valueRxSms < 84)
                {
                		menu->RxSmsStatusPost(valueRxSms,1,true);
                }
                else if ( voice_service->getVirtualMode() && menu->smsTxStage != 6)
                {
       				std::string str;
       				char syn[4] = {0,0,0,0};
       				sprintf(syn, "%d", menu->virtCounter);
       				str.append("\t\t").append(syncWaitingStr).append("\n\t ").append(syn).append(" / 120");
       				menu->initRxSmsDialog(menu->virtCounter ? str.c_str() : receiveStatusStr[1]);
                }
            }
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
                status   = voice_service->getAleState();
                menu->vmProgress = voice_service->getAleVmProgress();
            }

            if (menu->vmProgress == 100)
            {
            	uint8_t rxAddr = voice_service->getAleRxAddress();
            	if (rxAddr > 0)
            	{
            		char ch[3];
            		sprintf(ch, "%d", rxAddr);
            		ch[2] = '\0';
            		menu->voiceAddr.append(ch);
            		menu->putOffVoiceStatus++;
            		voice_service->stopAle();
            		Multiradio::voice_message_t message = voice_service->getAleRxVmMessage();
            		if (storageFs > 0)
            		{
            			GUI_Painter::ClearViewPort();
            			showMessage(waitingStr, flashProcessingStr, promptArea);
            			storageFs->writeMessage(DataStorage::FS::FT_VM, DataStorage::FS::TFT_RX, &message);
            			//draw();
            			menu->toVoiceMail = false;
            		}
            	}
            	else
            	{
            		voice_service->stopAle();
            		menu->putOffVoiceStatus = 1;
            		menu->voiceAddr.clear();
            		menu->channalNum.clear();
            		menu->offset = 1;
            		menu->focus = 3;
            		guiTree.backvard();
            		menu->inVoiceMail = false;
            		menu->toVoiceMail = false;
            	}
            	menu->vmProgress == 0;
            }

            menu->initRxPutOffVoiceDialogTest(status);

            break;
        }
        case GuiWindowsSubType::gpsCoord:
        {
#if !defined(PORT__PCSIMULATOR)
            setCoordDate(navigator->getCoordDate());
#endif
            if (menu->coord_log[0] == '0')
            	menu->initGpsCoordinateDialog( menu->coord_lat, &menu->coord_log[1]);
            else
            	menu->initGpsCoordinateDialog( menu->coord_lat, menu->coord_log);
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
            std::string dateTemplate = "--.--.--";
            str.append(dateTemplate.substr(str.size(),8-str.size()));
            menu->initSetDateOrTimeDialog( str );
            break;
        }
        case GuiWindowsSubType::setTime:
        {
            menu->setTitle(dataAndTime[1]);
            std::string str; str.append(st.listItem.front()->inputStr); //str.append("00:00:00");
            std::string timeTemplate = "--:--:--";
            str.append(timeTemplate.substr(str.size(),8-str.size()));
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
            bool f_error = false;
            std::string str, speed;

            switch (currentSpeed)
            {
            case Multiradio::voice_channel_speed_t::voicespeed600:
            { str.append("600"); break;}
            case Multiradio::voice_channel_speed_t::voicespeed1200:
            { str.append("1200"); break;}
            case Multiradio::voice_channel_speed_t::voicespeed2400:
            { str.append("2400"); break;}
            case Multiradio::voice_channel_speed_t::voicespeed4800:
            { str.append("4800"); break;}
            case Multiradio::voice_channel_speed_t::voicespeedInvalid:
            { str.append(errorStr); break;}
            default:
            {
                str.append(errorStr);
                f_error = true;
                break;
            }
            }
            speed.append(str);

            if (currentSpeed != Multiradio::voice_channel_speed_t::voicespeedInvalid && !f_error)
            {  str.append(" ").append(speed_bit); }

            str.push_back('\0');

            menu->initSetSpeedDialog(speed);
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
            menu->initSuppressDialog();
            break;
        }
        case GuiWindowsSubType::display:
        {
            menu->initDisplayBrightnessDialog();
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
        case GuiWindowsSubType::sheldure:
        {
            menu->initSheldureDialog(&sheldure_data, sheldure.size());
            break;
        }
        case GuiWindowsSubType::voiceMode:
        {
            menu->initSelectVoiceModeParameters(menu->useMode);
            break;
        }
        case GuiWindowsSubType::channelEmissionType:
        {
            menu->initSelectChEmissTypeParameters(menu->ch_emiss_type);
            break;
        }
        case GuiWindowsSubType::filetree:
        {
            if (!flashTestOn)
                menu->initFileManagerDialog(menu->filesStage);
            flashTestOn = false;
            break;
        }
        default:
            break;
        }
    }
    //showSchedulePrompt(DataStorage::FS::FT_SMS, 15);
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
        if (msg_box != nullptr)
        {
        	if (vect != nullptr)
        		if (isGucCoord)
        			msg_box->DrawWithCoord((uint8_t*)&gucCoords);
        		else
        			msg_box->DrawWithCoord((uint8_t*)0);
        	else
        		msg_box->Draw();
        }

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
    if (isShowSchedulePrompt)
    	showMessage("", schedulePromptText.c_str(), promptArea);
    if (isStartTestMsg)
    {
    	uint8_t address = voice_service->getStationAddress();
    	char add[3] = {0,0,0};
    	sprintf(add, "%d", address);
    	std::string str;
    	str.append(sazhenNameStr);
    	str.append(" # ");
    	str.append(add);

        GUI_Painter::ClearViewPort();
        GUI_Painter::DrawRect(0, 0, 159, 127, RDM_FILL);

        GUI_Painter::DrawText(35, 15, GUI_EL_TEMP_CommonTextAreaLT.font,(char*)radioStationStr);
        GUI_Painter::DrawText(35, 30, GUI_EL_TEMP_CommonTextAreaLT.font,(char*)str.c_str());
        GUI_Painter::DrawText(35, 65, GUI_EL_TEMP_CommonTextAreaLT.font,(char*)true_SWF);
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
    for(int i = 0; i <= len; i++){
        if ((str[i] == ' ') || (len == i && str[i-1] != ' '))
        {
            if (i - index == 2)
                number[2] = '\0';
            if (i - index == 1)
                number[1] = '\0';

            memcpy(number, &str[index], i - index);
            guc_command_vector.push_back(atoi(number));
            ++cnt;
            for(int j = 0; j < 3; j++) number[j] = '\0';
            index = i + 1;
        }
    }
}

void Service::setCoordDate(Navigation::Coord_Date date)
{
//    menu->coord_lat.clear();
//    menu->coord_log.clear();
    menu->date.clear();
    menu->time.clear();

    if (atoi((char*)date.latitude) > 0)
    {
    	memcpy(menu->coord_lat,date.latitude,11);
    	memcpy(menu->coord_log,date.longitude,12);
    }

    uint8_t *time;
    if (voice_service->getVirtualMode())
    	time = voice_service->getVirtualTime();
    else
        time = (uint8_t*)&date.time;

    std::string str;
    str.resize(9);

    str[0] = (char)time[0];
    str[1] = (char)time[1];
    str[2] = ':';
    str[3] = (char)time[2];
    str[4] = (char)time[3];
    str[5] = ':';
    str[6] = (char)time[4];
    str[7] = (char)time[5];
    str[8] = 0;

    //qmDebugMessage(QmDebug::Warning, "DATE TIME %s, isZda %d ", str.c_str(), navigator->isZda);
    indicator->date_time->SetText((char*)str.c_str());
    //if (guiTree.getCurrentState().getType() == GuiWindowTypes::mainWindow)
    drawIndicator();
}

void Service::gucFrame(int value, bool isTxAsk)
{       
#if grpFlashTest

   const char *sym = "Recieved packet for station\0";

   std::string gucText = "42 1 2 3 4 5 6 7 8 9 10 10.12.13.100 11.13.14.100";
   uint16_t size = gucText.size() + 1;
   uint8_t gucCommands[size];
   //for (uint8_t i = 0; i < size; i++)
  //   gucText[i] = gucText[i];
   memcpy(&gucCommands[0], &gucText[0], size);
   gucCommands[size] = 0;

   char ch[3];
   sprintf(ch, "%d", gucCommands[position]);
   ch[2] = '\0';

   char coords[26];
   memcpy(&coords[0], &gucText[24], 25);
   coords[25] = 0;

  // guiTree.append(messangeWindow, sym, ch);
   msgBox( titleGuc, gucCommands[position], size, position, (uint8_t*)&coords );
   if (storageFs > 0)
   {
       uint16_t fullSize = size;
       storageFs->setGroupCondCommand((uint8_t*)&gucCommands, fullSize);
   }

#else

    const char *sym = "Recieved packet for station\0";
    vect = voice_service->getGucCommand();

    isGucCoord = voice_service->getIsGucCoord();
    uint8_t size = vect[0];

    char longitude[14]; longitude[12] = '\n';
    char latitude[14]; latitude[12] = '\0';
    if (isGucCoord)
    {
        // uint8_t coord[9] = {0,0,0,0,0,0,0,0,0};
        // getGpsGucCoordinat(coord);
        sprintf(longitude, "%02d.%02d.%02d.%03d", vect[size+1],vect[size+2],vect[size+3],vect[size+4]);
        sprintf(latitude, "%02d.%02d.%02d.%03d", vect[size+5],vect[size+6],vect[size+7],vect[size+8]);
        memcpy(&gucCoords[0],&longitude[0],13);
        memcpy(&gucCoords[13],&latitude[0],13);
        gucCoords[12] = '\n';
    }
    else
    {
        //std::string str = std::string(coordNotExistStr);
        //memcpy(&coords[0],&str[0],str.size());

        // memcpy(&coords[0],&coordNotExistStr[0],25);
        // coords[25]='\0';
    }

    if (vect[0] != 0)
    {
        char ch[3];
        sprintf(ch, "%d", vect[position]);
        ch[2] = '\0';

        if (storageFs > 0)
        {
        	uint16_t len = size * 3;
            uint16_t fullSize = isGucCoord ? len + 26 : len;
            uint8_t cmdv[fullSize];
            char cmdSym[3];

            for (uint8_t cmdSymInd = 1; cmdSymInd <= size; cmdSymInd++)
            {
                sprintf(cmdSym, "%02d", vect[cmdSymInd]);
                cmdSym[2] = ' ';
                memcpy(&cmdv[(cmdSymInd - 1) * 3], &cmdSym[0], 3);
            }

            if (isGucCoord)
                memcpy(&cmdv[len], &gucCoords[0], 26);
            fileMsg.clear();
            fileMsg.resize(fullSize);
            memcpy(fileMsg.data(), &cmdv, fullSize);

            GUI_Painter::ClearViewPort();
            showMessage(waitingStr, flashProcessingStr, promptArea);
            storageFs->writeMessage(DataStorage::FS::FT_GRP, DataStorage::FS::TFT_RX, &fileMsg);
            draw();
        }
        gucAdd = (uint8_t)value;
        //guiTree.append(messangeWindow, sym, ch);
        char add[4] = {0,0,0,0};
        sprintf(add,"%d", gucAdd);
        std::string str(titleGuc);
        str.append(" ").append(fromStr).append("#").append(" ").append(add);
        if (isGucCoord)
        	msgBox( str.c_str(), vect[1], size, 0, (uint8_t*)&gucCoords );
        else
        	msgBox( str.c_str(), vect[1], size, 0);

    }
    if (isTxAsk)
    	isGucModeQwitTx = true;
    else
    	cntGucRx = 1;

#endif
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

   // systemTimeTimer->start();
}

void Service::smsMessage(int value)
{
    if (storageFs > 0)
    {
        fileMsg.clear();
        fileMsg.resize(value);
        const char *text = (const char*)voice_service->getSmsContent();
        std::string text_str = text;
        memcpy(fileMsg.data(), &text[0], value);
        GUI_Painter::ClearViewPort();
        showMessage(waitingStr, flashProcessingStr, promptArea);
        storageFs->writeMessage(DataStorage::FS::FT_SMS, DataStorage::FS::TFT_RX, &fileMsg);
        draw();
    }

    menu->virtCounter = 0;
    isSmsMessageRec = true;
    menu->smsTxStage = 4;

    showReceivedSms();

    //setFreq();
}

void Service::showReceivedSms()
{
    const char *text = (const char*)voice_service->getSmsContent();
    std::string title = "CMC";
    std::string text_str = text;
    menu->initTxSmsDialog(title,text_str);
    menu->virtCounter = 0;
}

void Service::updateAleVmProgress(uint8_t t)
{
#if VM_PROGRESS
    QM_UNUSED(t);

    CState currentState;
    guiTree.getLastElement(currentState);

    if (currentState.getType() == endMenuWindow)
    {
        GuiWindowsSubType subType = ((CEndState&)guiTree.getCurrentState()).subType;
        if ( (subType == txPutOffVoice && (menu->putOffVoiceStatus == 5)) || (subType == rxPutOffVoice && (menu->putOffVoiceStatus == 2)))
            drawMenu();
    }
#endif
}


void Service::msgBoxSms(const char *text)
{

	Alignment align007 = {alignHCenter,alignTop};
	MoonsGeometry area007 = {1, 1, (GXT)(159), (GYT)(127)};
	if(msg_box == nullptr)
	{
		msg_box = new GUI_Dialog_MsgBox(&area007, (char*)"SMS",(char*)text, align007);
	}

	msg_box->Draw_Sms();
    menu->virtCounter = 0;

}

void Service::updateAleState(AleState state)
{
    QM_UNUSED(state);

    CState currentState;
    guiTree.getLastElement(currentState);

#if VM_STATE

    if (currentState.getType() == endMenuWindow)
    {
        GuiWindowsSubType subType = ((CEndState&)guiTree.getCurrentState()).subType;
        if ( (subType == txPutOffVoice && (menu->putOffVoiceStatus == 5)) || (subType == rxPutOffVoice && (menu->putOffVoiceStatus == 2)))
            drawMenu();
    }

#endif
}

void Service::updateHSState(Headset::Controller::SmartHSState state)
{
    QM_UNUSED(state);

//    std::string str;
//    switch(state)
//    {
//		case Headset::Controller::SmartHSState_SMART_EMPTY_MESSAGE: str = "SmartHSState_SMART_EMPTY_MESSAGE"; break;
//		case Headset::Controller::SmartHSState_SMART_NOT_CONNECTED: str = "SmartHSState_SMART_NOT_CONNECTED"; break;
//		case Headset::Controller::SmartHSState_SMART_ERROR: str = "SmartHSState_SMART_ERROR"; break;
//		case Headset::Controller::SmartHSState_SMART_BAD_CHANNEL: str = "SmartHSState_SMART_BAD_CHANNEL"; break;
//		case Headset::Controller::SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL: str = "SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL"; break;
//		case Headset::Controller::SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE: str = "SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE"; break;
//		case Headset::Controller::SmartHSState_SMART_RECORD_DOWNLOADING: str = "SmartHSState_SMART_RECORD_DOWNLOADING"; break;
//		case Headset::Controller::SmartHSState_SMART_PLAYING: str = "SmartHSState_SMART_PLAYING"; break;
//		case Headset::Controller::SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL: str = "SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL"; break;
//		case Headset::Controller::SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE: str = "SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE"; break;
//		case Headset::Controller::SmartHSState_SMART_RECORDING: str = "SmartHSState_SMART_RECORDING"; break;
//		case Headset::Controller::SmartHSState_SMART_RECORD_UPLOADING: str = "SmartHSState_SMART_RECORD_UPLOADING"; break;
//		case Headset::Controller::SmartHSState_SMART_RECORD_TIMEOUT: str = "SmartHSState_SMART_RECORD_TIMEOUT"; break;
//		case Headset::Controller::SmartHSState_SMART_READY: str = "SmartHSState_SMART_READY"; break;
//    }
//    qmDebugMessage(QmDebug::Warning, "%s", str.c_str());

    CState currentState;
    guiTree.getLastElement(currentState);

    if (currentState.getType() == endMenuWindow)
    {
        static bool isUploaded = false;

        bool isRecord = false;
        GuiWindowsSubType subType = ((CEndState&)guiTree.getCurrentState()).subType;
        if ((subType == txPutOffVoice) && (menu->putOffVoiceStatus == 2))
        {
            if (state == Headset::Controller::SmartHSState::SmartHSState_SMART_RECORD_UPLOADING)
            {
                isUploaded = true;
            }
            else
            {
                if (isUploaded && state == Headset::Controller::SmartHSState::SmartHSState_SMART_READY)
                    isRecord = true;
            }
        }

        if ( (subType == txPutOffVoice && (menu->putOffVoiceStatus == 2)) || (subType == rxPutOffVoice && (menu->putOffVoiceStatus == 5)))
        {
            if (isRecord)
            {
                Multiradio::voice_message_t message = headset_controller->getRecordedSmartMessage();
              //  if (storageFs > 0)
              //      storageFs->writeMessage(DataStorage::FS::FT_VM, DataStorage::FS::TFT_TX, &message);
                isUploaded = false;
                drawMenu();
            }
            else
            {
                drawMenu();
            }
        }
    }
}

void Service::TxCondCmdPackage(int value)
{
    if (value == 30)
    {
    	menu->TxCondCmdPackage(0);
    	if (setAsk)
    	{
    		isWaitAnswer = true;
    	}
    	else
    	{
			guiTree.resetCurrentState();
			menu->txCondCmdStage = 0;
    	}
    	draw();
    }
    else
    {
        menu->TxCondCmdPackage(value);
        menu->txCondCmdStage = 6;
        menu->initCondCommDialog((CEndState&)guiTree.getCurrentState());
    }
}

std::vector<uint8_t>* Service::loadVoiceMail(uint8_t fileNumber, DataStorage::FS::TransitionFileType tft)
{
    uint8_t result = 0; // ok
    if (storageFs > 0){
        fileMsg.clear();

        int channelNum;
        Multiradio::voice_channel_t type;
        headset_controller->getSmartCurrentChannel(channelNum, type);

        result = voice_service->playVoiceMessage(fileNumber, tft, channelNum );
    }

    std::string stateStr;
    std::string errorReadStr(errorReadFile);
    std::string errorSpeakerOffStr(smatrHSStateStr[1]);

    switch (result){
        case 1: stateStr = errorReadStr; break;
        case 2: stateStr = errorSpeakerOffStr; break;
    }

    if (result != 0)
    {
        fileMsg.resize(stateStr.size());
        memcpy(fileMsg.data(),&stateStr[0],stateStr.size());
    }
    else
    {
        stateStr = (std::string)smatrHSStateStr[5];
        fileMsg.resize(stateStr.size());
        memcpy(fileMsg.data(),&stateStr[0],stateStr.size());
    }
    return &fileMsg;
}

std::vector<uint8_t>* Service::loadMessage(DataStorage::FS::FileType typeF, DataStorage::FS::TransitionFileType tft, uint8_t fileNumber)
{
    bool result = false;
    if (storageFs > 0){
        fileMsg.clear();
        result = storageFs->readMessage(typeF,tft,&fileMsg,fileNumber);
    }
    if (!result)
    {
        std::string errorReadStr(errorReadFile);
        fileMsg.resize(errorReadStr.size());
        memcpy(fileMsg.data(),&errorReadStr[0],errorReadStr.size());
    }
    return &fileMsg;
}

void Service::showMessage(const char *title, const char *text)
{
    MoonsGeometry area = {15,62,140,125};
    GUI_Dialog_MsgBox::showMessage(&area, true, title, text);
}

void Service::showMessage(const char *title, const char *text, MoonsGeometry area)
{
    GUI_Dialog_MsgBox::showMessage(&area, true, title, text);
}

void Service::startSchedulePromptTimer()
{
	isShowSchedulePrompt = true;
	schedulePromptRedrawTimer.setSingleShot(true);
	schedulePromptRedrawTimer.start(6000);
}

void Service::stopSchedulePromptTimer()
{
	isShowSchedulePrompt = false;
	draw();
}

void Service::showSchedulePrompt(DataStorage::FS::FileType fileType, uint16_t minutes)
{
    char min[5];
    sprintf((char*)&min, "%d", minutes);
    std::string text =
        std::string(min) +
        std::string(schedulePromptStr) +
        std::string(tmpParsing[fileType]);

    showMessage("",text.c_str(), promptArea);

    schedulePromptText = text;
    playSchedulePromptSignal();
    startSchedulePromptTimer();
}

// create list of sessions
// call on schedule changes
void Service::updateSessionTimeSchedule()
{
    uint8_t sessionCount = sheldure.size();

    if (sessionCount){

        uint8_t sessionTimeHour = 0;
        uint8_t sessionTimeMinute = 0;

        sessionList.clear();

        ScheduleTimeSession timeSession;
        for (uint8_t session = 0; session < sessionCount; session++){

            sessionTimeHour   = atoi(sheldure[session].time.substr(0,2).c_str());
            sessionTimeMinute = atoi(sheldure[session].time.substr(3,2).c_str());

            timeSession.index = session;
            timeSession.type = (DataStorage::FS::FileType)sheldure[session].type;
            timeSession.time = sessionTimeHour * 60 + sessionTimeMinute;

            uint8_t insertIndex = 0;

            if (sessionList.size() == 0)
                sessionList.push_back(timeSession);
            else
            {
                for (uint8_t sessionTime = 0; sessionTime < sessionList.size(); sessionTime++, insertIndex++){
                    if (timeSession.time < sessionList.at(sessionTime).time)
                        break;
                }
            	sessionList.insert(sessionList.begin() + insertIndex, timeSession);
            }
        }

        calcNextSessionIndex();
    } else
        schedulePromptTimer.stop();
}

void Service::calcNextSessionIndex()
{
    uint8_t curTimeHour = 0;
    uint8_t curTimeMinute = 0;
    uint8_t curTimeSecond = 0;

    getCurrentTime(&curTimeHour, &curTimeMinute, &curTimeSecond);

    uint16_t curTimeInMinutes = curTimeHour * 60 + curTimeMinute;

    for (uint8_t sessionTime = 0; sessionTime < sessionList.size(); sessionTime++){
        if (curTimeInMinutes > sessionList.at(sessionTime).time)
           continue;
        else
        {
           nextSessionIndex = sessionTime;
           break;
        }
    }
    onScheduleSessionTimer();
}

void Service::onScheduleSessionTimer()
{
    uint8_t curTimeHour = 0;
    uint8_t curTimeMinute = 0;
    uint8_t curTimeSecond = 0;

    getCurrentTime(&curTimeHour, &curTimeMinute, &curTimeSecond);

    uint16_t curTimeInMinutes = curTimeHour * 60 + curTimeMinute;

    uint16_t deltaTime = 0;
    if (nextSessionIndex == 0 && curTimeInMinutes > sessionList.at(0).time)
    	deltaTime = 24 * 60 - curTimeInMinutes + sessionList.at(nextSessionIndex).time; // timeTo0Hour + sessionTime
    else
    	deltaTime = sessionList.at(nextSessionIndex).time - curTimeInMinutes;

    if (deltaTime < 11){
        showSchedulePrompt(sessionList.at(nextSessionIndex).type, deltaTime);

        nextSessionIndex++;
        if (nextSessionIndex == sessionList.size())
           nextSessionIndex = 0; // cyclic
        if (sessionList.size() > 1){
            schedulePromptTimer.setInterval(2000);
            schedulePromptTimer.start();
        }
        return;
    }
    if (deltaTime <= 15){
        showSchedulePrompt(sessionList.at(nextSessionIndex).type, deltaTime);

        schedulePromptTimer.setInterval((deltaTime - 10)*60000);
        schedulePromptTimer.start();
        return;
    }
    schedulePromptTimer.setInterval((deltaTime - 15)*60000);
    schedulePromptTimer.start();
}

void Service::getCurrentTime(uint8_t* hour, uint8_t* minute, uint8_t* second)
{
#ifndef _DEBUG_
        uint8_t* time;
        Navigation::Coord_Date date = navigator->getCoordDate();

        isValidGpsTime = date.status;
        if ( voice_service->getVirtualMode() == true || (voice_service->getVirtualMode() == false && isValidGpsTime == false))
        {
        	time = voice_service->getVirtualTime();
            *hour   = (time[0] - 48) * 10 + (time[1] - 48);
            *minute = (time[2] - 48) * 10 + (time[3] - 48);
            *second = (time[4] - 48) * 10 + (time[5] - 48);
        }
        else
        {
        	*hour   = (date.time[0]-48)*10 + (date.time[1]-48);
        	*minute = (date.time[2]-48)*10 + (date.time[3]-48);
        	*second = (date.time[4]-48)*10 + (date.time[5]-48);
        }
 #endif
}

void Service::loadSheldure()
{
#ifndef _DEBUG_
   if (storageFs > 0){
       if (sheldureMass == 0)
          sheldureMass = new uint8_t[651];

       if (storageFs->getSheldure(sheldureMass)){
         sheldureParsing(sheldureMass);
         schedulePromptTimer.timeout.connect(sigc::mem_fun( this, &Service::onScheduleSessionTimer));
         schedulePromptRedrawTimer.timeout.connect(sigc::mem_fun( this, &Service::stopSchedulePromptTimer));
         if (isDspStarted)
            updateSessionTimeSchedule();
       }

       if (sheldureMass > 0){
            delete []sheldureMass;
            sheldureMass = 0;
       }
   }
#else
    if (sheldureMass == 0)
       sheldureMass = new uint8_t[651];

    uint8_t massTemp[] =
    {0x32,'0', '1','0',':','3','2',':','0','0',0x00,0x44,0xec,0x88};

    for (uint8_t i = 0; i < 5; i++)
     memcpy(&sheldureMass[1+i*13], &massTemp[1], 13);
    sheldureMass[0] = 5;

    sheldureParsing(sheldureMass);

    if (sheldureMass > 0){
         delete []sheldureMass;
         sheldureMass = 0;
    }
#endif
    sheldureToStringList();
}

void Service::uploadSheldure()
{
#ifndef _DEBUG_
    if (storageFs > 0){
        if (sheldureMass == 0)
           sheldureMass = new uint8_t[1 + sheldure.size() * 13];

        sheldureUnparsing(sheldureMass);
        storageFs->setSheldure(sheldureMass, sheldure.size() * 13 + 1);

        if (sheldureMass > 0){
             delete []sheldureMass;
             sheldureMass = 0;
        }       
    }
    updateSessionTimeSchedule();
#endif
}

void Service::msgGucTXQuit(int ans)
{
    if (ans != -1)
    {
    	char a[3]; a[2] = '\0';
    	sprintf(a,"%d",ans);
        msgBox( gucQuitTextOk, ans);
    }
    else
    {
        msgBox( "Guc", gucQuitTextFail);
    }
    isGucAnswerWaiting = false;
	menu->groupCondCommStage = 0;
	menu->focus = 0;
	guiTree.resetCurrentState();
}

void Service::sheldureParsing(uint8_t* sMass)
{
    if (sMass[0] > 0 && sMass[0] <= 50)
    {
        uint8_t sheldureSize = sMass[0];

        for(uint8_t i = 0; i < sheldureSize; i++)
        {
            tempSheldureSession.clear();

            // --------- type ---------

            tempSheldureSession.type = (DataStorage::FS::FileType)(sMass[ 1 + (i * 13) ] - 48);

            // --------- time ---------

            for(uint8_t j = 0; j < 5; j++)
                tempSheldureSession.time.push_back(sMass[ 2 + (i*13) + j]);

            // --------- freq ---------

            uint32_t frec = 0;
            for(uint8_t k = 0; k < 4; k++)
              frec += (uint8_t)(sMass[ 10 + (i*13) + k]) << (3-k)*8;

            char ch[8];
            sprintf(ch,"%d",frec);
            for(uint8_t j = 0; j < 7; j++)
                tempSheldureSession.freq.push_back(ch[j]);

            sheldure.push_back(tempSheldureSession);
        }
    }
}

void Service::sheldureUnparsing(uint8_t* sMass)
{
    if (storageFs > 0){

        sMass[0] = sheldure.size();

        for (uint8_t session = 0; session < sMass[0]; session++)
        {
            // ---------- type ----------

            sMass[ 1 + (session * 13) ] = sheldure.at(session).type + 48;

            // ---------- time ----------

            std::string sec = ":00";
            memcpy(&sMass[ 1 + 1 + (session * 13)], &sheldure.at(session).time[0], 5);
            memcpy(&sMass[ 1 + 1 +(session * 13) + 5], &sec[0], 3);

            // ---------- freq ----------

            uint32_t freq = atoi(sheldure.at(session).freq.c_str());
            for(int i = 3; i >= 0; i--)
              sMass[1 + session * 13 + 9 + (3 - i)] = freq >> 8 * i;
        }
    }
}

void Service::sheldureToStringList()
{
    sheldure_data.clear();

    uint8_t sheldureSize = sheldure.size();

     if (sheldureSize > 0 && sheldureSize <= 50)
     {
         for (uint8_t session = 0; session < sheldureSize; session++)
         {
             std::string s;

             // --------- type -----------

             uint8_t typeMsg = (uint8_t)sheldure[session].type;
             s.append(tmpParsing[typeMsg]);
             (sheldure[session].type % 2 == 0) ? s.append("  ") : s.append("   ");

              // --------- time -----------

             s.append(sheldure[session].time).append("\n ");

             // --------- freq -----------

             s.append(sheldure[session].freq).append(freq_hz);

             sheldure_data.push_back(s);
         }
     }
     if(sheldureSize < 50)
        sheldure_data.push_back(addSheldure);
}

void Service::setFreq()
{
	int freq = atoi(main_scr->oFreq.c_str());
    voice_service->tuneFrequency(freq);
}

void Service::playSoundSignal(uint8_t mode, uint8_t speakerVolume, uint8_t gain, uint8_t soundNumber, uint8_t duration, uint8_t micLevel)
{
   voice_service->playSoundSignal(mode, speakerVolume, gain, soundNumber, duration, micLevel);
}

void Service::playSchedulePromptSignal()
{

	voice_service->playSoundSignal(4, 100, 100, 2, 200, 100);
}

void Service::onCompletedStationMode(bool isGoToVoice)
{
	if (isGoToVoice)
		voice_service->goToVoice();

	voice_service->stopGucQuit();

	Headset::Controller::Status st = pGetHeadsetController()->getStatus();

    if (st ==  Headset::Controller::Status::StatusSmartOk || st == Headset::Controller::Status::StatusAnalog)
    {
        setFreq();
        garnitureStart();
    }

    menu->qwitCounter = 0;
    // qwit tx exit menu

    if (isCondModeQwitTx || isGucModeQwitTx)
    {
    	isCondModeQwitTx = false;
    	isGucModeQwitTx = false;

		menu->virtCounter = 0;
		menu->recvStage = 0;
		cntGucRx = -1;
		guiTree.resetCurrentState();
		draw();
    }
}

void Service::onDspStarted()
{
    isDspStarted = true;
    updateSessionTimeSchedule();
    checkHeadsetStatus();
}

void Service::getBatteryVoltage()
{
    if (pGetPowerBattery()->getChargeLevel()){ // if is battery(not power block)
        bool success = false;
        power_battery->requireVoltage(&success);
    }
}

void Service::onRecievingBatteryVoltage(int voltage)
{
    voice_service->sendBatteryVoltage(voltage);
}


void Service::garnitureStart()
{
	headset_controller->GarnitureStart();
}

void Service::onWaveInfoRecieved(float wave, float power)
{
    weveValue = wave;
    powerValue = power;
    qmDebugMessage(QmDebug::Warning, "SWR = %f, POWER = %f ", weveValue, powerValue);
#if PARAMS_DRAW
    //if (weveValue > 0 && powerValue > 0)
    	drawWaveInfo();
#endif
}

void Service::onRxModeSetting()
{
    curMode = 1;
}

void Service::onTxModeSetting()
{
    curMode = 2;
}

void Service::onSettingAleFreq(uint32_t freq)
{
    curAleFreq = freq;
}

void Service::drawWaveInfo()
{
    if (guiTree.getCurrentState().getType() == mainWindow && msg_box == nullptr )
    {
		if (multiradioStatus == Multiradio::VoiceServiceInterface::Status::StatusVoiceTx && (weveValue > 0.000 && powerValue > 0.000))
		{
			MoonsGeometry objArea = {  0, 0, 159, 127 };
			MoonsGeometry windowArea = {  90, 0, 159, 40 };
			//MoonsGeometry txrxArea   = {  0, 0,  10, 20 };
			MoonsGeometry waveArea   = { 105, 0,  150, 16 };
			MoonsGeometry powerArea  = {105, 16, 150, 32 };

			//std::string rxtxStr(curMode == 1 ? "Rx" : "Tx");

			char var[5] = {0,0,0,0,0};
			sprintf(var,"%03.1f",weveValue);
			//var[3] = 0;
			std::string waveStr("S: " + std::string(var));
			memset(&var, 0, 5);
			sprintf(var,"%03.1f",powerValue);
			//var[3] = 0;
			std::string powerStr("P: " + std::string(var));

			GUI_Obj obj(&objArea);

			LabelParams param = GUI_EL_TEMP_LabelTitle;
			param.element.align.align_h = alignLeft;

			GUI_EL_Window window     (&GUI_EL_TEMP_WindowGeneral, &windowArea,                         (GUI_Obj *)&obj);
		   // GUI_EL_Label  rxtxLabel  (&GUI_EL_TEMP_LabelTitle,    &txrxArea,   (char*)rxtxStr.c_str(), (GUI_Obj *)this);
			GUI_EL_Label  waveLabel  (&param,    &waveArea,   (char*)waveStr.c_str(), (GUI_Obj *)&obj);
			GUI_EL_Label  powerLabel (&param,    &powerArea,  (char*)powerStr.c_str(),(GUI_Obj *)&obj);

			window.Draw();
			//rxtxLabel.Draw();
			waveLabel.Draw();
			powerLabel.Draw();
		}
		else
		{
			MoonsGeometry windowArea = { 90, 0, 159, 40 };
			MoonsGeometry objArea = {  0, 0, 159, 127 };
			GUI_Obj obj(&objArea);

			GUI_EL_Window window     (&GUI_EL_TEMP_WindowGeneral, &windowArea,                         (GUI_Obj *)&obj);
			window.Draw();

//			MoonsGeometry windowArea2 = {  0, 0, 15, 25 };
//			GUI_EL_Window window2     (&GUI_EL_TEMP_WindowGeneral, &windowArea2,                         (GUI_Obj *)&obj);
//			window2.Draw();
		}
    }
}

void Service::onStartCondReceiving()
{
    isStartCond = true;
	draw();
}

void Service::onVirtualCounterChanged(uint8_t counter)
{
	menu->virtCounter = counter + 1;
	if (menu->virtCounter > 120)
		menu->virtCounter = 120;
    qmDebugMessage(QmDebug::Warning, "_____virtual counter = %d ", menu->virtCounter);
	draw();
}

void Service::onTransmitAsk(bool on)
{
	menu->isTransmitAsk = on;
	draw();
}

void Service::onQwitCounterChanged(uint8_t counter, uint8_t all)
{
	menu->qwitCounter = counter;
	menu->qwitCounterAll = all;

    qmDebugMessage(QmDebug::Warning, "____qwitCounter    = %d ", menu->qwitCounter);
    qmDebugMessage(QmDebug::Warning, "____qwitCounterAll = %d ", menu->qwitCounterAll);

    //if (isCondModeQwitTx)
    CState state = guiTree.getCurrentState();
    if(state.getType() != messangeWindow)
    	draw();
}

}/* namespace Ui */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(service, LevelDefault)
#include "qmdebug_domains_end.h"
