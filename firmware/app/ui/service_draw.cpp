
#include "qm.h"
#define QMDEBUGDOMAIN	service_draw
#include "qmdebug.h"
#include "dialogs.h"
#include "service.h"
#include "texts.h"
#include "../navigation/navigator.h"
#include "../../../system/reset.h"
#include "gui_tree.h"

namespace Ui {

MoonsGeometry ui_msg_box_area       = { 20,29,GDISPW-21,GDISPH-11 };
MoonsGeometry ui_menu_msg_box_area  = { 1,1,GDISPW-2,GDISPH-2 };

void Service::draw()
{
	//GUI_Painter::SetColorScheme(CST_DEFAULT);
  //  GUI_Painter::ClearViewPort(true);

//    GUI_Painter::SetColorScheme(CST_DEFAULT);
//   // GUI_Painter::DrawLine(0,0,10,0);
//   // GUI_Painter::DrawLine(0,20,10,20);
//    GUI_Painter::DrawRect(10,10,20,20,RDM_LINE);
//    return;


    CState currentState;
    guiTree.getLastElement(currentState);

#ifdef PORT__TARGET_DEVICE_REV1
    navigator->set1PPSModeCorrect(false);
#endif

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
        GUI_Painter::DrawRect(0, 0, 127, 127, RDM_FILL);

        GUI_Painter::DrawText(15, 15, GUI_EL_TEMP_CommonTextAreaLT.font,(char*)radioStationStr);
        GUI_Painter::DrawText(15, 30, GUI_EL_TEMP_CommonTextAreaLT.font,(char*)str.c_str());
        GUI_Painter::DrawText(15, 65, GUI_EL_TEMP_CommonTextAreaLT.font,(char*)true_SWF);
    }

    draw_emulate();
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
			case GuiWindowsSubType::condCommand:    	 { menu->initCondCommDialog(st, voice_service->getVirtualMode(), isWaitAnswer); break; }
			case GuiWindowsSubType::txGroupCondCmd: 	 { menu->initGroupCondCmd(st, isGucAnswerWaiting);                              break; }
			case GuiWindowsSubType::recvVoice:      	 {                                                                              break; }
			case GuiWindowsSubType::scan:           	 { menu->inclStatus = menu->scanStatus;    menu->initIncludeDialog();           break; }
			case GuiWindowsSubType::suppress:       	 { menu->inclStatus = menu->supressStatus; menu->initSuppressDialog();          break; }
			case GuiWindowsSubType::display:        	 { menu->initDisplayBrightnessDialog();    										break; }
			case GuiWindowsSubType::aruarmaus:      	 { menu->initAruarmDialog();               										break; }
			case GuiWindowsSubType::volume:         	 { menu->initVolumeDialog();               										break; }
			case GuiWindowsSubType::editRnKey:      	 { menu->initEditRnKeyDialog();           									    break; }
			case GuiWindowsSubType::sheldure:       	 { menu->initSheldureDialog(&sheldure_data, sheldure.size()); 					break; }
			case GuiWindowsSubType::voiceMode:      	 { menu->initSelectVoiceModeParameters(menu->useMode);							break; }
			case GuiWindowsSubType::channelEmissionType: { menu->initSelectChEmissTypeParameters(menu->ch_emiss_type);      			break; }
			case GuiWindowsSubType::gpsSync:             { menu->inclStatus = gpsSynchronization; menu->initIncludeDialog(); 			break; }
			case GuiWindowsSubType::txPutOffVoice:       { drawMenu_txPutOffVoice(); 													break; }
			case GuiWindowsSubType::txSmsMessage:		 { drawMenu_txSmsMessage();													    break; }
			case GuiWindowsSubType::recvCondCmd:		 { drawMenu_recvCondCmd();  											        break; }
			case GuiWindowsSubType::recvGroupCondCmd:	 { drawMenu_recvGroupCondCmd();  											    break; }
			case GuiWindowsSubType::rxSmsMessage:		 { drawMenu_rxSmsMessage();  											        break; }
			case GuiWindowsSubType::rxPutOffVoice:		 { drawMenu_rxPutOffVoice();  											        break; }
			case GuiWindowsSubType::gpsCoord:			 { drawMenu_gpsCoord();  											   		    break; }
			case GuiWindowsSubType::setDate:			 { drawMenu_setDate();  											            break; }
			case GuiWindowsSubType::setTime:			 { drawMenu_setTime();  											            break; }
			case GuiWindowsSubType::setFreq:			 { drawMenu_setFreq();  											            break; }
			case GuiWindowsSubType::setSpeed:			 { drawMenu_setSpeed();  											            break; }
			case GuiWindowsSubType::filetree:			 { drawMenu_filetree();  											            break; }
			default:									 { 																				break; }
        }
    }
    //showSchedulePrompt(DataStorage::FS::FT_SMS, 15);
}

void Service::drawIndicator()
{
    if (!isStartTestMsg)
    {
		static uint8_t gpsStatus = 0;
		if ( guiTree.getCurrentState().getType() == mainWindow && msg_box == nullptr)
		{
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
			indicator->UpdateSynchStatus(voice_service->getVirtualMode());

#if EMUL
            indicator->UpdateGpsStatus(2);
            indicator->UpdateBattery(50);
            indicator->UpdateHeadset(Headset::Controller::Status::StatusSmartOk);
            indicator->UpdateMultiradio(Multiradio::VoiceServiceInterface::Status::StatusVoiceRx);
            indicator->UpdateSynchStatus(true);
#endif

			indicator->Draw();


            MoonsGeometry objArea   = {  0,  0,  127, 127 };
            MoonsGeometry batArea   = {  2, 25,  22, 40 };

			int charge = pGetPowerBattery()->getChargeLevel();

			char var[4] = {0,0,0,0};
			sprintf(var,"%03i",charge);
			std::string chargeStr(var);

			GUI_Obj obj(&objArea);
			GUI_EL_Label  batLabel  (&GUI_EL_TEMP_LabelTitle,    &batArea,   (char*)chargeStr.c_str(), (GUI_Obj *)&obj);

			MoonsGeometry windowArea = {  70, 29, 90, 41 };
			GUI_EL_Window window     (&GUI_EL_TEMP_WindowGeneral, &windowArea,                         (GUI_Obj *)&obj);

			//window.Draw();
			batLabel.Draw();
		}
		else
		{
			if ( guiTree.getCurrentState().getType() == endMenuWindow)
			{
				CEndState st = (CEndState&)guiTree.getCurrentState();
				if (st.subType == GuiWindowsSubType::setTime)
				{
					indicator->DrawTime();
				}
			}
		}
    }
}

void Service::drawWaveInfo()
{

#if EMUL
  waveValue = 9.5;
  powerValue = 9.9;
#endif
    if (msg_box == nullptr )
//#endif
    {
    	if (waveValue > 0.000 && powerValue > 0.000)
		//if (multiradioStatus == Multiradio::VoiceServiceInterface::Status::StatusVoiceTx && (waveValue > 0.000 && powerValue > 0.000))
		{
            MoonsGeometry objArea    = {  0, 0, 127, 127 };
            MoonsGeometry windowArea = { 92, 0, 125, 35 };
            MoonsGeometry waveArea   = { 92, 2,  125, 18 };
            MoonsGeometry powerArea  = { 92, 16, 125, 28 };

			//waveValue = 99.0;

			char var[5] = {0,0,0,0,0};

			if (waveValue >= 10.0)
				sprintf(var,"%02.0f",waveValue);
			else
				sprintf(var,"%01.1f",waveValue);
			//std::string waveStr("S: " + std::string(var));
			std::string waveStr("S " + std::string(var));

			memset(&var, 0, 5);

			if (powerValue >= 10.0)
				sprintf(var,"%02.0f",powerValue);
			else
				sprintf(var,"%01.1f",powerValue);
			//std::string powerStr("P: " + std::string(var));
			std::string powerStr("P " + std::string(var));

			GUI_Obj obj(&objArea);

			LabelParams param = GUI_EL_TEMP_LabelTitle;
			param.element.align.align_h = alignRight;

			GUI_EL_Window window     (&GUI_EL_TEMP_WindowGeneral, &windowArea,                         (GUI_Obj *)&obj);
		   // GUI_EL_Label  rxtxLabel  (&GUI_EL_TEMP_LabelTitle,    &txrxArea,   (char*)rxtxStr.c_str(), (GUI_Obj *)this);
			GUI_EL_Label  waveLabel  (&param,    &waveArea,   (char*)waveStr.c_str(), (GUI_Obj *)&obj);
			GUI_EL_Label  powerLabel (&param,    &powerArea,  (char*)powerStr.c_str(),(GUI_Obj *)&obj);

			window.Draw();
			//rxtxLabel.Draw();
			powerLabel.Draw();
			waveLabel.Draw();
		}
		else
		{
			MoonsGeometry windowArea;
			if (guiTree.getCurrentState().getType() == mainWindow)
                windowArea = { 116, 0, 127, 35 };
			else
                windowArea = { 116, 0, 127, 28 };

            MoonsGeometry objArea = {  0, 0, 127, 127 };
			GUI_Obj obj(&objArea);

			GUI_EL_Window window     (&GUI_EL_TEMP_WindowGeneral, &windowArea,                         (GUI_Obj *)&obj);
			window.Draw();
		}
    }
}

void Service::drawMainWindow()
{
#ifdef PORT__TARGET_DEVICE_REV1
    navigator->set1PPSModeCorrect(true);
#endif

    if (!isStartTestMsg)
    {
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

void Service::drawMenu_txPutOffVoice()
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
}

void Service::drawMenu_txSmsMessage()
{
	if (!isSmsMessageRec)
	{
		std::string titleStr, fieldStr;

        CEndState st = (CEndState&)guiTree.getCurrentState();
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
				uint8_t percent = menu->calcPercent(menu->virtCounter, 120);
				char syn[4] = {0,0,0,0};
				sprintf(syn, "%d", percent);
				fieldStr.append("\t").append(syncWaitingStr).append("\n ").append(syn).append(" %");
			}
			else
			{
				uint8_t percent = menu->calcPercent(counter, 82);

				char pac[2];
				sprintf(pac,"%i", percent);

				fieldStr.append(pac);
				fieldStr.append(" %");
			}
			break;
		}
		}

		menu->initTxSmsDialog( titleStr, fieldStr );
	}
}

void Service::drawMenu_recvCondCmd()
{
	bool isSynch = voice_service->getVirtualMode();
    menu->initRxCondCmdDialog(isSynch, isStartCond);
}

void Service::drawMenu_recvGroupCondCmd()
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
}

void Service::drawMenu_rxSmsMessage()
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
        else if ( voice_service->getVirtualMode() && menu->smsTxStage != 6 && valueRxSms != 84)
        {
				std::string str;
				char syn[4] = {0,0,0,0};
				uint8_t percent = menu->calcPercent(menu->virtCounter, 120);
				sprintf(syn, "%d", percent);
				str.append("\t\t").append(syncWaitingStr).append("\n\t ").append(syn).append(" %");
				menu->initRxSmsDialog(menu->virtCounter ? str.c_str() : receiveStatusStr[1]);
        }
    }
}

void Service::drawMenu_rxPutOffVoice()
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
    		menu->voiceAddr = ch;
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
}

void Service::drawMenu_gpsCoord()
{
#if !defined(PORT__PCSIMULATOR)
	setCoordDate(navigator->getCoordDate());
#endif
	if (menu->coord_log[0] == '0')
		menu->initGpsCoordinateDialog( menu->coord_lat, &menu->coord_log[1]);
	else
		menu->initGpsCoordinateDialog( menu->coord_lat, menu->coord_log);
}

void Service::drawMenu_setDate()
{
    menu->setTitle(dataAndTime[0]);
    CEndState st = (CEndState&)guiTree.getCurrentState();

    if (not inDateMenu)
    {
    	st.listItem.front()->inputStr = voice_service->getVirtualDate();
    	inDateMenu = true;
    }

    std::string str;
    str.append(st.listItem.front()->inputStr); //str.append("00.00.00");
    std::string dateTemplate = "--.--.--";
    str.append(dateTemplate.substr(str.size(),8-str.size()));
    menu->initSetDateOrTimeDialog( str );
}

void Service::drawMenu_setTime()
{
    CEndState st = (CEndState&)guiTree.getCurrentState();
    menu->setTitle(dataAndTime[1]);
    std::string str;
    str.append(st.listItem.front()->inputStr); //str.append("00:00:00");
    std::string timeTemplate = "--:--:--";

    std::string curTime = indicator->getTime(); //
    str.append(timeTemplate.substr(str.size(),8-str.size()));
    std::string res;
    res.append("  ").append(curTime).append("\n\r  ").append(str);
    menu->initSetDateOrTimeDialog( res );
    //indicator->DrawTime();
}

void Service::drawMenu_setFreq()
{
    CEndState st = (CEndState&)guiTree.getCurrentState();
    std::string str; str.append(st.listItem.front()->inputStr); str.append("\n\r     ").append(freq_hz);
    menu->initSetParametersDialog( str );
}

void Service::drawMenu_setSpeed()
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
}

void Service::drawMenu_filetree()
{
    if (!flashTestOn)
        menu->initFileManagerDialog(menu->filesStage);
    flashTestOn = false;
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

void Service::msgBox(const char *title)
{
    Alignment align007 = {alignHCenter,alignTop};
    MoonsGeometry area007 = {1, 1, (GXT)(127), (GYT)(127)};

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
    MoonsGeometry area007 = {1, 1, (GXT)(127), (GYT)(127)};

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
    MoonsGeometry area007 = {1, 1, (GXT)(127), (GYT)(127)};

    if (msg_box != nullptr)
        delete msg_box;
    msg_box = new GUI_Dialog_MsgBox(&area007, (char*)title, (int)condCmd, align007);

    guiTree.append(messangeWindow, "");
    msg_box->setCmd(condCmd);
    if (!isStartTestMsg)
        msg_box->Draw();
    isDrawCondCmd = false;
}

void Service::showMessage(const char *title, const char *text)
{
    MoonsGeometry area = {5,62,122,125};
    GUI_Dialog_MsgBox::showMessage(&area, true, title, text);
}

void Service::showMessage(const char *title, const char *text, MoonsGeometry area)
{
    GUI_Dialog_MsgBox::showMessage(&area, true, title, text);
}

}/* namespace Ui */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(service_draw, LevelDefault)
#include "qmdebug_domains_end.h"
