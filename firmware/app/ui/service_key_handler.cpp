#include <stdlib.h>
#include "qm.h"
#include "qmdebug.h"
#include "dialogs.h"
#include "service.h"
#include "texts.h"
#include <thread>
#include "../navigation/navigator.h"
#include <math.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <string>
#include "../../../system/reset.h"
#include "../../../system/platform_hw_map.h"
#include "qmspiffs.h"
#include "qmspibus.h"
#include "qmm25pdevice.h"

namespace Ui {

#define TEST_VM_MESSAGE_TX 1

void Service::endMenuWindow_keyPressed(UI_Key key)
{
    estate = (CEndState&)guiTree.getCurrentState();

    switch(estate.subType)
    {
        case GuiWindowsSubType::recvCondCmd:		 recvCondCmd_keyPressed(key); 		  break; // УК  rx
		case GuiWindowsSubType::condCommand:     	 condCommand_keyPressed(key);	      break; // УК  tx
		case GuiWindowsSubType::recvGroupCondCmd:	 recvGroupCondCmd_keyPressed(key);    break; // ГУК rx
		case GuiWindowsSubType::txGroupCondCmd:		 txGroupCondCmd_keyPressed(key); 	  break; // ГУК tx
		case GuiWindowsSubType::rxSmsMessage:		 rxSmsMessage_keyPressed(key); 		  break; // СМС rx
		case GuiWindowsSubType::txSmsMessage:	     txSmsMessage_keyPressed(key); 		  break; // СМС tx
		case GuiWindowsSubType::rxPutOffVoice:		 rxPutOffVoice_keyPressed(key);       break; // ГП  rx
		case GuiWindowsSubType::txPutOffVoice:		 txPutOffVoice_keyPressed(key);		  break; // ГП  tx
		case GuiWindowsSubType::recvVoice:			 recvVoice_keyPressed(key); 		  break;
		case GuiWindowsSubType::volume:				 volume_keyPressed(key); 			  break;
		case GuiWindowsSubType::scan:				 scan_keyPressed(key); 				  break;
		case GuiWindowsSubType::suppress:			 suppress_keyPressed(key); 			  break;
		case GuiWindowsSubType::display:			 display_keyPressed(key); 			  break;
		case GuiWindowsSubType::aruarmaus:			 aruarmaus_keyPressed(key);			  break;
		case GuiWindowsSubType::gpsCoord:			 gpsCoord_keyPressed(key);			  break;
		case GuiWindowsSubType::rememberChan:		 storeChan_keyPressed(key);			  break;
		case GuiWindowsSubType::gpsSync:			 gpsSync_keyPressed(key);			  break;
		case GuiWindowsSubType::setDate:			 setDate_keyPressed(key); 			  break;
		case GuiWindowsSubType::setTime:			 setTime_keyPressed(key); 			  break;
		case GuiWindowsSubType::setFreq: 			 voiceMode_keyPressed(key);           break;
		case GuiWindowsSubType::setSpeed: 			 setSpeed_keyPressed(key); 			  break;
		case GuiWindowsSubType::editRnKey:			 editRnKey_keyPressed(key);			  break;
		case GuiWindowsSubType::voiceMode:           voiceMode_keyPressed(key); 		  break;
		case GuiWindowsSubType::channelEmissionType: channelEmissionType_keyPressed(key); break;
		case GuiWindowsSubType::antenaType:			 antennaType_keyPressed(key);		  break;
		case GuiWindowsSubType::filetree: 			 filetree_keyPressed(key);            break;
		case GuiWindowsSubType::sheldure:            sheldure_keyPressed(key);            break;
        case GuiWindowsSubType::tuneGen:             tuneGen_keyPressed(key);             break;
        case GuiWindowsSubType::stationAddress:      stationAddress_keyPressed(key);      break;
        case GuiWindowsSubType::softwareVersion:     softwareVersion_keyPressed(key);     break;
        case GuiWindowsSubType::gucInputType:	     gucInputType_keyPressed(key);        break;
        case GuiWindowsSubType::usbSetting:	         usbSetKeyPressed(key);               break;
        case GuiWindowsSubType::clearFlash:	         clearFlash_keyPressed(key);          break;
        case GuiWindowsSubType::utcSetting:			 utcKeyPressed(key); 			      break;
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
                main_scr->mwFocus = noFocus;
                main_scr->editing = false;
                main_scr->setFreq(main_scr->oFreq.c_str());
            }
            break;
        case keyEnter:
            if (main_scr->mwFocus == frFocus)
            {
                int freq = atoi(main_scr->nFreq.c_str());

                if (freq < 1500000) { main_scr->nFreq = "1500000"; freq = 1500000;  }
                if (freq > 50000000){ main_scr->nFreq = "50000000"; freq = 50000000;}

                    main_scr->mwFocus = noFocus;
                    main_scr->editing = false;

                	main_scr->oFreq.clear();
                	main_scr->oFreq.append(main_scr->nFreq.c_str());
                	isFirstInputFreqEdit = true;

                	//showMessage(waitingStr, flashProcessingStr, promptArea);
                	voice_service->tuneFrequency(freq, true);
            }

            break;
        case keyUp:
        {
			main_scr->mwFocus = chFocus;
			isFirstInputFreqEdit = true;
			main_scr->channelEditing = true;
        }
        case keyDown:
        {
        	main_scr->editing = false;
            if (key == keyDown)
            	main_scr->mwFocus = noFocus;
            main_scr->nFreq.clear();
            main_scr->setFreq(main_scr->oFreq.c_str());
            break;
        }
        case keyLeft:
            if (main_scr->mwFocus == frFocus && main_scr->mainWindowModeId > 0)
            {
                main_scr->mainWindowModeId--;
                this->voice_service->setVoiceMode(Multiradio::VoiceServiceInterface::VoiceMode(main_scr->mainWindowModeId));
            }
            break;
        case keyRight:
            if (main_scr->mwFocus == frFocus && main_scr->mainWindowModeId < 1)
            {
                main_scr->mainWindowModeId++;
                this->voice_service->setVoiceMode(Multiradio::VoiceServiceInterface::VoiceMode(main_scr->mainWindowModeId));
            }
            break;
        default:
            if ( main_scr->mwFocus == frFocus)
            {
            	if (isFirstInputFreqEdit && key > 5 && key < 17)
            	{
            		isFirstInputFreqEdit = false;
            		main_scr->nFreq.clear();
            	}
                main_scr->editingFreq(key);
            }
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
    			Headset::Controller::Status st = pGetHeadsetController()->getStatus();


    		    if (st == Headset::Controller::Status::StatusAnalog)
    		    {
    		    	//showMessage(waitingStr, flashProcessingStr, promptArea);
    		    	pGetVoiceService()->tuneChannel(channelNumberEditing);
    		    }
    		    else
    		    	headset_controller->setChannel(channelNumberEditing);

    			onCompletedStationMode(true);
    			channelNumberEditing = 0;
    			channelNumberSyms = 0;
    		    main_scr->channelEditing = false;
    			main_scr->mwFocus = noFocus;
    			isFirstInputFreqEdit = true;
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
    			main_scr->mwFocus = noFocus;
    		}
    	}
    	if (key == keyDown)
    	{
    		{
    			channelNumberSyms = 0;
    			channelNumberEditing = 0;
    		    main_scr->channelEditing = false;
    		    if (this->voice_service->getVoiceMode() == Multiradio::VoiceServiceInterface::VoiceModeManual)
    		    {
    		    	main_scr->mwFocus = frFocus;
    		    	main_scr->editing = true;
    		    }
    		    else
    		    	main_scr->mwFocus = noFocus;
    		}
    	}
    	if (key == keyUp)
    	{
    		{
    			isFirstInputFreqEdit = false;
    			channelNumberSyms = 0;
    			channelNumberEditing = 0;
    		    main_scr->channelEditing = false;
    		    main_scr->mwFocus = noFocus;
    		}
    	}
    }
    else if (main_scr->emModeEditing)
    {
        switch(key)
        {
			case keyLeft:
			case keyRight:
			{
				menu->ch_emiss_type = !menu->ch_emiss_type;
		        if (menu->ch_emiss_type)

		            voice_service->newTuneEmissionType(Multiradio::voice_emission_t::voiceemissionFM);
		        else
		            voice_service->newTuneEmissionType(Multiradio::voice_emission_t::voiceemissionUSB);
				break;
		    	main_scr->mwFocus = noFocus;
				main_scr->emModeEditing = false;
			}
			case keyUp:
			{
				indicator->isSynchFocus = true;
		    	main_scr->mwFocus = noFocus;
				main_scr->emModeEditing = false;
				main_scr->synchModeEditing = true;
				break;
			}
			case keyDown:
			{
		    	main_scr->mwFocus = mdFocus;
				main_scr->emModeEditing = false;
				main_scr->workModeEditing = true;
				break;
			}
			case keyBack:
			case keyEnter:
			{
				main_scr->mwFocus = noFocus;
				main_scr->emModeEditing = false;
				break;
			}
        }
        main_scr->setEmModeText(ch_em_type_str[menu->ch_emiss_type]);
    }
    else if (main_scr->workModeEditing)
    {
        switch(key)
        {
			case keyLeft:
			{
				if (workModeNum_tmp > 0)
					workModeNum_tmp--;
				if (workModeNum_tmp == 1 && menu->ch_emiss_type)	// skip GUK
					workModeNum = --workModeNum_tmp;
				break;
			}
			case keyRight:
			{
				if (workModeNum_tmp < 3)
					workModeNum_tmp++;
				if (workModeNum_tmp == 1 && menu->ch_emiss_type)	// skip GUK
					workModeNum = ++workModeNum_tmp;
				break;
			}
			case keyUp:
			{
				workModeNum_tmp = workModeNum;
				main_scr->mwFocus = emFocus;
				main_scr->emModeEditing = true;
				main_scr->workModeEditing = false;
				break;
			}
			case keyDown:
			{
				workModeNum_tmp = workModeNum;
				main_scr->mwFocus = noFocus;
				main_scr->workModeEditing = false;
				break;
			}
			case keyBack:
			{
				workModeNum_tmp = workModeNum;
				main_scr->mwFocus = noFocus;
				main_scr->workModeEditing = false;
				break;
			}
			case keyEnter:
			{
				workModeNum = workModeNum_tmp;
				main_scr->mwFocus = noFocus;
				main_scr->workModeEditing = false;
				guiTree.advance(0);
				guiTree.advance(1);
				guiTree.advance(workModeNum);
				keyPressed(UI_Key::keyEnter, false);
				keyPressed(UI_Key::keyEnter, false);
				keyPressed(UI_Key::keyEnter, false);
				break;
			}
        }
        main_scr->setModeText(mainScrMode[workModeNum_tmp]);
    }
    else if (main_scr->synchModeEditing)
    {
        switch(key)
        {
			case keyLeft:
			case keyRight:
			{
				gpsSynchronization = !gpsSynchronization;

				voice_service->setVirtualMode(!gpsSynchronization);
				if (storageFs > 0)
				{
					storageFs->setGpsSynchroMode((uint8_t)gpsSynchronization);
				}
				isChangeGpsSynch = false;
				updateSessionTimeSchedule();
				break;
			}
			case keyUp:
			{
				indicator->isSynchFocus = false;
				main_scr->mwFocus = emFocus;
				main_scr->synchModeEditing = false;
				break;
			}
			case keyDown:
			{
				indicator->isSynchFocus = false;
				main_scr->mwFocus = emFocus;
				main_scr->emModeEditing = true;
				main_scr->synchModeEditing = false;
				break;
			}
			case keyBack:
			{
				indicator->isSynchFocus = false;
				main_scr->mwFocus = noFocus;
				main_scr->synchModeEditing = false;
				break;
			}
			case keyEnter:
			{
				indicator->isSynchFocus = false;
				main_scr->mwFocus = noFocus;
				main_scr->synchModeEditing = false;
				break;
			}
        }
    }
    else
    {
        switch(key)
        {
			case keyChNext:
				//showMessage(waitingStr, flashProcessingStr, promptArea);
				bool var;
				if (headset_controller->getStatus() ==  Headset::Controller::Status::StatusSmartOk)
				{
					int number; Multiradio::voice_channel_t type;
					headset_controller->getSmartCurrentChannel(number, type);
					number += 1;
				    if (number > 98 || number < 1) number = 1;
					headset_controller->setChannel(number);

				}
				else if (headset_controller->getAnalogStatus(var))
				{
					voice_service->tuneNextChannel();
				}
				break;
			case keyChPrev:
				//showMessage(waitingStr, flashProcessingStr, promptArea);
				if (headset_controller->getStatus() ==  Headset::Controller::Status::StatusSmartOk)
				{
					int number; Multiradio::voice_channel_t type;
					headset_controller->getSmartCurrentChannel(number, type);
					number -= 1;
					if (number > 98 || number < 1) number = 1;
					headset_controller->setChannel(number);
				}
				else if (headset_controller->getAnalogStatus(var))
				{
					voice_service->tunePreviousChannel();
				}
				break;
			case keyBack:
				main_scr->mwFocus = noFocus;
				break;
			case keyUp:

				if (main_scr->mwFocus == noFocus)
					main_scr->mwFocus = mdFocus;
				else if (main_scr->mwFocus == mdFocus)
					main_scr->mwFocus = emFocus;
				else if (main_scr->mwFocus == emFocus)
					main_scr->mwFocus = syFocus;
				else if (main_scr->mwFocus == chFocus)
					main_scr->mwFocus = noFocus;
				else if (main_scr->mwFocus == frFocus)
					main_scr->mwFocus = chFocus;
				break;
			case keyDown:

				if (main_scr->mwFocus == noFocus)
					main_scr->mwFocus = chFocus;
				else if (main_scr->mwFocus == chFocus)
					main_scr->mwFocus = frFocus;
				else if (main_scr->mwFocus == syFocus)
					main_scr->mwFocus = emFocus;
				else if (main_scr->mwFocus == emFocus)
					main_scr->mwFocus = mdFocus;
				else if (main_scr->mwFocus == mdFocus)
					main_scr->mwFocus = noFocus;
				break;
			case keyEnter:
				if (main_scr->mwFocus == noFocus)
					guiTree.advance(0);
				break;
        }
        if (main_scr->mwFocus == chFocus)
        {
            main_scr->channelEditing = true;
            oldChannelNumber = voice_service->getCurrentChannelNumber();
        }
        if (main_scr->mwFocus == frFocus)
        {
            if (this->voice_service->getVoiceMode() == Multiradio::VoiceServiceInterface::VoiceModeManual)
            {
                main_scr->editing = true;
                if (not isFirstInputFreqEdit)
                	isFirstInputFreqEdit = true;
            }
        }
        if (main_scr->mwFocus == emFocus)
        {
            main_scr->emModeEditing = true;
        }
        if (main_scr->mwFocus == mdFocus)
        {
            main_scr->workModeEditing = true;
        }
        if (main_scr->mwFocus == syFocus)
        {
            main_scr->synchModeEditing = true;
			indicator->isSynchFocus = true;
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

        if(isDrawErrorFS)
        {
        	formatFlashCard();
        	isDrawErrorFS = false;
        }

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
    if (key == keyBack)
    {
        if(isDrawErrorFS)
        {
            guiTree.delLastElement();
            isDrawErrorFS = false;
        }
    }
	if (vect != nullptr)
	{
		if (position == 0)
			position = 1;
		if (key == keyUp && position > 1)
			position--;
		if (key == keyDown)
			position++;

	   // msg_box->setCmd(vect[position]);
		msg_box->keyPressed(key);
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
            	currentSpeed = headset_controller->realCurrentSpeed;
               // currentSpeed = voice_service->getCurrentChannelSpeed();
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

void Service::condCommand_keyPressed_stage(UI_Key key)
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
//				else
//				{
//					isWaitAnswer = false;
//					guiTree.backvard();
//					onCompletedStationMode();
//				}
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
}

void Service::condCommand_enter_keyPressed()
{
	CEndState estate = (CEndState&)guiTree.getCurrentState();
    int size = 6;
	bool isFromFiveStage = false;

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
            return;
        }

        // indiv
        if (menu->txCondCmdStage == 0 && menu->condCmdModeSelect == 1)
        {
            menu->txCondCmdStage = 1;
            return;
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
            return;
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
            return;
        }

        auto iter = estate .listItem.begin();

        switch (menu->txCondCmdStage)
        {
            case 2: (*iter)++;            if (!((*iter)->inputStr == ""))    menu->txCondCmdStage++; break;
            case 3: (*iter)++; (*iter)++; if ((*iter)->inputStr.size() != 0) menu->txCondCmdStage++; break;
            case 4: if ((*iter)->inputStr.size() != 0) menu->txCondCmdStage++; break;
            case 5: menu->txCondCmdStage++; isFromFiveStage = true; break;
        }
    }

    if ( menu->txCondCmdStage == 6 && isFromFiveStage)
    {
    	condCommand_send();
    }
}

void Service::condCommand_back_keyPressed()
{
	CEndState estate = (CEndState&)guiTree.getCurrentState();

    auto iter = estate.listItem.begin();

    switch(menu->txCondCmdStage)
    {
		case 0:
		{
			guiTree.backvard();
			onCompletedStationMode();
			isWaitAnswer = false;
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
			guiTree.backvard();
			menu->txCondCmdStage = 0;
			onCompletedStationMode();
			isWaitAnswer = false;
			break;
		}
    }
}

void Service::condCommand_send()
{
	CEndState estate = (CEndState&)guiTree.getCurrentState();
#ifndef _DEBUG_

	// [0] - cmd, [1] - raddr, [2] - retrans
	// condCmdModeSelect, 1 - individ, 2 - quit
	int param[3] = {0,0,0}, i = 0;
	for(auto &k: estate.listItem)
	{
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

		storageFs->writeMessage(DataStorage::FS::FT_CND, DataStorage::FS::TFT_TX, &fileMsg);
		draw();
	}

	menu->virtCounter = 0;

	if (menu->condCmdModeSelect == 0)
		voice_service->TurnPSWFMode(0, param[0], 0,0); 				 // групповой вызов
	if (menu->condCmdModeSelect == 1)
		voice_service->TurnPSWFMode(0, param[0], param[2],param[1]); // индивидуальный вызов
	if (menu->condCmdModeSelect == 2){
		param[2] +=32;
		voice_service->TurnPSWFMode(1,param[0],param[2],0); 		 // с квитанцией
		setAsk = true;
		menu->qwitCounter = 65;
	}

	for (auto &k: estate.listItem)
		k->inputStr.clear();


#else
	menu->txCondCmdStage = 0;
	guiTree.resetCurrentState();
	for(auto &k: estate.listItem)
		k->inputStr.clear();

#endif
}

void Service::condCommand_keyPressed(UI_Key key)
{
	CEndState estate = (CEndState&)guiTree.getCurrentState();
    //[0] - CMD, [1] - R_ADDR, [2] - retrans
	condCommand_keyPressed_stage(key);

    switch (key)
    {
		case keyEnter:
		{
			condCommand_enter_keyPressed();
			break;
		}
		case keyBack:
		{
			condCommand_back_keyPressed();
			break;
		}
    }
}

void Service::insertGroupCondCmdSymbol(std::string* commands, UI_Key key)
{
	if (isGucFullCmd)
	{
		uint16_t cmdSize = commands->size();
		bool isLastCmdSymbol = (cmdSize == 400);

		if (cmdSize < 399 && menu->cmdCount < 101 )
		{
			bool isFirstCmdSymbol  = commands->size() % 4 == 0;
			bool isSecondCmdSymbol = commands->size() % 4 == 1;
			bool isThirdCmdSymbol  = commands->size() % 4 == 2;

			if (isFirstCmdSymbol)                                    // if first symbol
			{
				if (key == key0 or key == key1)
				{
					commands->push_back( (char)(key + 42) );         // ins [0-1] [_] [_]
				}
				else
				{
					// not insert incorrect symbol.
				}
			}
			else if (isSecondCmdSymbol)
			{
				UI_Key firstCmdKey = (UI_Key)(commands->at(commands->size() - 1) - 42);

				if (firstCmdKey == key0)                             // if [0] [_] [_]
				{
					commands->push_back( (char)(key + 42) );         // ins [0] [0-9] [_]
				}
				else if (firstCmdKey == key1)                        // if [1] [_] [_]
				{
					if (key >= key0 and key <= key2)
					{
						commands->push_back( (char)(key + 42) );     // ins [1] [0-2] [_]
					}
					else
					{
						// not insert incorrect symbol.
					}
				}
				else
				{
					// ERROR! cmd is not in [0; 127]                  // [2-9] [_] [_]
				}
			}
			else if (isThirdCmdSymbol)
			{
				UI_Key firstCmdKey = (UI_Key)(commands->at(commands->size() - 2) - 42);
				UI_Key secondCmdKey = (UI_Key)(commands->at(commands->size() - 1) - 42);

				if (firstCmdKey == key0)                              // if [0] [0-9] [_]
				{
					commands->push_back( (char)(key + 42) );          // ins [0] [0-9] [0-9]
				}
				else if (firstCmdKey == key1)                         // if [1] [_] [_]
				{
					if (secondCmdKey == key0 or secondCmdKey == key1) 	// if [1] [0-1] [_]
					{
						commands->push_back( (char)(key + 42) );      	// ins [1] [0-1] [0-9]
					}
					else if (secondCmdKey == key2)                   	 // if [1] [2] [_]
					{
						if (key >= key0 and key <= key7)
						{
							commands->push_back( (char)(key + 42) ); 	// ins [1] [2] [0-7]
						}
					}
					else
					{
						// not insert incorrect symbol.
					}
				}
				else
				{
					// ERROR! cmd is not in [0; 127],  >= 200 // [2-9] [_] [_]
				}
			}
			else
			{
				// ERROR! is sym # 4. its impossible.
			}

			menu->cmdCount = (commands->size() / 4) + ((commands->size() % 4) > 0);
//			if (commands->size() > 0 && commands->size() % 4 == 1)
//			{
//				menu->cmdCount++;
//			}


			if (commands->size() > 0 && (commands->size() + 1) % 4 == 0)
			{
				commands->push_back((char) ' ');
			}
				menu->cmdScrollIndex += 20;
		}
	}
	else
	{
		if (commands->size() < 299 && menu->cmdCount < 101 )
		{
			commands->push_back( (char)(key + 42) );
			if (commands->size() > 0 && commands->size() % 3 == 1)
				menu->cmdCount++;

			if (commands->size() > 0 && (commands->size() + 1) % 3 == 0)
			{
				commands->push_back((char) ' ');
			}
				menu->cmdScrollIndex += 20;
		}
	}
}

void Service::deleteGroupCondCmdSymbol(std::string* commands)
{
	if (isGucFullCmd)
	{
		if (commands->size() % 4 == 1)
			menu->cmdCount--;

		if ( commands->size() > 3 && commands->back() == ' ' )
		{
			commands->pop_back();
		}
		commands->pop_back();
	}
	else
	{
		if (commands->size() % 3 == 1)
			menu->cmdCount--;

		if ( commands->size() > 2 && commands->back() == ' ' )
		{
			commands->pop_back();
		}
		commands->pop_back();
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
                menu->focus = 1;
                if (pGetHeadsetController()->getStatus() ==  Headset::Controller::Status::StatusSmartOk){
                	setFreq();
                }
                isTurnGuc = false;
                onCompletedStationMode(true);
                isGucAnswerWaiting = false;
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
                	deleteGroupCondCmdSymbol(commands);
                }
                else
                {
                    menu->cmdCount = 0;
                    menu->groupCondCommStage--;     // одному
                        if(menu->sndMode)
                            menu->groupCondCommStage--; // всем
                }
            }
            if ( key == keyLeft )
            {
            	commands->clear();
            	menu->cmdSymCount = 0;
                menu->cmdCount = 0;
                menu->groupCondCommStage--;     // одному
                    if(menu->sndMode)
                        menu->groupCondCommStage--; // всем
            }

            if ( key == keyEnter )
            {
                if (commands->size() > 0)
                {
                	if ((isGucFullCmd && (commands->size() % 4 == 0)) || !isGucFullCmd)
                		menu->groupCondCommStage++;
                }
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
            	insertGroupCondCmdSymbol(commands, key);
            }
            menu->cmdSymCount = commands->size();
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
                //freqs = mas[0];
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
                    storageFs->writeMessage(DataStorage::FS::FT_GRP, DataStorage::FS::TFT_TX, &fileMsg );
                    draw();
                }

                voice_service->saveFreq(getFreq());
                if (not menu->isCoordValid)
                	menu->useSndCoord = false;
                voice_service->TurnGuc(r_adr,speed,guc_command_vector,menu->useSndCoord);
                isTurnGuc = true;
                if (!menu->sndMode)
                	menu->qwitCounter = 180;

#else
                for (auto &k: estate.listItem)
                    k->inputStr.clear();
                menu->groupCondCommStage = 0;
                guiTree.resetCurrentState();
#endif

            }
            break;
        }
        case 6:
        {
        	if ( key == keyBack )
        	{
        		menu->groupCondCommStage = 0;
                guiTree.backvard();
                menu->focus = 1;
                if (pGetHeadsetController()->getStatus() ==  Headset::Controller::Status::StatusSmartOk){
                	setFreq();
                }
                isTurnGuc = false;
                onCompletedStationMode(true);
                isGucAnswerWaiting = false;
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
        if ( menu->voiceMailSource == VMS_CHANNEL && key > 5 && key < 16 && menu->channalNum.size() < 2 )
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
        	if (menu->voiceMailSource != VMS_CHANNEL)
        	{
            	while (menu->channalNum.size() > 0)
        		{
            		menu->channalNum.pop_back();
        		}
        	}
        	else if (menu->channalNum.size() > 0)
        		menu->channalNum.pop_back();

        	if (menu->voiceMailSource == VMS_CHANNEL && menu->channalNum.size() == 0 || menu->voiceMailSource != VMS_CHANNEL)
        	{
        		guiTree.backvard();
        		menu->offset = 1;
        		menu->focus = 3;
        		menu->inVoiceMail = false;
        		menu->toVoiceMail = false;
        		voice_service->stopAle();
        		isVm = true;
        		onCompletedStationMode(true);
        	}

        }
        if (key == keyLeft)
        {
        	switch (menu->voiceMailSource)
        	{
        		case VMS_CHANNEL: menu->voiceMailSource = VMS_TX_FILE; menu->old_voiceMailSource = VMS_CHANNEL; break;
        		case VMS_TX_FILE: menu->voiceMailSource = VMS_RX_FILE; menu->old_voiceMailSource = VMS_TX_FILE; break;
        	}
        	menu->inVoiceMail = true;
        }
        if (key == keyRight)
        {
        	switch (menu->voiceMailSource)
        	{
        		case VMS_TX_FILE: menu->voiceMailSource = VMS_CHANNEL; menu->old_voiceMailSource = VMS_TX_FILE; break;
        		case VMS_RX_FILE: menu->voiceMailSource = VMS_TX_FILE; menu->old_voiceMailSource = VMS_RX_FILE; break;
        	}
        	menu->inVoiceMail = true;
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
        if (key == keyEnter && menu->voiceMailSource == VMS_CHANNEL && menu->channalNum.size() > 0)
        {
        	uint8_t chan = (uint8_t)atoi( menu->channalNum.c_str());

        	headset_controller->setChannelManual(chan, Multiradio::voicespeed600);
        	//headset_controller->setSmartCurrentChannelSpeed(Multiradio::voicespeed600)
        	//headset_controller->setChannel		(chan);
        	QmThread::msleep(6000);
            headset_controller->startSmartRecord(chan);
            menu->putOffVoiceStatus++;
            menu->inVoiceMail = true;
        }
        if (key == keyEnter && menu->voiceMailSource != VMS_CHANNEL)
        {
        	menu->putOffVoiceStatus++;
        	menu->putOffVoiceStatus++;
        	menu->inVoiceMail = true;
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
    {
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
            	if (menu->voiceMailSource != VMS_CHANNEL)
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
        	Multiradio::voice_message_t message;

//#if TEST_VM_MESSAGE_TX

        	if (menu->voiceMailSource != VMS_CHANNEL)
        	{
				if (storageFs > 0)
				{
					storageFs->readMessage(DataStorage::FS::FT_VM, menu->voiceMailSource == VMS_TX_FILE ? DataStorage::FS::TFT_TX : DataStorage::FS::TFT_RX, &message, 0);
				}
				voice_service->startAleTx((uint8_t)atoi(menu->voiceAddr.c_str()),message);
        	}
        	else
        	{
				message = headset_controller->getRecordedSmartMessage();

				if (storageFs > 0)
				{
					storageFs->writeMessage(DataStorage::FS::FT_VM, DataStorage::FS::TFT_TX, &message);
					menu->toVoiceMail = false;
					draw();
				}
				voice_service->startAleTx((uint8_t)atoi(menu->voiceAddr.c_str()),message);
				//Запись во флеш
        	}

            menu->putOffVoiceStatus++;
        }
#endif
        break;
    }
    case 5:
    {// статус
        if (key == keyEnter)
        {
#ifndef _DEBUG_
            voice_service->stopAle();
#endif
    		guiTree.backvard();
    		menu->offset = 1;
    		menu->focus = 3;
    		menu->inVoiceMail = false;
    		menu->toVoiceMail = false;
    		isVm = true;
    		onCompletedStationMode(true);
            menu->putOffVoiceStatus = 1;
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
					menu->focus = 2;
					onCompletedStationMode();
					menu->virtCounter = 0;
					rememberTextSms = "";
					menu->index_store_sms = 0;
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
				{
					menu->smsTxStage++;
					 if (storageFs > 0)
						  storageFs->getFileNamesByType(&menu->tFiles[DataStorage::FS::FT_SMS], DataStorage::FS::FT_SMS);
				}
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

			uint8_t maxTxCountFiles = storageFs->getTransmitFileTypeCount(DataStorage::FS::FT_SMS,(DataStorage::FS::TransitionFileType::TFT_TX));
			uint8_t maxRxCountFiles = storageFs->getTransmitFileTypeCount(DataStorage::FS::FT_SMS, (DataStorage::FS::TransitionFileType::TFT_RX));
			uint8_t focus = 0;

			switch (key)
			{
			case keyLeft:
			case keyRight:
			{
				if (menu->index_store_sms == 0)
				{
					rememberTextSms = (*iter)->inputStr;
				}

				if (key == keyLeft)
				{
					if (menu->index_store_sms  > -maxRxCountFiles)
						--menu->index_store_sms;
				}
				if (key == keyRight)
				{
					if (menu->index_store_sms  < maxTxCountFiles)
						++menu->index_store_sms;
				}

				if (menu->index_store_sms == 0)
				{
					 (*iter)->inputStr = rememberTextSms;
				}
				else
				{
					int param = (menu->index_store_sms < 0) ? (menu->index_store_sms * (-1)):  (menu->index_store_sms);

					DataStorage::FS::TransitionFileType ttype;
					if(menu->index_store_sms > 0)
						ttype =DataStorage::FS::TransitionFileType::TFT_TX;
					if (menu->index_store_sms < 0)
						ttype =DataStorage::FS::TransitionFileType::TFT_RX;

					focus = menu->recalcFileFocus(param, DataStorage::FS::FT_SMS,  ttype);

					if (menu->tFiles[DataStorage::FS::FT_SMS].size() > 0)
					{
						menu->fileMessage = loadMessage(DataStorage::FS::FT_SMS, ttype, storageFs->getFileNumber(DataStorage::FS::FT_SMS, focus + 1));
						std::string s( menu->fileMessage->begin(), menu->fileMessage->end());
						(*iter)->inputStr = s;
					}
				}

				break;
			}
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
				case keyBack:
				{
					guiTree.backvard();
					menu->offset = 0;
					menu->focus = 2;
					onCompletedStationMode();
					menu->virtCounter = 0;
					menu->smsTxStage = 1;
					rememberTextSms = "";
					menu->index_store_sms = 0;
					break;
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
			case 1:
			case 2:
			case 3:
			{
				isStartCond = false;
				guiTree.backvard();
				onCompletedStationMode();
				menu->virtCounter = 0;
				menu->recvStage = 0;
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
            bool isDraw = false;
            if (menu->recvStage < 1)
            {
            	isDraw = true;
            	menu->recvStage++;
            }
            if (menu->recvStage == 1 && isDraw)
            {
                failFlag = false;
				menu->virtCounter = 0;
                voice_service->TurnPSWFMode(0,0,0,0); // 1 param - request /no request
            }
#endif
        }
    }
}
void Service::rxSmsMessage_keyPressed(UI_Key key)
{
	bool isDraw = false;
	if ( key == keyBack)
	{
		if (cntSmsRx > 0)
			--cntSmsRx;
		if (cntSmsRx == 0 || cntSmsRx == 1)
		{
			cntSmsRx = -1;
			guiTree.backvard();
			menu->offset = 1;
			menu->focus = 2;
			isSmsMessageRec = false;
			menu->smsTxStage = 1;
			onCompletedStationMode();
			menu->virtCounter = 0;
			return;
		}
	}
	if ( key == keyEnter)
	{
		if (cntSmsRx < 2)
		{
			++cntSmsRx;
			isDraw = true;
		}
	}
	if ( (key == keyBack || key == keyEnter) && isDraw)
	{
		if (cntSmsRx == 1) //start
		{
			menu->virtCounter = 0;
			menu->initRxSmsDialog(startStr, cntSmsRx);
			isSmsMessageRec = false;
		}
		if (cntSmsRx == 2) // synch or rx
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
					uint8_t percent = menu->calcPercent(menu->virtCounter, 120);
					char syn[4] = {0,0,0,0};
					sprintf(syn, "%d", percent);
					str.append("\t\t").append(syncWaitingStr).append("\n\t ").append(syn).append(" %");
					menu->initRxSmsDialog(menu->virtCounter ? str.c_str() : receiveStatusStr[1]);
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
	// 1 - start
	// 2 - rx
	// 3 - not used
	// 4 - tx quit

	bool isDraw = false;

	if ( key == keyBack)
	{
		cntGucRx = -1;
		menu->offset = 0;
		menu->focus = 1;
		guiTree.backvard();
		onCompletedStationMode(false);
	}
    if ( key == keyEnter && cntGucRx < 2)
    {
    	++cntGucRx;
    	isDraw = true;
    }
    if ( (key == keyBack || key == keyEnter) && isDraw)
    {
		switch (cntGucRx)
		{
			case 1:
			{
				menu->initRxSmsDialog(STARTS,11);
				break;
			}
			case 2:
			{
                menu->initRxSmsDialog(receiveStatusStr[1],11);
        		//setFreq();
				#ifndef PORT__PCSIMULATOR
				voice_service->saveFreq(getFreq());
				voice_service->TurnGuc();
				#else
                guiTree.resetCurrentState();
				#endif
				break;
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
            onCompletedStationMode(true);
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

            menu->offset = 1;
            menu->focus = 3;
            guiTree.backvard();
            menu->inVoiceMail = false;
            menu->toVoiceMail = false;
#ifndef _DEBUG_
            onCompletedStationMode(true);
#endif
            menu->putOffVoiceStatus = 1;

           // menu->putOffVoiceStatus--;
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
            {
                menu->putOffVoiceStatus--;
            }
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
            guiTree.backvard();
            menu->inVoiceMail = false;
            menu->toVoiceMail = false;
            voice_service->stopAle();
            onCompletedStationMode(true);
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
        menu->offset = 1;
        menu->focus = 2;
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
        menu->offset = 1;
        menu->focus = 2;
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
        menu->offset = 2;
        menu->focus = 3;
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
        menu->offset = 2;
        menu->focus = 3;
        oldSuppress = -1;
    }
}

void Service::utcKeyPressed(UI_Key key)
{
	if (key == keyBack )
	{
		guiTree.backvard();
	}
	if (key == keyLeft )
	{
	  if (menu->UtcStatusStatus > 0)
	  		 menu->UtcStatusStatus--;
	}
	if ( key == keyRight)
	{
		if (menu->UtcStatusStatus < 11)
			menu->UtcStatusStatus++;
	}
	if (key == keyEnter)
	{
		storageFs->setTimeZone(menu->UtcStatusStatus);
		//menu->UtcStatusStatus = menu->UtcStatusStatusTemp;
		guiTree.backvard();
	}
}

void Service::display_keyPressed(UI_Key key)
{
   if (key == keyLeft )
    {
        if (menu->displayBrightness_tmp > 0)
        menu->displayBrightness_tmp--;
    }
    if ( key == keyRight)
    {
        if (menu->displayBrightness_tmp < 2)
        menu->displayBrightness_tmp++;
    }
    if ( key == keyBack)
    {
        guiTree.backvard();
        menu->offset = 3;
        menu->focus = 4;

		if (menu->displayBrightness == 2)
			set_max_pal();
		if (menu->displayBrightness == 1)
			set_mid_pal();
		if (menu->displayBrightness == 0)
			set_min_pal();
		menu->displayBrightness_tmp = menu->displayBrightness;
        return;
    }
    if (key == keyEnter)
    {
        guiTree.backvard();
        menu->offset = 3;
        menu->focus = 4;
        menu->displayBrightness = menu->displayBrightness_tmp;
    }

    if ( key != keyBack)
    {
		if (menu->displayBrightness_tmp == 2)
			set_max_pal();
		if (menu->displayBrightness_tmp == 1)
			set_mid_pal();
		if (menu->displayBrightness_tmp == 0)
			set_min_pal();
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
            menu->aruArmMruStatus[menu->focus] = menu->aruArmMruStatus[menu->focus] ? false : true;
        }
#ifndef PORT__PCSIMULATOR
        uint8_t vol = menu->getAruArmMru();

        if (menu->focus == 2)
        {
        	int level = 255;
        	if (!vol) level = 24;
        	voice_service->TuneMicLevel(level);
        }
        else
        {
        	voice_service->TurnAGCMode(vol, menu->focus);
        }
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


void Service::storeChan_keyPressed(UI_Key key)
{
	if (key == keyBack)
	{
		if (labelChan.size() == 0)
			guiTree.backvard();
		if (labelChan.size() > 0)
			labelChan.pop_back();
	}

	if ( key >= key0 && key <= key9 )
	{
		labelChan.push_back( (char)(key + 42) );
		if ( atoi(labelChan.c_str()) > 98)
			labelChan.clear();
	}

	if (key == keyEnter)
	{
		Headset::Controller::SmartStatusDescription desc;
		bool digit  = headset_controller->getSmartStatus (desc);

		int chan = atoi(labelChan.c_str());

		if (chan <= 98 && chan > 0)
		{
			int freq  = getFreq();
			uint8_t speed, type;
			headset_controller->WorkChannelSpeed(speed, type);

			Multiradio::voice_channel_entry_t entry;
			entry.frequency = freq;
			entry.speed     = (Multiradio::voice_channel_speed_t)speed;
			entry.type      = (Multiradio::voice_channel_t)      type;

			storageFs->addVoiceChannelTable(chan, entry);
          }
		labelChan.clear();
		guiTree.backvard();
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
				{
			        //GUI_Painter::ClearViewPort(true);
			        //showMessage(waitingStr, flashProcessingStr, promptArea);
					storageFs->setGpsSynchroMode((uint8_t)gpsSynchronization);
					//draw();
				}
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
            inDateMenu = false;
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
    	if (st.size() == 8)
    		voice_service->setVirtualDate(st);
    	guiTree.backvard();
    	inDateMenu = false;
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
    	if (st.size() == 8)
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
        if (freq >= 1500000 && freq <= 50000000)
        {
            //GUI_Painter::ClearViewPort(true);
            //showMessage(waitingStr, flashProcessingStr, promptArea);
            voice_service->tuneFrequency(freq, true);
            //draw();

            guiTree.resetCurrentState();
            menu->focus = 0;
        }
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
            //GUI_Painter::ClearViewPort(true);
            //showMessage(waitingStr, flashProcessingStr, promptArea);
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
		case keyRight:
		{
			if ( currentSpeed < Multiradio::voice_channel_speed_t(4) )
			{
				int i = currentSpeed;
				currentSpeed = Multiradio::voice_channel_speed_t(++i);
			}
			break;
		}
		case keyDown:
		case keyLeft:
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
            uint16_t t;
            storageFs->getFhssKey(t);
            char ch[4]; sprintf(ch, "%d", t); ch[3] = '\0';
            menu->RN_KEY = ch;
            menu->focus = 4;
            menu->offset = 3;
            guiTree.backvard();
        }
    }
    if (key == keyEnter)
    {
    	if (storageFs)
    	{
            //GUI_Painter::ClearViewPort(true);
            //showMessage(waitingStr, flashProcessingStr, promptArea);
    		storageFs->setFhssKey((uint16_t)atoi(menu->RN_KEY.c_str()));
    		//draw();
    	}
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

        if (storageFs)
        {
			//GUI_Painter::ClearViewPort(true);
			//showMessage(waitingStr, flashProcessingStr, promptArea);
			storageFs->setVoiceMode(menu->useMode);
			//draw();
        }
        guiTree.backvard();
        menu->focus = 5;
        menu->offset = 3;
    }
    if ( key == keyBack)
    {
        guiTree.backvard();
        menu->focus = 5;
        menu->offset = 3;
    }
    if (key == keyUp || key == keyDown || key == keyLeft || key == keyRight)
    {
        menu->useMode = !menu->useMode;
    }
}

void Service::channelEmissionType_keyPressed(UI_Key key)
{
    if ( key == keyEnter || key == keyBack)
    {
        //GUI_Painter::ClearViewPort(true);
        //showMessage(waitingStr, flashProcessingStr, promptArea);

        if (menu->ch_emiss_type)
            voice_service->newTuneEmissionType(Multiradio::voice_emission_t::voiceemissionFM);
        else
            voice_service->newTuneEmissionType(Multiradio::voice_emission_t::voiceemissionUSB);

        guiTree.backvard();
        menu->focus = 4;
        menu->offset = 3;
    }
    if (key == keyUp || key == keyDown || key == keyLeft || key == keyRight)
    {
        menu->ch_emiss_type = !menu->ch_emiss_type;
    }
}

void Service::antennaType_keyPressed(UI_Key key)
{

    if ( key == keyEnter )
    {
    	menu->antenna = menu->antenna_tmp;
    	voice_service->setWorkAtu(menu->antenna);

        guiTree.backvard();
        menu->focus = 6;
        menu->offset = 4;
    }

    if ( key == keyBack )
    {
    	menu->antenna_tmp = menu->antenna;
        guiTree.backvard();
        menu->focus = 6;
        menu->offset = 4;
    }

    if (menu->antenna_tmp < 2 && (key == keyUp || key == keyRight))
    {
        menu->antenna_tmp++;
    }

    if (menu->antenna_tmp > 0 && (key == keyDown || key == keyLeft))
    {
    	menu->antenna_tmp--;
    }
}

void Service::filetree_keyPressed(UI_Key key)
{
    if ( key == keyEnter)
    {
        if (menu->filesStage == 0) // mode list
        {

            menu->fileType = (DataStorage::FS::FileType)menu->filesStageFocus[0];
             if (storageFs > 0)
                  storageFs->getFileNamesByType(&menu->tFiles[menu->fileType], menu->fileType);
        }

        if (menu->filesStage == 1) // Rx / Tx
        {
            menu->transitionfileType = (DataStorage::FS::TransitionFileType)menu->filesStageFocus[1];
        }

        if (menu->filesStage == 2) // file names
        {
        	uint8_t focus = menu->recalcFileFocus(menu->filesStageFocus[2], menu->fileType, menu->transitionfileType);

            switch (menu->fileType)
            {
				case DataStorage::FS::FT_SMS:
				case DataStorage::FS::FT_CND:
				case DataStorage::FS::FT_GRP:
					if (menu->tFiles[menu->fileType].size() > 0)
						menu->fileMessage = loadMessage(menu->fileType, menu->transitionfileType, storageFs->getFileNumber(menu->fileType, focus));
					break;

				case DataStorage::FS::FT_VM:
					if (menu->tFiles[menu->fileType].size() > 0)
						menu->fileMessage = loadVoiceMail(storageFs->getFileNumber(menu->fileType, focus), menu->transitionfileType);
					break;
				case DataStorage::FS::FT_SP: break;
            }
        }

        if (menu->filesStage < 3)
        {
        	if (menu->filesStage == 2)
        	{
        		uint8_t maxCountFiles = storageFs->getTransmitFileTypeCount(menu->fileType, menu->transitionfileType);
        		if (maxCountFiles)
        			menu->filesStage++;
        	}
        	else
        		menu->filesStage++;
        }
    }
    if ( key == keyBack)
    {
        if (menu->filesStage > 0)
        {
        	if (menu->filesStage == 2)
        		menu->filesStageFocus[menu->filesStage] = 0;
        	menu->filesStage--;
        }
        else
        {
           guiTree.backvard();
           menu->focus = 3;
           menu->offset = 1;
        }
    }
    if (key == keyUp)
    {
        switch(menu->filesStage){
        case 0:
            if (menu->filesStageFocus[menu->filesStage] > 1)
                menu->filesStageFocus[menu->filesStage]--;
            break;
        case 1:
        case 2:
            if (menu->filesStageFocus[menu->filesStage] > 0)
                menu->filesStageFocus[menu->filesStage]--;
            break;
        case 3:
            if (menu->textAreaScrollIndex > 0)
            menu->textAreaScrollIndex--;
            break;
        }
    }
    if (key == keyDown)
    {
        switch (menu->filesStage)
        {
			case 0:
			{
				if (menu->filesStageFocus[menu->filesStage] < 4)
					menu->filesStageFocus[menu->filesStage]++;
				break;
			}
			case 1:
			{
				if (menu->filesStageFocus[menu->filesStage] < 1)
					menu->filesStageFocus[menu->filesStage]++;
				break;
			}
			case 2:
			{
				uint8_t maxCountFiles = storageFs->getTransmitFileTypeCount(menu->fileType, menu->transitionfileType);
				if (menu->filesStageFocus[menu->filesStage] < maxCountFiles - 1)
					menu->filesStageFocus[menu->filesStage]++;
				break;
			}
			case 3:
			{
				menu->textAreaScrollIndex++;
				break;
			}
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
            menu->offset = 3;
            menu->focus = 5;
            break;
        }
        if ( key == keyEnter )
        {
            if (menu->sheldureStageFocus[menu->sheldureStage] + 1 == sheldure_data.size() && sheldure.size() < 50)
            {
                menu->sheldureStage = 1;
                isNew = true;
                tempSheldureSession.clear();
            }
            else
            {
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
        if (tempSheldureSession.time.size() > 0) {
            tempSheldureSession.time.pop_back();
        }
        else {
            menu->sheldureStage = 1;
        }
        if ( tempSheldureSession.time.size() == 3)
        {
            tempSheldureSession.time.pop_back();
        }
    }
    if ( key == keyEnter )
    {
    	if ((tempSheldureSession.type == 1 //CC (conditional commands)
    			|| tempSheldureSession.type == 3 //SMS
				|| tempSheldureSession.type == 4) //VM (voice mail)
    			&& tempSheldureSession.time.size() == 5)
    	{
    		tempSheldureSession.freq.clear();
    		tempSheldureSession.freq.push_back('0');
    		addSession(isNew);
    	}
    	else if (tempSheldureSession.time.size() == 5)
    	{
            menu->sheldureStage = 3;
    	}
    }
    if ( key >= key0 && key <= key9 )
    {
    	if (tempSheldureSession.time.size() == 0
    			&& key >= key3 && key <= key9)
    	{
    		tempSheldureSession.time.push_back('0');
    	}
    	if (tempSheldureSession.time.size() == 1
    			&& tempSheldureSession.time.at(0) == '2'
    	    			&& key >= key4 && key <= key5)
    	{
    	    		tempSheldureSession.time = "02";
    	}

    	if ( tempSheldureSession.time.size() == 2)
    	{
            tempSheldureSession.time.push_back(':');
    	}

     	if (tempSheldureSession.time.size() == 3
        		&& key >= key6 && key <= key9)
        {
     		tempSheldureSession.time.push_back('0');
        }

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


    }
    menu->sheldureTimeStr = tempSheldureSession.time;
    if (!isNew)
        tempSheldureSession.freq = sheldure[menu->sheldureStageFocus[0]].freq;
    menu->sheldureFreqStr = tempSheldureSession.freq;
   // menu->sheldureFreqStr.append(" ").append(freq_hz);
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
            if (atoi(tempSheldureSession.freq.c_str()) >= 1500000
					&& atoi(tempSheldureSession.freq.c_str()) <= 50000000)
            {
            	addSession(isNew);
            }
        }
        if ( key >= key0 && key <= key9 )
        {
        	if (!isNew && tempSheldureSession.freq.size() > 0
        			&& tempSheldureSession.freq.at(0) == '0')
        	{
        		tempSheldureSession.freq.clear();
        	}
        	if (tempSheldureSession.freq.size() == 0 && key == key0)
        	{
        		tempSheldureSession.freq.clear();
        	} else if (tempSheldureSession.freq.size() < 8 ) {
                tempSheldureSession.freq.push_back(key + 42);
        	}
        }
        menu->sheldureFreqStr = tempSheldureSession.freq;
        //menu->sheldureFreqStr.append(" ").append(freq_hz);
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
    }
}

void Service::addSession(bool isNew)
{
	if (isNew) {
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

void Service::tuneGen_keyPressed(UI_Key key)
{
    switch(key)
    {
        case keyBack:
        {
        	guiTree.backvard();
        	menu->offset = 0;
        	menu->focus = 0;
            break;
        }
        case keyLeft:
        {
            if (gen_test_focus != 0)
                gen_test_focus--;
            break;
        }
        case keyRight:
        {
            if(gen_test_focus < 3)
              gen_test_focus++;
            break;
        }
        case keyUp:
        {
            isIncKey = true;
            break;
        }
        case keyDown:
        {
            isDecKey = true;
            break;
        }
        case keyEnter:
        {
            genTune();
            break;
        }
    }
}

void Service::genTune()
{
    int tuneGenValue = 0;
    for(uint8_t i = 4; i > 0; i--)
    {
        tuneGenValue += tuneDigt[i-1] * pow(10, 4-i);
    }
#ifndef PORT__PCSIMULATOR
    navigator->setGeneratorAbsValue(tuneGenValue);
#endif
}

void Service::stationAddress_keyPressed(UI_Key key)
{
    if ( key == keyBack )
    {
        guiTree.backvard();
        menu->offset = 0;
        menu->focus = 1;
    }
}

void Service::softwareVersion_keyPressed(UI_Key key)
{
    if ( key == keyBack )
    {
        guiTree.backvard();
        menu->offset = 0;
        menu->focus = 2;
    }
}

void Service::gucInputType_keyPressed(UI_Key key)
{
    if ( key == keyLeft )
    {
        isGucFullCmd_tmp = false;
    }
    if ( key == keyRight )
    {
        isGucFullCmd_tmp = true;
    }
    if ( key == keyBack )
    {
        guiTree.backvard();
        menu->offset = 1;
        menu->focus = 4;
    }
    if ( key == keyEnter )
    {
        guiTree.backvard();
        menu->offset = 1;
        menu->focus = 4;
        isGucFullCmd = isGucFullCmd_tmp;
    }
}

void Service::usbSetKeyPressed(UI_Key key)
{
	if (key == keyLeft)
	{
		 is_usb_on = false;
	}
	if (key == keyRight)
	{
		is_usb_on = true;
	}
	if ( key == keyBack )
	{
		guiTree.backvard();
		menu->offset = 1;
		menu->focus = 3;
	}
	if ( key == keyEnter )
	{
		guiTree.backvard();
		menu->offset = 1;
		menu->focus = 3;
		voice_service->newUsbState(is_usb_on);
	}
}

void Service::clearFlash_keyPressed(UI_Key key)
{
	 if ( key == keyBack )
	 {
		 guiTree.backvard();
		 menu->offset = 1;
		 menu->focus = 5;
	 }

	 if ( key == keyLeft )
	 {
		 isFlashErase_tmp = false;
	 }
	 if ( key == keyRight )
	 {
		 isFlashErase_tmp = true;
	 }

	 if ( key == keyEnter )
	 {
		 guiTree.backvard();
		 menu->offset = 1;
		 menu->focus = 5;
		 isGucFullCmd = isFlashErase_tmp;

		 if (isGucFullCmd)
		 {
			 formatFlashCard();
			 guiTree.resetCurrentState();
		 }
	 }
}

void Service::formatFlashCard()
{
	 QmSPIBus::enable(platformhwDataFlashSpi);

	 QmM25PDevice::Config data_flash_config;
	 data_flash_config.sector_size    = 64*1024;
	 data_flash_config.sectors_count  = 32;
	 data_flash_config.speed          = 75000000;
	 data_flash_config.idle_clock_low = false;

	 QmM25PDevice data_flash_device(data_flash_config, platformhwDataFlashSpi, platformhwDataFlashCsPin);
	 QmSpiffs::Config data_fs_config;

	 data_fs_config.device             = &data_flash_device;
	 data_fs_config.physical_address   = 0;
	 data_fs_config.physical_size      = 32*64*1024;
	 data_fs_config.logical_block_size = 64*1024;
	 data_fs_config.logical_page_size  = data_flash_device.getPageSize();
	 data_fs_config.max_opened_files   = 10;

	 QmSpiffs::format(data_fs_config);
	 QmSpiffs::mount("data", data_fs_config);
}

}/* namespace Ui */

