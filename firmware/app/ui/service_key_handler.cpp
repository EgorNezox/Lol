#include <stdlib.h>
#include "qm.h"
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

namespace Ui {

void Service::endMenuWindow_keyPressed(UI_Key key)
{
    CEndState estate = (CEndState&)guiTree.getCurrentState();

    switch(estate.subType)
    {
		case GuiWindowsSubType::condCommand:     	 condCommand_keyPressed(key); break;
		case GuiWindowsSubType::txGroupCondCmd:		 txGroupCondCmd_keyPressed(key); break;
		case GuiWindowsSubType::txPutOffVoice:		 txPutOffVoice_keyPressed(key); break;
		case GuiWindowsSubType::txSmsMessage:	     txSmsMessage_keyPressed(key); break;
		case GuiWindowsSubType::recvVoice:			 recvVoice_keyPressed(key); break;
		case GuiWindowsSubType::recvCondCmd:		 recvCondCmd_keyPressed(key); break;
		case GuiWindowsSubType::rxSmsMessage:		 rxSmsMessage_keyPressed(key); break;
		case GuiWindowsSubType::recvGroupCondCmd:	 recvGroupCondCmd_keyPressed(key); break;
		case GuiWindowsSubType::rxPutOffVoice:		 rxPutOffVoice_keyPressed(key); break;
		case GuiWindowsSubType::volume:				 volume_keyPressed(key); break;
		case GuiWindowsSubType::scan:				 scan_keyPressed(key); break;
		case GuiWindowsSubType::suppress:			 suppress_keyPressed(key); break;
		case GuiWindowsSubType::display:			 display_keyPressed(key); break;
		case GuiWindowsSubType::aruarmaus:			 aruarmaus_keyPressed(key); break;
		case GuiWindowsSubType::gpsCoord:			 gpsCoord_keyPressed(key); break;
		case GuiWindowsSubType::gpsSync:			 gpsSync_keyPressed(key); break;
		case GuiWindowsSubType::setDate:			 setDate_keyPressed(key); break;
		case GuiWindowsSubType::setTime:			 setTime_keyPressed(key); break;
		case GuiWindowsSubType::setFreq: 			 setFreq_keyPressed(key); break;
		case GuiWindowsSubType::setSpeed: 			 setSpeed_keyPressed(key); break;
		case GuiWindowsSubType::editRnKey:			 editRnKey_keyPressed(key); break;
		case GuiWindowsSubType::voiceMode:           voiceMode_keyPressed(key); break;
		case GuiWindowsSubType::channelEmissionType: channelEmissionType_keyPressed(key); break;
		case GuiWindowsSubType::filetree: 			 filetree_keyPressed(key); break;
		case GuiWindowsSubType::sheldure:            sheldure_keyPressed(key); break;
    }
}

void Service::mainWindow_keyPressed(UI_Key key)
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
}

void Service::messangeWindow_keyPressed(UI_Key key)
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
}

void Service::menuWindow_keyPressed(UI_Key key)
{
    CState state = guiTree.getCurrentState();
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
}

void Service::condCommand_keyPressed(UI_Key key)
{
	CEndState estate = (CEndState&)guiTree.getCurrentState();
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
				{
					menu->useCmdRetrans = not menu->useCmdRetrans;
				}
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
					(*iter)++;
				if (menu->txCondCmdStage == 3)
					(*iter)++;

				if ((*iter)->inputStr.size() < 2 )
				{
					(*iter)->inputStr.push_back((char)(42+key));
					int rc = atoi((*iter)->inputStr.c_str());
					uint8_t num = menu->txCondCmdStage == 4 ? 99 : 31;
					if ( rc > num )
					  (*iter)->inputStr.clear();
			 	}
			}
			break;
		}
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
//                break;
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
    }
}

void Service::txGroupCondCmd_keyPressed(UI_Key key)
{
	CEndState estate = (CEndState&)guiTree.getCurrentState();
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
                if (address->size() > 0)
                    address->pop_back();
                else
                    menu->groupCondCommStage--;
            }

            if ( key == keyEnter )
            {
                if (address->size() > 0)
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
                if(menu->cmdScrollIndex > 0)
                	menu->cmdScrollIndex--;
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
}

void Service::txPutOffVoice_keyPressed(UI_Key key)
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
    }
}

void Service::txSmsMessage_keyPressed(UI_Key key)
{
	CEndState estate = (CEndState&)guiTree.getCurrentState();
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
			}
			break;
		}
		case 6:
		{
			switch (key)
			{
				case keyEnter:
				{
					menu->smsTxStage = 1;
					guiTree.resetCurrentState();
					onCompletedStationMode();
					menu->virtCounter = 0;
				}
			}
		}
    }
    }
}

void Service::recvVoice_keyPressed(UI_Key key)
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
		{
			break;
		}
    }
}

void Service::recvCondCmd_keyPressed(UI_Key key)
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
}
void Service::rxSmsMessage_keyPressed(UI_Key key)
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
			return;
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
}

void Service::recvGroupCondCmd_keyPressed(UI_Key key)
{
	if ( key == keyBack)
	{
		switch (cntGucRx)
		{
			case 0:
			{
				cntGucRx = -1;
                menu->offset = 1;
                menu->focus = 2;
                guiTree.backvard();
                onCompletedStationMode(false);
				break;
			}
			case 4:
			{
    			cntGucRx = -1;
                onCompletedStationMode(true);
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
    if ( key == keyEnter && cntGucRx != 4)
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
				//menu->initRxSmsDialog(txQwit,10);
				//break;
			}
		}
    }
}

void Service::rxPutOffVoice_keyPressed(UI_Key key)
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
}

void Service::volume_keyPressed(UI_Key key)
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
}

void Service::scan_keyPressed(UI_Key key)
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
}

void Service::suppress_keyPressed(UI_Key key)
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
}

void Service::display_keyPressed(UI_Key key)
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
}

void Service::aruarmaus_keyPressed(UI_Key key)
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
}

void Service::gpsCoord_keyPressed(UI_Key key)
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
}

void Service::gpsSync_keyPressed(UI_Key key)
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
}

void Service::setDate_keyPressed(UI_Key key)
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
}

void Service::setTime_keyPressed(UI_Key key)
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
}

void Service::setFreq_keyPressed(UI_Key key)
{
    CEndState estate = (CEndState&)guiTree.getCurrentState();
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
}

void Service::setSpeed_keyPressed(UI_Key key)
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
    }
}

void Service::editRnKey_keyPressed(UI_Key key)
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
}

void Service::voiceMode_keyPressed(UI_Key key)
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
}

void Service::channelEmissionType_keyPressed(UI_Key key)
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
}

void Service::filetree_keyPressed(UI_Key key)
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
}

void Service::sheldure_keyPressed(UI_Key key)
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
}

}/* namespace Ui */

