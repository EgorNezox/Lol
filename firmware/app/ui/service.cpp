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

    this->headset_controller->delaySpeachStateChanged.connect(sigc::mem_fun(this, &Service::onDelaySpeachStateChanged));


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
		case mainWindow:     mainWindow_keyPressed(key);     break;
		case messangeWindow: messangeWindow_keyPressed(key); break;
		case menuWindow:     menuWindow_keyPressed(key);     break;
		case endMenuWindow:  endMenuWindow_keyPressed(key);  break;
    }
    draw();
}


int Service::getLanguage()
{
    return 0;
}

void Service::onSmsCounterChange(int param)
{
  //  qmDebugMessage(QmDebug::Warning, "______sms counter: %d ", param);
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

            GUI_Painter::ClearViewPort(true);
            showMessage(waitingStr, flashProcessingStr, promptArea);
            storageFs->writeMessage(DataStorage::FS::FT_CND, DataStorage::FS::TFT_RX, &fileMsg);
            draw();
        }

         //guiTree.append(messangeWindow, "Принятый пакет ", sym);
         condCmdValue = packet;
         isDrawCondCmd = true;

		 char addressStr[4] = {0,0,0,0};
		 sprintf(addressStr, "%d", address);

		 std::string str;
         if (isRec)
         {
			 if (setAsk)
			 {
				 guiTree.resetCurrentState();
				 menu->txCondCmdStage = 0;
				 isWaitAnswer = false;

				 str.append(cmdRec);
				 setAsk = false;
			 }
			 else
			 {
				 str.append(recPacket);
				 isCondModeQwitTx = true;
			 }
         }
         else
         {
			 str.append(notReiableRecPacket);
         }
		 str.append(" ").append(fromStr).append(" ").append(addressStr);
    	 msgBox(  str.c_str(), (int)packet );
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
	navigator->set1PPSModeCorrect(true);

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
				case 2: (*iter)++; // address retr
				case 4: (*iter)++; // message
				case 3:            // address dst
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
						fieldStr.append("\t").append(syncWaitingStr).append("\n ").append(syn).append(" / 120");
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
            if (cntGucRx == -1)
            {
        		cntGucRx = 0;
        		keyPressed(keyEnter);
        	}
            if (cntGucRx == 4)
            {
				menu->initRxSmsDialog(txQwit,10);
            	//keyPressed(keyEnter);
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
            }

            menu->vmProgress = voice_service->getAleVmProgress();
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
            			GUI_Painter::ClearViewPort(true);
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
            	menu->vmProgress = 0;
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

    navigator->set1PPSModeCorrect(false);

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

        GUI_Painter::ClearViewPort(true);
        GUI_Painter::DrawRect(0, 0, 159, 127, RDM_FILL);

        GUI_Painter::DrawText(35, 15, GUI_EL_TEMP_CommonTextAreaLT.font,(char*)radioStationStr);
        GUI_Painter::DrawText(35, 30, GUI_EL_TEMP_CommonTextAreaLT.font,(char*)str.c_str());
        GUI_Painter::DrawText(35, 65, GUI_EL_TEMP_CommonTextAreaLT.font,(char*)true_SWF);
    }
}

int Service::getFreq()
{
	int freq = atoi(main_scr->oFreq.c_str());
    return freq;
}
//
//void Service::setFreq(int isFreq)
//{
//    Service::isFreq = isFreq;
//}

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

            GUI_Painter::ClearViewPort(true);
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
    {
    	cntGucRx = 4;
    	isGucModeQwitTx = true;
    }
    else
    {
    	cntGucRx = 1;
    }

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
        GUI_Painter::ClearViewPort(true);
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

        if (channelNum % 2 == 0)
        result = voice_service->playVoiceMessage(fileNumber, tft, channelNum );
        else
        result = 1;
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

       schedulePromptTimer.timeout.connect(sigc::mem_fun( this, &Service::onScheduleSessionTimer));
       schedulePromptRedrawTimer.timeout.connect(sigc::mem_fun( this, &Service::stopSchedulePromptTimer));

       if (storageFs->getSheldure(sheldureMass))
       {
         sheldureParsing(sheldureMass);
         if (isDspStarted)
            updateSessionTimeSchedule();
       }

       if (sheldureMass > 0)
       {
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
    if (storageFs > 0)
    {
        GUI_Painter::ClearViewPort(true);
        showMessage(waitingStr, flashProcessingStr, promptArea);

        if (sheldureMass == 0)
           sheldureMass = new uint8_t[1 + sheldure.size() * 13];

        sheldureUnparsing(sheldureMass);
        storageFs->setSheldure(sheldureMass, sheldure.size() * 13 + 1);

        draw();

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
	voice_service->stopGucQuit();

	Headset::Controller::Status st = pGetHeadsetController()->getStatus();

    if (st ==  Headset::Controller::Status::StatusSmartOk || st == Headset::Controller::Status::StatusAnalog)
    {
        setFreq();
        garnitureStart();
    }

	if (isGoToVoice)
		voice_service->goToVoice();

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
//    qmDebugMessage(QmDebug::Warning, "SWR = %f, POWER = %f ", weveValue, powerValue);
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
 //   qmDebugMessage(QmDebug::Warning, "_____virtual counter = %d ", menu->virtCounter);
	draw();
}

void Service::onTransmitAsk(bool on)
{
	menu->isTransmitAsk = on;
	draw();
}

void Service::onQwitCounterChanged(uint8_t counter)
{
	menu->qwitCounter = counter;
	//menu->qwitCounterAll = all;

   // qmDebugMessage(QmDebug::Warning, "____qwitCounter    = %d ", menu->qwitCounter);
   // qmDebugMessage(QmDebug::Warning, "____qwitCounterAll = %d ", menu->qwitCounterAll);

    //if (isCondModeQwitTx)
    CState state = guiTree.getCurrentState();
    if(state.getType() != messangeWindow)
    	draw();
}

void Service::onDelaySpeachStateChanged(bool isOn)
{
	CEndState estate = (CEndState&)guiTree.getCurrentState();
	if ((not isOn) and (estate.subType == txPutOffVoice) and (menu->putOffVoiceStatus == 2))
	{
		menu->putOffVoiceStatus = 1;
		menu->toVoiceMail = false;
	    draw();
	}
}

}/* namespace Ui */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(service, LevelDefault)
#include "qmdebug_domains_end.h"
