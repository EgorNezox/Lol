/**
 ******************************************************************************
 * @file    Controller.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @author  Petr Dmitriev
 * @date    29.10.2015
 *
 ******************************************************************************
 */

#define QMDEBUGDOMAIN	hscontroller

#include "qm.h"
#include "qmdebug.h"
#include "qmendian.h"
#include "qmpushbuttonkey.h"
#include "qmtimer.h"

#include "headsetcrc.h"
#include "controller.h"
#include "smarttransport.h"

#include "qmthread.h"

#define min_debug  0
#define full_analize_status 0
#include "../../system/platform_hw_map.h"


namespace Headset {

Controller::Controller(int rs232_uart_resource, int ptt_iopin_resource) :
	state(StateNone), status(StatusNone), ptt_pressed(false),
	cmd_repeats_counter(0),
	ch_number(1),

	ch_type(Multiradio::channelInvalid),
	ch_speed(Multiradio::voicespeedInvalid),
	indication_enable(true), squelch_enable(false),
	hs_state(SmartHSState_SMART_NOT_CONNECTED),

	message_record_data_ready(false),
	message_to_play_data_packets_sent(0),
	message_to_play_last_packet_data_size(0)
{
	// создаем кнопку тангенты с событием реакции на нажатие
	ptt_key = new QmPushButtonKey(ptt_iopin_resource);
	ptt_debounce_timer = new QmTimer(true, this);
	ptt_debounce_timer->timeout.connect(sigc::mem_fun(this, &Controller::processPttDebounceTimeout));
	ptt_debounce_timer->setInterval(PTT_DEBOUNCE_TIMEOUT);

	// создаем таймер повтора команды до 10 попыток
	ptt_resp_timer = new QmTimer(true, this);
	ptt_resp_timer->timeout.connect(sigc::mem_fun(this, &Controller::processPttResponseTimeout));
	ptt_resp_timer->setInterval(HS_RESPONCE_TIMEOUT);

	// создаем таймер опроса гарнитуры на исправность
	poll_timer = new QmTimer(false, this);
	poll_timer->timeout.connect(sigc::mem_fun(this, &Controller::processHSUartPolling));
	poll_timer->setInterval(HS_UART_POLLING_INTERVAL);

	// создаем таймер ответа на команду
	cmd_resp_timer = new QmTimer(true, this);
	cmd_resp_timer->timeout.connect(sigc::mem_fun(this, &Controller::processCmdResponceTimeout));
	cmd_resp_timer->setInterval(HS_RESPONCE_TIMEOUT);

	// создаем таймер для отрисовки событий с гарнитуры
	delay_timer = new QmTimer(false, this);
	delay_timer->setInterval(300);
	delay_timer->timeout.connect(sigc::mem_fun(this, &Controller::setUpdateState));

	repeatPlayTimer = new QmTimer(true, this);
	repeatPlayTimer->setInterval(3000);
	repeatPlayTimer->timeout.connect(sigc::mem_fun(this, &Controller::onRepeatPlaying));

	// включаем прием по uart
	transport = new SmartTransport(rs232_uart_resource, 1, this);
	transport->receivedCmd.connect(sigc::mem_fun(this, &Controller::processReceivedCmd));
	smart_status_description.channels_mismatch = false;


	pin_debug = new QmIopin(platformhwDebugIoPin, this);
	//pin_debug->writeOutput(QmIopin::Level_Low);
}

Controller::~Controller()
{
}

void Controller::startServicing(const Multiradio::voice_channels_table_t& local_channels_table)
{
	//qmDebugMessage(QmDebug::Info, "start servicing...");
	// выбираем таблицу каналов
	ch_table = &local_channels_table;
	if (local_channels_table.size() == 0)
	{
		ch_number = 0;
		ch_type = Multiradio::channelInvalid;
		ch_speed = Multiradio::voicespeedInvalid;
	}
	else
	{
		ch_number = 1;
		ch_type = ch_table->at(0).type;
		ch_speed = ch_table->at(0).speed;

	}
	// включаем uart
	transport->enable();
	// начинаем опрос гарнитуры
	poll_timer->start();
	// проверяем состояние нажатия ганитуры

	ptt_pressed = ptt_key->isPressed();
	// добавляем обработку тангенты
	ptt_key->stateChanged.connect(sigc::mem_fun(this, &Controller::processPttStateChanged));
}

Controller::Status Controller::getStatus()
{
	// аналоговая или цифровая гарнитура
	return status;
}

bool Controller::getSmartStatus(SmartStatusDescription& description)
{
	description = smart_status_description;
	return state == StateSmartOk;
}

bool Controller::getAnalogStatus(bool& open_channels_missing)
{
	open_channels_missing = false;
	return state == StateAnalog;
}

bool Controller::getPTTState(bool& state)
{
	state = ptt_pressed;
	return this->state == StateAnalog || this->state == StateSmartOk;
}

bool Controller::getSmartCurrentChannel(int& number, Multiradio::voice_channel_t &type)
{
	number = ch_number;
	type = ch_type;
	return state == StateSmartOk;
}

bool Controller::setSmartCurrentChannelSpeed(Multiradio::voice_channel_speed_t speed)
{
	if (state == StateSmartOk)
	{
		ch_speed = speed;
		synchronizeHSState();
		return true;
	}
	return false;
}


void Controller::processPttStateChanged()
{
	pin_debug->writeOutput(QmIopin::Level_High);
	//ptt_debounce_timer->start();


	if (getStatus() != StatusSmartOk)
	{
//		ptt_pressed = !ptt_pressed;
		pttStateChanged(ptt_key->isPressed());
	}
	else
	{
		ptt_debounce_timer->start();
	}

	pin_debug->writeOutput(QmIopin::Level_Low);
}

void Controller::processPttDebounceTimeout()
{
	if (ptt_pressed != ptt_key->isPressed())
	{
		ptt_pressed = !ptt_pressed;

		#if (min_debug)
		qmDebugMessage(QmDebug::Dump, "ptt state changed: %d", ptt_pressed);
		#endif

		uint8_t data = ptt_pressed ? 0xFF : 0;

        #if (min_debug)
		qmDebugMessage(QmDebug::Dump, "transmit HS_CMD_PTT_STATE: 0x%02X", data);
		#endif

		++cmd_repeats_counter;
		transport->transmitCmd(HS_CMD_PTT_STATE, &data, 1);
		ptt_resp_timer->start();
	}
}

void Controller::processPttResponseTimeout()
{
	#if (min_debug)
	qmDebugMessage(QmDebug::Info, "ptt response timeout, repeat %d", cmd_repeats_counter);
	#endif
	// при повторе команды повтор или сброс на команду опроса статуса
	(cmd_repeats_counter < HS_MAX_CMD_REPEATS) ? repeatLastCmd() : resetState();
}

void Controller::transmitCmd(uint8_t cmd, uint8_t *data, int data_len)
{
	#if (min_debug)
	qmDebugMessage(QmDebug::Dump, "------------- transmitCmd %d", cmd);
	#endif
	QM_ASSERT(data_len <= SmartTransport::MAX_FRAME_DATA_SIZE);

	++cmd_repeats_counter;
	transport->transmitCmd(cmd, data, data_len);
	cmd_resp_timer->start();

	#if (min_debug)
	qmDebugMessage(QmDebug::Dump, "cmd_resp_timer started");
	#endif
}

void Controller::transmitResponceCmd(uint8_t cmd, uint8_t *data, int data_len)
{
	transport->transmitCmd(cmd, data, data_len);
}

void Controller::processCmdResponceTimeout()
{
	#if (min_debug)
	qmDebugMessage(QmDebug::Info, "cmd response timeout, repeat %d", cmd_repeats_counter);
	#endif

	(cmd_repeats_counter < HS_MAX_CMD_REPEATS) ? repeatLastCmd() : resetState();
}

void Controller::repeatLastCmd()
{
	++cmd_repeats_counter;
	transport->repeatLastCmd();
	cmd_resp_timer->start();
}

void Controller::processHSUartPolling()
{
	transmitCmd(HS_CMD_STATUS, NULL, 0);
}

void Controller::GarnitureStart()
{
	ptt_key->stateChanged();
}

void  Controller::WorkChannelSpeed(uint8_t &speed, uint8_t &type)
{
	speed = ch_speed;
	type  = ch_type;
}

Controller::SmartHSState Controller::getSmartHSState()
{
    return hs_state;
}

Multiradio::voice_message_t Controller::getRecordedSmartMessage()
{
	if (message_record_data_ready)
		return message_record_data;

	return Multiradio::voice_message_t();
}

void Controller::setSmartMessageToPlay(Multiradio::voice_message_t data)
{
	message_to_play_data = data;
}

void Controller::setSmartHSState(SmartHSState state)
{
    //qmDebugMessage(QmDebug::Dump, "setSmartHSState() state = %d", state);
	if (state != hs_state)
	{
		hs_state = state;
		isSmartHSStateChange = true;
	}
	//smartHSStateChanged(hs_state);
}

void Controller::checkUpdateSmartHSState()
{
	if (isSmartHSStateChange)
	{
	  isSmartHSStateChange = false;
	  delay_timer->start();
	}
}

void Controller::setChannel(uint8_t channel)
{
	old_ch_number = ch_number;
	ch_number = channel;
	ch_speed = getChannelSpeed(channel);
	synchronizeHSState();
}

void Controller::setChannelManual(uint8_t channel, Multiradio::voice_channel_speed_t speed )
{
	old_ch_number = ch_number;
	ch_number = channel;
	ch_speed = speed;
	isSetCurrentChannel = true;
	synchronizeHSState();
}

void Controller::messagePacketResponce(int packet_number)
{
#if (min_debug)
	qmDebugMessage(QmDebug::Dump, "messagePacketResponce() packet_number %d", packet_number);
#endif

	const int data_size = 4;
	uint8_t data[data_size];

	for (int i = 0; i < data_size; ++i) data[i] = 0;

	data[2] = (uint8_t)packet_number;
	transmitResponceCmd(HS_CMD_MESSAGE_UPLOAD, data, data_size);
}


uint16_t Controller::calcPacketCrc(uint8_t* data, int data_len)
{
	HeadsetCRC crc;
	crc.update(data, (uint16_t)data_len);
	uint16_t result = ~crc.result();
	uint16_t least = (result & 0x00FF);
	result = (result >> 8) & 0x00FF;
	result |= (least << 8) & 0xFF00;
	return result;
}

Multiradio::voice_channel_t Controller::getChannelType(uint32_t channel)
{
	if (channel > 0 && channel < ch_table->size())
		return ch_table->at(channel - 1).type;

	return Multiradio::voice_channel_t::channelInvalid;
}

Multiradio::voice_channel_speed_t Controller::getChannelSpeed(uint32_t channel)
{
	if (channel > 0 && channel < ch_table->size())
		return ch_table->at(channel - 1).speed;

	return Multiradio::voice_channel_speed_t::voicespeedInvalid;
}

void Controller::updateStatus(Status new_status)
{
	if (status != new_status)
	{
        //qmDebugMessage(QmDebug::Dump, "status changed: %d", new_status);
		status = new_status;
		statusChanged(new_status);
	}
}

void Controller::resetState()
{
	updateState(StateNone);
	cmd_repeats_counter               = 0;
	message_to_play_data_packets_sent = 0;

	if (!poll_timer->isActive()) poll_timer->start();
}

void Controller::setUpdateState()
{
	delay_timer->stop();
	smartHSStateChanged(hs_state);
}

void Controller::onRepeatPlaying()
{
	if (isNotInitModule)
	{
		setSmartHSState(SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL);
		setChannelManual(ch_number, Multiradio::voicespeed600);
		qmDebugMessage(QmDebug::Dump, "onRepeatPlaying() isNotInitModule: true");
	}
}

void Controller::processReceivedCmd(uint8_t cmd, uint8_t* data, int data_len)
{
	//pin_debug->writeOutput(QmIopin::Level_High);

	#if (min_debug)
	qmDebugMessage(QmDebug::Dump, "processReceivedCmd() cmd = 0x%2X, data_len = %d", cmd, data_len);
	#endif
	cmd_repeats_counter = 0;

	switch (cmd)
	{
	case HS_CMD_STATUS:
	{
		if (changeSpeedAuto)
		{
			changeSpeedAuto = false;
			autoSpeedChanged();
		}

		#if (min_debug)
		qmDebugMessage(QmDebug::Dump, "cmd_resp_timer(%p): %d", cmd_resp_timer, cmd_resp_timer->isActive());
		#endif

		if (state != StateSmartOk && !cmd_resp_timer->isActive())
		{
            #if (min_debug)
			qmDebugMessage(QmDebug::Dump, "unexpected cmd HS_CMD_STATUS");
	        #endif
			break;
		}

		if (state == StateSmartOk && !cmd_resp_timer->isActive())
		{
			#if (min_debug)
			qmDebugMessage(QmDebug::Dump, "asynchronous cmd HS_CMD_STATUS");
			#endif
			processReceivedStatus(data, data_len);
			break;
		}
		cmd_resp_timer->stop();

		if (data_len == 0)
		{
			//идентичный отправленному пакет
			cmd_resp_timer->stop();

			if (state == StateNone)
				updateState(StateAnalog);

			else if (state == StateSmartOk)
				updateState(StateNone);
		}
		else if (data_len == HS_CMD_STATUS_RESP_LEN)
		{
			processReceivedStatus(data, data_len);
		}
		else
		{
			cmd_resp_timer->stop();

			#if (min_debug)
			qmDebugMessage(QmDebug::Dump, "wrong data length of responce to cmd HS_CMD_STATUS");
			#endif

			resetState();
		}
		break;
	}
	case HS_CMD_CH_LIST:
	{
		cmd_resp_timer->stop();
		if (state != StateSmartInitChList)
		{
			#if (min_debug)
			qmDebugMessage(QmDebug::Dump, "unexpected cmd HS_CMD_CH_LIST");
			#endif

			resetState();
			break;
		}
		else if (data_len != HS_CMD_CH_LIST_RESP_LEN)
		{
			#if (min_debug)
			qmDebugMessage(QmDebug::Dump, "wrong data length of responce to cmd HS_CMD_CH_LIST");
			#endif

			resetState();
			updateState(StateSmartMalfunction);
			break;
		}
		else if (!verifyHSChannels(data, data_len))
		{
			#if (min_debug)
			qmDebugMessage(QmDebug::Warning, "Headset channels is not match to local channels table");
			#endif
			smart_status_description.channels_mismatch = true;
		}
		updateState(StateSmartInitHSModeSetting);
		synchronizeHSState();
		break;
	}
	case HS_CMD_PTT_STATE:
	{
		ptt_resp_timer->stop();
		#if (min_debug)
		qmDebugMessage(QmDebug::Dump, "ptt response received");
		#endif
		if (ptt_pressed == ptt_key->isPressed())
		{
			//pin_debug->writeOutput(QmIopin::Level_Low);

			bool accepted = pttStateChanged(ptt_pressed);
			#if (min_debug)
			qmDebugMessage(QmDebug::Dump, "ptt state accepted: %d", accepted);
			#endif
		}
		break;
	}
	case HS_CMD_MESSAGE_DOWNLOAD:
	{
		if (data[0] == 0)
			changeSpeedAuto = true;

		cmd_resp_timer->stop();
		switch (hs_state)
		{
		case SmartHSState_SMART_RECORD_DOWNLOADING:
			if (message_to_play_data_packets_sent == message_to_play_data_packets.size())
			{
				setSmartHSState(SmartHSState_SMART_PLAYING);
				qmDebugMessage(QmDebug::Dump, "SmartHSState_SMART_RECORD_DOWNLOADING. switch to SmartHSState_SMART_PLAYING");
			}
			else
			{
				sendHSMessageData();
				qmDebugMessage(QmDebug::Dump, "SmartHSState_SMART_RECORD_DOWNLOADING. sendHSMessageData()");
			}
			break;

		default: break;
		}
		break;
	}
	case HS_CMD_MESSAGE_UPLOAD:
	{
		cmd_resp_timer->stop();
		if (hs_state == SmartHSState_SMART_RECORDING)
		{
			setSmartHSState(SmartHSState_SMART_RECORD_UPLOADING);
		}
		else if (hs_state != SmartHSState_SMART_RECORD_UPLOADING)
		{
			break;
		}

		#if (min_debug)
		qmDebugMessage(QmDebug::Dump, "message packet received");
		#endif

		if (data_len != HS_CMD_MESSAGE_LEN)
		{
			#if (min_debug)
			qmDebugMessage(QmDebug::Dump, "wrong data length of cmd HS_CMD_MESSAGE_RECORD");
			#endif
			break;
		}
		messagePacketReceived(data, data_len);
		break;
	}
	default:
        #if (min_debug)
		qmDebugMessage(QmDebug::Dump, "receved unknown cmd 0x%02X", cmd);
	    #endif
		break;
	}
	checkUpdateSmartHSState();
}


void Controller::processReceivedStatus(uint8_t* data, int data_len)
{
	#if (min_debug)
	qmDebugMessage(QmDebug::Dump, "processReceivedStatus()");
	#endif

	logOut(data);

	switch (state)
	{
		case StateNone:
		{
			poll_timer->stop();
			ch_number = qmFromLittleEndian<uint16_t>(data + 4);
			uint8_t ch_mask = data[8];
			switch (ch_mask & 0x03)
			{
				case 0: ch_type = Multiradio::channelOpen;     break;
				case 1: ch_type = Multiradio::channelClose;    break;
				default: ch_type = Multiradio::channelOpen;
			}
			switch((ch_mask & 0x1C) >> 2)
			{
				case 1: ch_speed = Multiradio::voicespeed600;  break;
				case 3: ch_speed = Multiradio::voicespeed1200; break;
				case 4: ch_speed = Multiradio::voicespeed2400; break;
				case 5: ch_speed = Multiradio::voicespeed4800; break;
				default: ch_speed = Multiradio::voicespeed1200;
			}
			updateState(StateSmartInitChList);
			transmitCmd(HS_CMD_CH_LIST, NULL, 0);
			break;
		}
		case StateSmartInitHSModeSetting:
		{
	//		poll_timer->start();
			smartCurrentChannelChanged(ch_number, ch_type);
	//		no break;
		}
		case StateSmartOk:
		{
			uint16_t chan_number = qmFromLittleEndian<uint16_t>(data + 4);
			uint8_t ch_mask 				= data[8];
			uint8_t mode_mask 				= data[9];

			switch((ch_mask & 0x1C) >> 2)
			{
				case 1: realCurrentSpeed = Multiradio::voicespeed600;  break;
				case 3: realCurrentSpeed = Multiradio::voicespeed1200; break;
				case 4: realCurrentSpeed = Multiradio::voicespeed2400; break;
				case 5: realCurrentSpeed = Multiradio::voicespeed4800; break;
				default: realCurrentSpeed = Multiradio::voicespeed1200;
			}

			if (error_status)
			{
			    //qmDebugMessage(QmDebug::Dump, "smart headset error");
				updateState(StateSmartMalfunction);
				break;
			}

			if (isNotInitModule)
			{
				//_isNotInitModule = true;
				qmDebugMessage(QmDebug::Dump, "isNotInitModule !!!");
				return;
			}

			Multiradio::voice_channel_t chan_type = Multiradio::channelInvalid;
			switch (ch_mask & 0x03)
			{
				case 0: chan_type = Multiradio::channelOpen; break;
				case 1: chan_type = Multiradio::channelClose; break;
				case 2: chan_type = Multiradio::channelInvalid; break;
			}
			//qmDebugMessage(QmDebug::Dump, "headset channel speed = %d", ((ch_mask & 0x1C) >> 2));

			if (old_ch_number != chan_number || ch_type != chan_type)
			{
				old_ch_number = ch_number;
				ch_number = chan_number;
				ch_type = chan_type;
				updateState(StateSmartOk);
				smartCurrentChannelChanged(ch_number, ch_type);
			}
			else
			{
				updateState(StateSmartOk);
			}

			indication_enable = (mode_mask_add & 0x01)     == 0;
			squelch_enable = ((mode_mask_add >> 1) & 0x01) == 0;

			switch (hs_state)
			{
				case SmartHSState_SMART_RECORDING:
				{
					if (!delaySpechRecord)
					{
						updateState(StateSmartOk);
						setSmartHSState(SmartHSState_SMART_READY);
						delaySpeachStateChanged(false);
					}
					break;
				}
				case SmartHSState_SMART_PLAYING:
				{
					if (!(mode_mask & 0x40))
					{
						setSmartHSState(SmartHSState_SMART_READY);
					}
					break;
				}
				case SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL:
				{
					startMessageRecord();
					break;
				}
				case SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE:
				{
					cmd_resp_timer->setInterval(HS_RESPONCE_TIMEOUT); //TODO: fix it
					if (mode_mask & 0x10)
					{
						setSmartHSState(SmartHSState_SMART_RECORDING);
					}
					break;
				}
				case SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL:
				{
					startMessagePlay();
					break;
				}
				case SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE:
				{

					if (mode_mask & 0x40)
					{
						setSmartHSState(SmartHSState_SMART_RECORD_DOWNLOADING);
						sendHSMessageData();
						qmDebugMessage(QmDebug::Dump, "SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE. MODE MASK CORRECT");
					}
					else
					{
						qmDebugMessage(QmDebug::Dump, "SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE. MODE MASK INCORRECT!!!");
					}
					break;
				}
			}
			break;
		}
		case StateSmartInitChList:
		{
		    //qmDebugMessage(QmDebug::Dump, "unexpected cmd HS_CMD_STATUS");
			resetState();
			break;
		}
		default:
			break;
	}
	checkUpdateSmartHSState();
}

void Controller::processReceivedStatusAsync(uint8_t* data, int data_len)
{
	#if (min_debug)
	qmDebugMessage(QmDebug::Dump, "processReceivedStatusAsync()");
	#endif

	logOut(data);

	switch (state)
	{
		case StateSmartOk:
		{
			uint16_t chan_number = qmFromLittleEndian<uint16_t>(data + 4);
			uint8_t ch_mask = data[8];
			uint8_t mode_mask = data[9];
			uint8_t mode_mask_add = data[10];
			uint8_t error_status = data[16] & 0x1;
			uint8_t isNotInitModule  = data[16] & 0x80;
//			qmDebugMessage(QmDebug::Dump, "chan_number: %d", chan_number);
//			qmDebugMessage(QmDebug::Dump, "ch_mask: 0x%X", ch_mask);
//			qmDebugMessage(QmDebug::Dump, "mode_mask: 0x%X", mode_mask);
//			qmDebugMessage(QmDebug::Dump, "mode_mask_add: 0x%X", mode_mask_add);

			if (error_status)
			{
				qmDebugMessage(QmDebug::Dump, "smart headset error");
				updateState(StateSmartMalfunction);
				break;
			}

			Multiradio::voice_channel_t chan_type = Multiradio::channelInvalid;
			switch (ch_mask & 0x03)
			{
				case 0: chan_type = Multiradio::channelOpen; break;
				case 1: chan_type = Multiradio::channelClose; break;
			}
		    //qmDebugMessage(QmDebug::Dump, "headset channel speed = %d", ((ch_mask & 0x1C) >> 2));
			if (ch_number != chan_number || ch_type != chan_type)
			{
				ch_number = chan_number;
				ch_type = chan_type;
				updateState(StateSmartOk);
				smartCurrentChannelChanged(ch_number, ch_type);
				synchronizeHSState();
			}
			else
			{
				updateState(StateSmartOk);
			}
			indication_enable = (mode_mask_add & 0x01)        == 0;
			squelch_enable    = ((mode_mask_add >> 1) & 0x01) == 0;

			_isNotInitModule = (bool)isNotInitModule;
			if (!_isNotInitModule)
			{
				processReceivedStatus(data, data_len);
			}
		}
	}
	checkUpdateSmartHSState();
}

void Controller::logOut(uint8_t* data)
{
	uint16_t chan_number_ = qmFromLittleEndian<uint16_t>(data + 4);
	uint8_t ch_mask_ 				= data[8];
	uint8_t mode_mask_ 				= data[9];

	uint8_t energy                  = data[9] & 1;    // режим работы
	uint8_t modeKB                  = data[9] & 14;   // скорость гарнитуры
	        delaySpechRecord        = data[9] & 0x10;

	uint8_t rxtxMode                = data[9] & 0x20;
	uint8_t delayMsgOn              = data[9] & 0x40;
	uint8_t isDelayMsgPlaying       = data[9] & 0x80;

	        mode_mask_add 			= data[10];
	      	error_status 			= data[16] & 0x1;

#if (full_analize_status)
	uint8_t isModuleSkziMalfunction = data[16] & 0x2;
	uint8_t isNotKeys 		 		= data[16] & 0x4;
	uint8_t isLineOutSbError 		= data[16] & 0x8;
	uint8_t isLineOutError 			= data[16] & 0x10;
	uint8_t isFlashWritingError 	= data[16] & 0x20;
	uint8_t isPreDefControlError 	= data[16] & 0x40;
#endif

			isNotInitModule 	    = data[16] & 0x80;

#if (full_analize_status)
	uint8_t isOverKeysInput 	    = data[17] & 0x1;
	uint8_t isUykipError     	    = data[17] & 0x2;
	uint8_t isChangeChannelError    = data[17] & 0x4;

	uint8_t isHardwareModuleError   = data[18] & 0x1;
	uint8_t isMaskReadingError      = data[18] & 0x2;
	uint8_t isNSDSwitch             = data[18] & 0x80;

	uint8_t isCmdKeysReset          = data[19] & 0x1;
	uint8_t isDelayRecordImp        = data[19] & 0x2;  // channel without keys
	uint8_t isDelayPlayingImpBuf    = data[19] & 0x4;  // buffer is not exist
	uint8_t isDelayPlayingImpCRC    = data[19] & 0x8;  // CRC error
	uint8_t isDelayPlayingImpDevice = data[19] & 0x10; // packets from many devices


	qmDebugMessage(QmDebug::Dump, "chan_number:             %d", chan_number_);
	qmDebugMessage(QmDebug::Dump, "ch_mask:                 0x%X", ch_mask_);
	qmDebugMessage(QmDebug::Dump, "mode_mask:               0x%X", mode_mask_);
	qmDebugMessage(QmDebug::Dump, "mode_mask_add:           0x%X", mode_mask_add);

	qmDebugMessage(QmDebug::Dump, "energy:                  %d", energy);
	qmDebugMessage(QmDebug::Dump, "modeKB:                  %d", modeKB);
	qmDebugMessage(QmDebug::Dump, "delaySpechRecord:        %d", delaySpechRecord);
	qmDebugMessage(QmDebug::Dump, "rxtxMode:                %d", rxtxMode);
	qmDebugMessage(QmDebug::Dump, "delayMsgOn:              %d", delayMsgOn);
	qmDebugMessage(QmDebug::Dump, "isDelayMsgPlaying:       %d", isDelayMsgPlaying);

	qmDebugMessage(QmDebug::Dump, "isModuleSkziMalfunction: %d", isModuleSkziMalfunction);
	qmDebugMessage(QmDebug::Dump, "isNotKeys:               %d", isNotKeys);
	qmDebugMessage(QmDebug::Dump, "isLineOutSbError:        %d", isLineOutSbError);
	qmDebugMessage(QmDebug::Dump, "isLineOutError:          %d", isLineOutError);
	qmDebugMessage(QmDebug::Dump, "isFlashWritingError:     %d", isFlashWritingError);
	qmDebugMessage(QmDebug::Dump, "isPreDefControlError:    %d", isPreDefControlError);
	qmDebugMessage(QmDebug::Dump, "isNotInitModule:         %d", isNotInitModule);
	qmDebugMessage(QmDebug::Dump, "isOverKeysInput:         %d", isOverKeysInput);
	qmDebugMessage(QmDebug::Dump, "isUykipError:            %d", isUykipError);
	qmDebugMessage(QmDebug::Dump, "isChangeChannelError:    %d", isChangeChannelError);

	qmDebugMessage(QmDebug::Dump, "isHardwareModuleError:   %d", isHardwareModuleError);
	qmDebugMessage(QmDebug::Dump, "isMaskReadingError:      %d", isMaskReadingError);
	qmDebugMessage(QmDebug::Dump, "isNSDSwitch:             %d", isNSDSwitch);
	qmDebugMessage(QmDebug::Dump, "isCmdKeysReset:          %d", isCmdKeysReset);
	qmDebugMessage(QmDebug::Dump, "isDelayRecordImp:        %d", isDelayRecordImp);
	qmDebugMessage(QmDebug::Dump, "isDelayPlayingImpBuf:    %d", isDelayPlayingImpBuf);
	qmDebugMessage(QmDebug::Dump, "isDelayPlayingImpCRC:    %d", isDelayPlayingImpCRC);
	qmDebugMessage(QmDebug::Dump, "isDelayPlayingImpDevice: %d", isDelayPlayingImpDevice);
#endif
}

void Controller::updateState(State new_state)
{
	if (state != new_state)
	{
	   //qmDebugMessage(QmDebug::Dump, "state changed: %d", new_state);
		state = new_state;
		switch (state)
		{
		case StateNone:
			updateStatus(StatusNone);
			setSmartHSState(SmartHSState_SMART_NOT_CONNECTED);
			break;

		case StateSmartOk:
			updateStatus(StatusSmartOk);
			setSmartHSState(SmartHSState_SMART_READY);
			break;

		case StateSmartMalfunction:
			updateStatus(StatusSmartMalfunction);
			setSmartHSState(SmartHSState_SMART_ERROR);
			break;

		case StateAnalog: updateStatus(StatusAnalog); break;
		case StateSmartInitChList: 					  break;
		case StateSmartInitHSModeSetting: 			  break;
		default: 									  break;
		}
	}
	checkUpdateSmartHSState();
}



bool Controller::verifyHSChannels(uint8_t* data, int data_len)
{
	// check 98 channels
	if (data_len != HS_CMD_CH_LIST_RESP_LEN)
		return false;

	if (ch_table->size() != HS_CHANNELS_COUNT)
		return false;

//	qmDebugMessage(QmDebug::Dump, "Channels types:");
//	int type_[4] = {0, 0, 0, 0};
//	for (int i = 0; i < 25; ++i) {
//		type_[3] = (data[i] >> 6) & 0x03;
//		type_[2] = (data[i] >> 4) & 0x03;
//		type_[1] = (data[i] >> 2) & 0x03;
//		type_[0] = data[i] & 0x03;
//		for (int j = 0; j < 4; ++j) {
//			qmDebugMessage(QmDebug::Dump, "%d) %d", i * 4 + j, type_[j]);
//		}
//	}
	int type[4];


	for (int byte_ind = 0; byte_ind < 25; ++byte_ind)
	{
		type[0] = data [byte_ind]       & 0x03;
		type[1] = (data[byte_ind] >> 2) & 0x03;
		type[2] = (data[byte_ind] >> 4) & 0x03;
		type[3] = (data[byte_ind] >> 6) & 0x03;

		for (int bit_ind = 0; bit_ind < 4; ++bit_ind)
		{
			// reserved
			if ((byte_ind == 0 && bit_ind == 0) || (byte_ind == 24 && bit_ind == 3))
				continue;
			switch (type[bit_ind])
			{
			case 0:
				if (ch_table->at(byte_ind * 4 + bit_ind - 1).type != Multiradio::channelOpen)
					return false;
				break;
			case 1:
				if (ch_table->at(byte_ind * 4 + bit_ind - 1).type != Multiradio::channelClose)
					return false;
				break;
			case 2:
				if (ch_table->at(byte_ind * 4 + bit_ind - 1).type != Multiradio::channelInvalid)
					return false;
				break;
			default:
				return false;
			}
		}
	}
	return true;
}

void Controller::synchronizeHSState()
{
	qmDebugMessage(QmDebug::Dump, " ---------- synchronizeHSState() channel number: %d", ch_number);
//	ch_number = channel;
//	ch_speed =  getChannelSpeed(channel);

	const int data_size = 16;
	uint8_t data[data_size];
	data[0] = 0xAB;
	data[1] = 0xBA;
	data[2] = 0x01;

	switch (ch_speed)
	{
		case Multiradio::voicespeed600:  data[2] |= 0x02; break;
		case Multiradio::voicespeed1200: data[2] |= 0x06; break;
		case Multiradio::voicespeed2400: data[2] |= 0x08; break;
		case Multiradio::voicespeed4800: data[2] |= 0x0B; break;
		default: 						 data[2] |= 0x02; break;
	}

	data[2] |= (uint8_t)((squelch_enable ? 0 : 1) << 5); // mode mask
	data[3] = ch_number; //isSetCurrentChannel ? 0 : ch_number; 								 // channel number
	data[4] = 0xFF; 									 // reserved
	data[5] = 0x01 | (uint8_t)((indication_enable ? 0 : 1) << 2);

	for (int i = 6; i < data_size; ++i)
		data[i] = 0;
	transmitCmd(HS_CMD_SET_MODE, data, data_size);

	isSetCurrentChannel = false;
}

void Controller::startSmartPlay(uint8_t channel)
{
	if (ch_table->at(channel - 1).type != Multiradio::channelClose)
	{
		//qmDebugMessage(QmDebug::Warning, "startSmartPlay() channel %d is not close", channel);
		setSmartHSState(SmartHSState_SMART_BAD_CHANNEL);
		checkUpdateSmartHSState();
		return;
	}
	//qmDebugMessage(QmDebug::Dump, "startSmartPlay() channel %d", channel);
	if (message_to_play_data.size() == 0)
	{
		qmDebugMessage(QmDebug::Warning, "startSmartPlay() message is empty");
		setSmartHSState(SmartHSState_SMART_EMPTY_MESSAGE);
		checkUpdateSmartHSState();
		return;
	}

	if (hs_state != SmartHSState_SMART_READY)
	{
		qmDebugMessage(QmDebug::Dump, "startSmartPlay() unexpected state: hs_state = %d");
		//hs == Headset::Controller::SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE)
		//return;
	}

	messageToPlayDataPack();
	setSmartHSState(SmartHSState_SMART_PREPARING_PLAY_SETTING_CHANNEL);
	setChannelManual(channel, Multiradio::voicespeed600);

	repeatPlayTimer->start();

	//QmThread::msleep(10000);

	//ch_number = channel;
	//ch_speed = Multiradio::voicespeed600;

	delay_timer->start();
	//synchronizeHSState();
	checkUpdateSmartHSState();
}

void Controller::startMessagePlay()
{
    //qmDebugMessage(QmDebug::Warning, "startMessagePlay()");
	setSmartHSState(SmartHSState_SMART_PREPARING_PLAY_SETTING_MODE);
	message_to_play_data_packets_sent = 0;

	const int data_size = 16;
	uint8_t data[data_size];
	data[0] = 0xAB;
	data[1] = 0xBA;
	data[2] = 0x01;
	data[2] |= 0x40;
	data[2] |= (uint8_t)((squelch_enable ? 0 : 1) << 5); // mode mask
	data[3] = 0;      									 // channel number
	data[4] = 0xFF; 									 // reserved
	data[5] = 0x01 | (uint8_t)((indication_enable ? 0 : 1) << 2);

	for (int i = 6; i < data_size; ++i)
		data[i] = 0;

	cmd_resp_timer->setInterval(3000);
	transmitCmd(HS_CMD_SET_MODE, data, data_size);
	checkUpdateSmartHSState();
}

void Controller::stopSmartPlay() {
//	qmDebugMessage(QmDebug::Dump, "stopSmartPlay()");
	setSmartHSState(SmartHSState_SMART_READY);
	synchronizeHSState();
	checkUpdateSmartHSState();
}

void Controller::startSmartRecord(uint8_t channel)
{
	if (ch_table->at(channel - 1).type != Multiradio::channelClose)
	{
		//qmDebugMessage(QmDebug::Warning, "startSmartRecord() channel %d is not close", channel);
		setSmartHSState(SmartHSState_SMART_BAD_CHANNEL);
		checkUpdateSmartHSState();
		return;
	}
	if (hs_state != SmartHSState_SMART_READY)
	{
		//qmDebugMessage(QmDebug::Dump, "startSmartRecord() unexpected state");
		return;
	}
	//qmDebugMessage(QmDebug::Dump, "startSmartRecord() channel %d", channel);
	message_record_data_ready = false;
	message_record_data.clear();

	setSmartHSState(SmartHSState_SMART_PREPARING_RECORD_SETTING_CHANNEL);
	//ch_number = channel;
	//ch_speed = Multiradio::voicespeed600;
	//synchronizeHSState();
	//startMessageRecord();
	delay_timer->start();
	//checkUpdateSmartHSState();
}

void Controller::startMessageRecord()
{
    //qmDebugMessage(QmDebug::Dump, "startMessageRecord()");
	cmd_resp_timer->setInterval(3000);
	setSmartHSState(SmartHSState_SMART_PREPARING_RECORD_SETTING_MODE);

	const int data_size = 16;
	uint8_t data[data_size];
	data[0] = 0xAB;
	data[1] = 0xBA;
	data[2] = 0x01;
	data[2] |= 0x10;
	data[2] |= (uint8_t)((squelch_enable ? 0 : 1) << 5); // mode mask
	data[3] = 0;										 //ch_number;//0x00;
	data[4] = 0xFF; 									 // reserved
	data[5] = 0x01 | (uint8_t)((indication_enable ? 0 : 1) << 2);

	for (int i = 6; i < data_size; ++i)
		data[i] = 0;
	transmitCmd(HS_CMD_SET_MODE, data, data_size);
	checkUpdateSmartHSState();
}

void Controller::stopSmartRecord()
{
    #if (min_debug)
	qmDebugMessage(QmDebug::Dump, "stopSmartRecord()");
	#endif

	setSmartHSState(SmartHSState_SMART_READY);
	synchronizeHSState();
	checkUpdateSmartHSState();
}

void Controller::sendHSMessageData()
{
	QM_ASSERT(message_to_play_data.size() != 0);
    //qmDebugMessage(QmDebug::Dump, "sendHSMessageData()");
	const int data_size = 208;
	uint8_t data[data_size];
	data[0] = 0xAA;
	data[1] = 0xCD;
	qmToLittleEndian(message_to_play_data_packets_sent, &data[2]);

	if (message_to_play_data_packets_sent     == message_to_play_data_packets.size() - 1)
	{
		qmToLittleEndian((uint16_t)0, &data[4]);
	}
	else if (message_to_play_data_packets_sent == message_to_play_data_packets.size() - 2)
	{
	    qmToLittleEndian((uint16_t)message_to_play_last_packet_data_size, &data[4]);
	}
	else
	{
	    qmToLittleEndian((uint16_t)message_to_play_data_packets.at(message_to_play_data_packets_sent).size(), &data[4]);
	}

	memcpy(&data[6], message_to_play_data_packets.at(message_to_play_data_packets_sent).data(),message_to_play_data_packets.at(message_to_play_data_packets_sent).size());
	qmToLittleEndian(calcPacketCrc(&data[2], 204), &data[206]);
  //  for (int i = 0; i < data_size; ++i)
//	    qmDebugMessage(QmDebug::Dump, "data[%d] = %2X size: %d", i, data[i], data_size);
	//QmThread::msleep(300);
	transmitCmd(HS_CMD_MESSAGE_DOWNLOAD, data, data_size);
	++message_to_play_data_packets_sent;
}

void Controller::messageToPlayDataPack()
{
	QM_ASSERT(message_to_play_data.size() != 0);
	message_to_play_data_packets.clear();

	message_to_play_last_packet_data_size = message_to_play_data.size() % 200;
	if (message_to_play_data.size() <= 200)
	{
		message_to_play_data_packets.push_back(message_to_play_data);

		for (int i = message_to_play_data.size(); i < 200; ++i)
		{
			message_to_play_data_packets.at(0).push_back(0);
		}
	}
	else
	{
		const size_t data_size = message_to_play_data.size();

		for (size_t i = 0; i < data_size; ++i)
		{
			if (i % 200 == 0)
			{
				message_to_play_data_packets.push_back(Multiradio::voice_message_t());
			}
			message_to_play_data_packets.at(i / 200).push_back(message_to_play_data.at(i));
		}
		if (message_to_play_data_packets.at(message_to_play_data_packets.size() - 1).size() != 200)
		{
			const int last = message_to_play_data_packets.size() - 1;

			for (int i = message_to_play_data_packets.at(message_to_play_data_packets.size() - 1).size(); i < 200; ++i)
			{
				message_to_play_data_packets.at(last).push_back(0);
			}
		}
	}
	message_to_play_data_packets.push_back(Multiradio::voice_message_t());
	const int last = message_to_play_data_packets.size() - 1;
	for (int i = 0; i < 200; ++i)
	{
		message_to_play_data_packets.at(last).push_back(0);
	}
//	qmDebugMessage(QmDebug::Dump, "messageToPlayDataPack() packets number = %d", message_to_play_data_packets.size());
}

void Controller::messagePacketReceived(uint8_t* data, int data_len)
{
	//qmDebugMessage(QmDebug::Dump, "messagePacketReceived() data_len = %d", data_len);
	//for (int i = 0; i < data_len; ++i)
	//qmDebugMessage(QmDebug::Dump, "data[%d] = %2X", i, data[i]);

	if (data[0] != 0xAA && data[1] != 0xCD)
	{
        //qmDebugMessage(QmDebug::Dump, "messagePacketReceived() data header is not valid");
		return;
	}
	uint16_t data_crc = calcPacketCrc(&data[2], 204);
	uint16_t crc = qmFromLittleEndian<uint16_t>(data + 206);

	if (data_crc != crc)
	{
	    //qmDebugMessage(QmDebug::Dump, "messagePacketReceived() crc is not valid");
		return;
	}

	uint16_t packet_number = qmFromLittleEndian<uint16_t>(data + 2);
	uint16_t data_size     = qmFromLittleEndian<uint16_t>(data + 4);

	if (data_size > 200)
	{
        //qmDebugMessage(QmDebug::Dump, "messagePacketReceived() data size %d is not valid", data_size);
		return;
	}
	else if (data_size == 0)
	{
        //qmDebugMessage(QmDebug::Dump, "total message data size = %d", message_record_data.size());
		message_record_data_ready = true;
		setSmartHSState(SmartHSState_SMART_READY);
	}

	for (int i = 6; i < data_size + 6; ++i)
		message_record_data.push_back(data[i]);

	messagePacketResponce(packet_number);
	checkUpdateSmartHSState();
}

} /* namespace Headset */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(hscontroller, LevelVerbose)
#include "qmdebug_domains_end.h"
