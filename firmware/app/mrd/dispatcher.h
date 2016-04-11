/**
 ******************************************************************************
 * @file    dispatcher.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_MRD_DISPATCHER_H_
#define FIRMWARE_APP_MRD_DISPATCHER_H_

#include "qmobject.h"
#include "multiradio.h"
#include "../headset/controller.h"
#include "../dsp/dspcontroller.h"
#include "../atu/atucontroller.h"

namespace Multiradio {

class MainServiceInterface;
class VoiceServiceInterface;

class Dispatcher : public QmObject
{
public:
	Dispatcher(int dsp_uart_resource, int dspreset_iopin_resource, int atu_uart_resource,
			Headset::Controller *headset_controller, Navigation::Navigator *navigator);
	~Dispatcher();
	void startServicing(const Multiradio::voice_channels_table_t &voice_channels_table);
	MainServiceInterface* getMainServiceInterface();
	VoiceServiceInterface* getVoiceServiceInterface();

private:
	friend MainServiceInterface;
	friend VoiceServiceInterface;

	void processDspStartup();
	bool processHeadsetPttStateChange(bool new_state);
	void processHeadsetSmartCurrentChannelChange(int new_channel_number, voice_channel_t new_channel_type);
	void setupVoiceMode(Headset::Controller::Status headset_status);
	void setVoiceChannel();
	bool changeVoiceChannel(int number, voice_channel_t type);
	void updateVoiceChannel();
	void setVoiceDirection(bool ptt_state);
	bool isVoiceMode();
	bool isVoiceChannelTunable();
	void processDspSetRadioCompletion();
	void startIdle();
	void startVoiceTx();
	void prepareTuningTx();
	void processAtuModeChange(AtuController::Mode new_mode);
	void processAtuRequestTx(bool enable);

	DspController *dsp_controller;
	AtuController *atu_controller;
	Headset::Controller *headset_controller;
	MainServiceInterface *main_service;
	VoiceServiceInterface *voice_service;
	voice_channels_table_t voice_channels_table;
	voice_channels_table_t::iterator voice_channel;

    sigc::signal<void> getPSWF();
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_DISPATCHER_H_ */
