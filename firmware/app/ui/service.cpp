
#include "qm.h"
#define QMDEBUGDOMAIN	service
#include "qmdebug.h"
#include "dialogs.h"
#include "service.h"
#include "texts.h"
#include "../navigation/navigator.h"
#include "../../../system/reset.h"
#include "gui_tree.h"
#include "../../system/usb_cdc.h"

#define VM_STATE 1
#define VM_PROGRESS 1
#define SMS_PROGRESS 1
#define TIME_ON_GPS_MARKER 0

MoonsGeometry ui_common_dialog_area = { 0,24,GDISPW-1,GDISPH-1 };
MoonsGeometry ui_menu_msg_box_area  = { 1,1,GDISPW-2,GDISPH-2 };

MoonsGeometry ui_indicator_area     = { 0,0,95,30};

namespace Ui {

bool Service::single_instance = false;

Service::Service( matrix_keyboard_t                  matrixkb_desc,
                  aux_keyboard_t                     auxkb_desc,
                  Headset::Controller               *headset_controller,
                  Multiradio::VoiceServiceInterface *mr_voice_service,
                  Power::Battery                    *power_battery,
                  Navigation::Navigator             *navigator,
                  DataStorage::FS                   *fs,
				  Multiradio::usb_loader            *usb)
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

    keyboard  = new QmMatrixKeyboard(matrix_kb.resource);
    chnext_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChNext]);
    chprev_bt = new QmPushButtonKey(aux_kb.key_iopin_resource[auxkbkeyChPrev]);

    keyboard->keyAction.connect(sigc::mem_fun(this, &Service::keyHandler));
    keyboard->keyStateChanged.connect(sigc::mem_fun(this, &Service::keyChangeHandler));
    chnext_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chNextHandler));
    chprev_bt->stateChanged.connect(sigc::mem_fun(this, &Service::chPrevHandler));

    mainWindowShowTimer = new QmTimer();
    mainWindowShowTimer->setInterval(1000);
    mainWindowShowTimer->setSingleShot(false);
    mainWindowShowTimer->timeout.connect(sigc::mem_fun(this, &Service::onHideMainWindow));

    main_scr  = new GUI_Dialog_MainScr(&ui_common_dialog_area);
    indicator = new GUI_Indicator     (&ui_indicator_area);

    menu = nullptr; msg_box = nullptr; bool useMode = false;

    if( menu == nullptr )
        menu = new CGuiMenu(&ui_menu_msg_box_area, mainMenu[0], {alignHCenter,alignTop});

    if (storageFs > 0)
    {
    	menu->setFS(storageFs);
    	storageFs->getVoiceMode(&useMode);
    }

    menu->useMode = (bool)useMode;

	if (fs->getTimeZone(menu->UtcStatusStatus) == false)
		menu->UtcStatusStatus = 0;

    voice_service->setVoiceMode((Multiradio::VoiceServiceInterface::VoiceMode)!menu->useMode);

    voice_service->command_tx30.connect(sigc::mem_fun(this, &Service::TxCondCmdPackage));
    voice_service->rxRssiLevel.connect(sigc::mem_fun(this, &Service::RxRssi));
    voice_service->aleStateChanged.connect(sigc::mem_fun(this, &Service::updateAleState));
    voice_service->aleVmProgressUpdated.connect(sigc::mem_fun(this, &Service::updateAleVmProgress));
    voice_service->statusChanged.connect(sigc::mem_fun(this, &Service::updateMultiradio));
    voice_service->firstPacket.connect(sigc::mem_fun(this,&Service::FirstPacketPSWFRecieved));
    voice_service->smsMess.connect(sigc::mem_fun(this,&Service::smsMessage));
    voice_service->smsFailed.connect(sigc::mem_fun(this,&Service::FailedSms));
    voice_service->respGuc.connect(sigc::mem_fun(this,&Service::gucFrame));
    voice_service->atuMalfunction.connect(sigc::mem_fun(this, &Service::showAtuMalfunction));
    voice_service->dspHardwareFailed.connect(sigc::mem_fun(this, &Service::showDspHardwareFailure));
    voice_service->messageGucTxQuit.connect(sigc::mem_fun(this, &Service::msgGucTXQuit));
    voice_service->gucCrcFailed.connect(sigc::mem_fun(this,&Service::errorGucCrc));
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

    power_battery->voltageChanged.connect(sigc::mem_fun(this, &Service::batteryVoltageChanged));
    power_battery->chargeLevelChanged.connect(sigc::mem_fun(this, &Service::batteryChargeChanged));

    headset_controller->statusChanged.connect(sigc::mem_fun(this, &Service::updateHeadset));
    headset_controller->smartHSStateChanged.connect(sigc::mem_fun(this, &Service::updateHSState));
    headset_controller->delaySpeachStateChanged.connect(sigc::mem_fun(this, &Service::onDelaySpeachStateChanged));
    //headset_controller->BOOM.connect(sigc::mem_fun(this, &Service::resetLogicDSPforGarniture));

    voice_service->emulKey.connect(sigc::mem_fun(this, &Service::emulkeyHandler));


    voice_service->smsFreq.connect(sigc::mem_fun(this, &Service::showFreq));


    valueRxSms = 0;  command_rx_30 = 0;
    pswf_status = false;

#ifndef PORT__PCSIMULATOR
    navigator->PswfSignal.connect(sigc::mem_fun(this,&Service::setPswfStatus));
#if TIME_ON_GPS_MARKER
    navigator->syncPulse.connect(sigc::mem_fun(this,&Service::updateSystemTime));
#else
    systemTimeTimer = new QmTimer(true);
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

#if EMUL
    Navigation::Coord_Date data;
    memset( &data.latitude,  0, 11 );
    memset( &data.longitude, 0, 11 );

    data.data[0] = 49;
    data.data[1] = 50;
    data.data[2] = 49;
    data.data[3] = 50;
    data.data[4] = 49;
    data.data[5] = 50;

    data.time[0] = 49;
    data.time[1] = 50;
    data.time[2] = 50;
    data.time[3] = 50;
    data.time[4] = 51;
    data.time[5] = 52;

    setCoordDate(data );
#endif

    this->usb_service = usb;
    if (navigator)
        navigator->setGPSTuneFlag(true);

    draw();
}

Service::~Service()
{
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

void Service::onHideMainWindow()
{
	if (!keyboard->isKeyPressed(1))
	{
		isMainMenuKeyPressed = false;
		mainWindowShowTimer->stop();
		isDrawMainWindow = false;
		if (isDrawOnHideMainMenu)
		{
			isRedrawOnHideMainWindow = true;
			draw();
			isRedrawOnHideMainWindow = false;
		}
	}
}

void Service::showFreq(int freq)
{
   menu->currentFrequency = freq;
}

void Service::emulkeyHandler(int key)
{
	keyHandler(key,QmMatrixKeyboard::PressType::PressSingle);
#if TRACE_DISPLAY_TO_PORT
	draw_emulate();
#endif
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
    if (storageFs > 0)
    {
       storageFs->getGpsSynchroMode((uint8_t*)&gpsSynchronization);
       voice_service->setVirtualMode(!gpsSynchronization);
    }
}

void Service::startRxQuit()
{
	if (menu->sndMode)
		isGucAnswerWaiting = false;
	else
		isGucAnswerWaiting = true;

	if (!isGucAnswerWaiting)
	{
		menu->groupCondCommStage = 0;
		guiTree.resetCurrentState();
	}

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
	if ((subdevice_code == 7) && (error_code == 5))
	{
		title = dsphardwarefailure_7_5_title_str;
		char text_buffer[50] = {'\0'};
		sprintf(text_buffer , dsphardwarefailure_wave_power, waveValue, powerValue);
		text = text_buffer;

	}
	else
	{
		title = dsphardwarefailure_unknown_title_str;
		char text_buffer[50];
		sprintf(text_buffer , dsphardwarefailure_unknown_text_str, subdevice_code, error_code);
        text = text_buffer;
	}
	msgBox(title.c_str(), text.c_str());
}

void Service::errorGucCrc()
{
    msgBox( err, errCrc);
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
//    if (multiradioStatus == Multiradio::VoiceServiceInterface::Status::StatusVoiceTx)
//    	voice_service->setSwrTimerState(true);
//    else
//    	voice_service->setSwrTimerState(false);
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


void Service::setNotification(NotificationType type)
{
    switch(type)
    {
    case NotificationMissingVoiceChannelsTable:
        msgBox(missing_ch_table_txt[0]);
        break;
    case NotificationMissingOpenVoiceChannels:
        msgBox(missing_open_ch_txt[0]);
        break;
    case NotificationMismatchVoiceChannelsTable:
        msgBox(ch_table_mismatch_txt[0]);
        break;
    default:
        QM_ASSERT(0);
        break;
    }
}

void Service::keyHandler(int key_id, QmMatrixKeyboard::PressType pr_type)
{
    QM_UNUSED(pr_type);

    isDrawOnHideMainMenu = false;
    onHideMainWindow();
    isDrawOnHideMainMenu = true;

	if (UI_Key::keyUp == (UI_Key)matrix_kb.key_id[key_id])
	{
    	isMainMenuKeyPressed = true;
    	mainWindowShowTimer->start();
	}
    keyPressed((UI_Key)matrix_kb.key_id[key_id]);
}

void Service::keyChangeHandler(int key_id, bool isPress)
{
//	if (UI_Key::key0 == (UI_Key)matrix_kb.key_id[key_id])
//	{
//    	isMainMenuKeyPressed = isPress; //keyboard->isKeyPressed(UI_Key::key0);
//    	if (!isPress)
//    	{
//    		isDrawMainWindow = false;
//    		draw();
//    	}
//
//	}
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
        keyPressed(keyChNext);
}

void Service::chPrevHandler()
{
    if(chprev_bt->isPressed())
        keyPressed(keyChPrev);
}

void Service::checkHeadsetStatus()
{
    //  0 - skzi open
    //  1 - polev open
    //  2 - skzi close
    uint8_t headsetType = 0;
    bool chMiss = false;
    if (pGetHeadsetController()->getAnalogStatus(chMiss))
    {
      headsetType = 1;
      voice_service->sendHeadsetType(headsetType);
    }
    else
    {
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

void Service::keyPressed(UI_Key key, bool isDraw)
{
    CState state = guiTree.getCurrentState();

    bool isEndWindow = false;
    switch( state.getType() )
    {
		case mainWindow:     mainWindow_keyPressed(key);     isDrawMainWindow = false; mainWindowShowTimer->stop(); break;
		case messangeWindow: messangeWindow_keyPressed(key); break;
		case menuWindow:     menuWindow_keyPressed(key);     break;
		case endMenuWindow:  endMenuWindow_keyPressed(key);  isEndWindow = true; break;
    }

    bool isTune = (state.getType() != endMenuWindow);
    if (navigator)
        navigator->setGPSTuneFlag(isTune);

    if (isEndWindow)
    {
    	bool isDrawMw = false;

    	if (estate.subType == condCommand)
    		isDrawMw = true;
    	if (estate.subType == recvCondCmd)
    		isDrawMw = true;
    	if (estate.subType == txGroupCondCmd && menu->groupCondCommStage != 4)
    		isDrawMw = true;
    	if (estate.subType == recvGroupCondCmd && cntGucRx == 4)
    		isDrawMw = true;
    	if (estate.subType == txSmsMessage && menu->smsTxStage != 4)
    		isDrawMw = true;
    	if (estate.subType == rxSmsMessage && cntSmsRx == 2)
    		isDrawMw = true;
    	if (estate.subType == txPutOffVoice)
    		isDrawMw = true;
    	if (estate.subType == rxPutOffVoice)
    		isDrawMw = true;

        if (isDrawMw && isMainMenuKeyPressed)
        {
        	isDrawMainWindow = true;
        }
    }

    if (isDraw)
    	draw();
}

void Service::onSmsCounterChange(int param)
{
  //  qmDebugMessage(QmDebug::Warning, "______sms counter: %d, cntSmsRx: %d smsTxStage: %d", param, cntSmsRx,  menu->smsTxStage);

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

    	valueRxSms = param;
    	if (param > 0 && param < 84)
    	{
    		CState currentState;
    		guiTree.getLastElement(currentState);

    		if (currentState.getType() == endMenuWindow)
    		{
    			GuiWindowsSubType subType = ((CEndState&)guiTree.getCurrentState()).subType;
    			if ((subType == rxSmsMessage) && (cntSmsRx == 2))
    				drawMenu();
    		}
    	}
    	else
    	{
    		if (cntSmsRx != 2)
    			menu->smsTxStage = 6;
    	}
#endif
    }
    drawWaveInfoOnTx();
    //qmDebugMessage(QmDebug::Warning, "______sms smsTxStage: %d ", menu->smsTxStage);
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

            storageFs->writeMessage(DataStorage::FS::FT_CND, DataStorage::FS::TFT_RX, &fileMsg);
            draw();
        }

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

int Service::getFreq()
{
	int freq = atoi(main_scr->oFreq.c_str());
    return freq;
}

void Service::parsingGucCommand(uint8_t *str)
{
    int index = 0;
    char number[4] = {'\0','\0','\0','\0'};

    int len = strlen((const char*)str);
    for(int i = 0; i <= len; i++)
    {
        if ((str[i] == ' ') || (len == i && str[i-1] != ' '))
        {
        	if (isGucFullCmd)
        	{
        		if (i - index == 3)
        			number[3] = '\0';
        	}
            if (i - index == 2)
                number[2] = '\0';
            if (i - index == 1)
                number[1] = '\0';

            memcpy(number, &str[index], i - index);
            guc_command_vector.push_back(atoi(number));

            for (int j = 0; j < 3; j++)
            	number[j] = '\0';
            index = i + 1;
        }
    }
}

void Service::setCoordDate(Navigation::Coord_Date date)
{
	static bool isValidWasExist = false; // ���� �� ������ ���� ��� �������
	if (date.status)
		isValidWasExist = true;

    menu->date.clear();
    menu->time.clear();

    if (atoi((char*)date.latitude) > 0)
    {
    	memcpy(menu->coord_lat,date.latitude,11);
    	memcpy(menu->coord_log,date.longitude,12);
    	menu->isCoordValid = true;
    }
    else
    {
    	menu->isCoordValid = false;
    }

    uint8_t zeroTime[10] = {0};

    uint8_t *time = (uint8_t*)&zeroTime;

    if (isValidWasExist)
    {
    	time = (uint8_t*)&date.time;
    }

	if (voice_service->getVirtualMode() || !isValidWasExist)
	{
		time = voice_service->getVirtualTime();
	}

#if EMUL
    time = (uint8_t*)&date.time;
#endif

    std::string str = "";
    str.resize(9);

    int h = 0;
    h   += (time[0] - 48) * 10;
    h   += (time[1] - 48);

    uint8_t add = menu->UtcStatusStatus;
    if (add > 0)
    {
    	h = (h + add) % 24;
    	time[0] = (char) (h / 10 + 48);
    	time[1] = (char) (h % 10 + 48);
    }


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

void Service::msgBox(const char *title, const int size, const int pos, uint8_t* data, uint16_t dataSize)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(127), (GYT)(127)};

    if (msg_box != nullptr)
        delete msg_box;
    msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, 0, (int) size, (int) pos, align007);

    msg_box->setText(data, dataSize);
    msg_box->position = pos;

    guiTree.append(messangeWindow, "");
    if (!isStartTestMsg)
        msg_box->DrawGuc();
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

    vect = voice_service->getGucCommand();

    isGucCoord = voice_service->getIsGucCoord();
    uint8_t size = vect[0];

    char longitude[20]; longitude[15] = '\n';
    char latitude[20]; latitude[15] = '\0';
    if (isGucCoord)
    {
        sprintf(longitude, "%02d.%02d.\n%02d.%03d N", vect[size+1],vect[size+2],vect[size+3],vect[size+4]);
        sprintf(latitude, "%02d.%02d.\n%02d.%03d E", vect[size+5],vect[size+6],vect[size+7],vect[size+8]);
        memcpy(&gucCoords[0],&longitude[0],15);
        memcpy(&gucCoords[16],&latitude[0],15);
        gucCoords[15] = '\n';
        gucCoords[31] = 0;
    }

    if (vect[0] != 0)
    {
        bool isFullCmd = false; // используются ли трехсимвольные команды
        for (uint8_t cmdSymInd = 1; cmdSymInd <= size; cmdSymInd++)
        {
        	if (vect[cmdSymInd] > 99)
        	{
        		isFullCmd = true;
        		break;
        	}
        }

        uint8_t cmdFactor = isFullCmd ? 4 : 3;
        uint16_t len = size * cmdFactor;
        uint16_t fullSize = isGucCoord ? len + 31 + 10 : len;
        uint8_t cmdv[fullSize] = {'\0'};
        char cmdSym[4];

        for (uint8_t cmdSymInd = 1; cmdSymInd <= size; cmdSymInd++)
        {
        	if (isFullCmd)
        	{
				sprintf(cmdSym, "%03d", vect[cmdSymInd]);
				cmdSym[3] = ' ';
				memcpy(&cmdv[(cmdSymInd - 1) * 4], &cmdSym[0], 4);
        	}
        	else
        	{
				sprintf(cmdSym, "%02d", vect[cmdSymInd]);
				cmdSym[2] = ' ';
				memcpy(&cmdv[(cmdSymInd - 1) * 3], &cmdSym[0], 3);
        	}
        }

        cmdv[fullSize-1] = '\0';

        if (isGucCoord)
        {
            std::string coordStr(coodGucStr);
            uint8_t ssize = coordStr.size();
            coordStr.resize(ssize + 31);
            memcpy(&coordStr[ssize], &gucCoords[0], 31);
            ssize = coordStr.size();
            memcpy(&cmdv[len], &coordStr[0], ssize);
        }

        fileMsg.clear();
        fileMsg.resize(fullSize);
        memcpy(fileMsg.data(), &cmdv, fullSize);

        if (storageFs > 0)
        {
            storageFs->writeMessage(DataStorage::FS::FT_GRP, DataStorage::FS::TFT_RX, &fileMsg);
            draw();
        }

        gucAdd = (uint8_t)value;
        char add[4] = {0,0,0,0};
        sprintf(add,"%d", gucAdd);
        std::string str(titleGuc);
        str.append(" ").append(fromStr).append("#").append(" ").append(add);
        msgBox(str.c_str(), size, 0, (uint8_t*)fileMsg.data(), fileMsg.size());
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
        storageFs->writeMessage(DataStorage::FS::FT_SMS, DataStorage::FS::TFT_RX, &fileMsg);
        //draw();
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
	title.append("#");
	uint8_t sender = voice_service->smsSender();
	if (sender > 0 && sender < 10)
		title += (char) (48 + sender);

	if (sender >=10 && sender < 32)
	{
		title += (char) 48 + (sender / 10);
		title += (char) 48 + (sender % 10);
	}


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

    static Headset::Controller::SmartHSState oldState = Headset::Controller::SmartHSState_SMART_READY;
    static Headset::Controller::SmartHSState newState = state;
    oldState = newState;
    newState = state;
//    std::string str;
    bool isEndPlaying = (oldState == Headset::Controller::SmartHSState_SMART_PLAYING) && (newState == Headset::Controller::SmartHSState_SMART_READY);
//    switch(state)
    std::string str;
    switch(state)
    {
		case Headset::Controller::SmartHSState_SMART_EMPTY_MESSAGE: 					str = "SmartHSState_SMART_EMPTY_MESSAGE"; break;
		case Headset::Controller::SmartHSState_SMART_NOT_CONNECTED: 					str = "SmartHSState_SMART_NOT_CONNECTED"; break;
		case Headset::Controller::SmartHSState_SMART_ERROR: 	    					str = "SmartHSState_SMART_ERROR"; break;
		case Headset::Controller::SmartHSState_SMART_BAD_CHANNEL:   					str = "SmartHSState_SMART_BAD_CHANNEL"; break;
		case Headset::Controller::SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL: 	str = "SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL"; break;
		case Headset::Controller::SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE:   	str = "SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE"; break;
		case Headset::Controller::SmartHSState_SMART_RECORD_DOWNLOADING: 				str = "SmartHSState_SMART_RECORD_DOWNLOADING"; break;
		case Headset::Controller::SmartHSState_SMART_PLAYING: 							str = "SmartHSState_SMART_PLAYING"; break;
		case Headset::Controller::SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL: 	str = "SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL"; break;
		case Headset::Controller::SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE: 	str = "SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE"; break;
		case Headset::Controller::SmartHSState_SMART_RECORDING: 						str = "SmartHSState_SMART_RECORDING"; break;
		case Headset::Controller::SmartHSState_SMART_RECORD_UPLOADING: 					str = "SmartHSState_SMART_RECORD_UPLOADING"; break;
		case Headset::Controller::SmartHSState_SMART_RECORD_TIMEOUT: 					str = "SmartHSState_SMART_RECORD_TIMEOUT"; break;
		case Headset::Controller::SmartHSState_SMART_READY: 							str = "SmartHSState_SMART_READY"; break;
    }
    qmDebugMessage(QmDebug::Warning, "%s", str.c_str());
//    {
    CState currentState;
    guiTree.getLastElement(currentState);

    bool isRecord = false;
    static bool isUploaded = false;

    if (currentState.getType() == endMenuWindow)
    {
        GuiWindowsSubType subType = ((CEndState&)guiTree.getCurrentState()).subType;

        if ((subType == filetree) && isEndPlaying)
        {
        	menu->filesStage = 2;
        	drawMenu();
        }

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

   // qmDebugMessage(QmDebug::Warning, "isUploaded = %i", isUploaded);
   // qmDebugMessage(QmDebug::Warning, "isRecord = %i", isRecord);
   // qmDebugMessage(QmDebug::Warning, "menu->putOffVoiceStatus = %i", menu->putOffVoiceStatus);
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
        drawWaveInfoOnTx();
    }
}

void Service::RxRssi(int value)
{
	menu->reciveRSSI = value;
	drawInfoRssi();
}

std::vector<uint8_t>* Service::loadVoiceMail(uint8_t fileNumber, DataStorage::FS::TransitionFileType tft)
{
    uint8_t result = 0; // ok
    if (storageFs > 0)
    {
        fileMsg.clear();

        int channelNum;
        Multiradio::voice_channel_t type;
        headset_controller->getSmartCurrentChannel(channelNum, type);

        if (channelNum % 2 == 0)
        	result = voice_service->playVoiceMessage(fileNumber, tft, channelNum );
        else
        	result = 3;
    }

    std::string stateStr;
    std::string errorReadStr(errorReadFile);
    std::string errorSpeakerOffStr(noHeadsetPlayErrorStr);

    switch (result)
    {
        case 1: stateStr = errorReadStr; break;
        case 2: stateStr = errorSpeakerOffStr; break;
        case 3: stateStr = openChannelPlayErrorStr; break;
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
        msgBox( callSubMenu[3], gucQuitTextFail);
    }
    isGucAnswerWaiting = false;
	menu->groupCondCommStage = 0;
	menu->focus = 0;
	guiTree.resetCurrentState();
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

void Service::onCompletedStationMode(bool isGoToVoice)
{
	workModeNum_tmp = 0;		// for set YK as default
	workModeNum = 0;
	main_scr->setModeText(mainScrMode[workModeNum]);

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

    isGucAnswerWaiting = false;

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
    waveValue = wave;
    powerValue = power;
    qmDebugMessage(QmDebug::Warning, "onWaveInfoRecieved() SWR = %f, POWER = %f ", waveValue, powerValue);
    drawWaveInfo();
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

void Service::keyEmulate(int key)
{
	keyPressed((UI_Key)key);
}

void Service::draw_emulate()
{
#if TRACE_DISPLAY_TO_PORT

	if (usb_service != NULL)
	{
		if (usb_service->getUsbStatus())
		{
			displayBuf[0]    = 0x10;
			displayBuf[1]    = displayBufSize % 256;
			displayBuf[2]    = displayBufSize / 256;
			displayBuf[3]    = 0x9;
			gsetvp(0,0,128,128);
			ggetsym(0,0,128,128,(GSYMBOL*)&displayBuf[4], displayBufSize - 5);
#ifndef PORT__PCSIMULATOR
			displayBuf[displayBufSize - 1] = 0x11;
			usb_service->transmit(&displayBuf[0], displayBufSize);
#endif
		}
	}
#endif
}

}/* namespace Ui */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(service, LevelVerbose)
#include "qmdebug_domains_end.h"
