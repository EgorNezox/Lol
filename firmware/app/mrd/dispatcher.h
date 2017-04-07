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
#include "../datastorage/fs.h"
#include "../power/battery.h"
#include "aleservice.h"
#include "ale_fxn.h"

namespace Multiradio {

class VoiceServiceInterface;
class AleService;
class AleFxn;

class Dispatcher : public QmObject
{
public:
	Dispatcher(int dsp_uart_resource, int dspreset_iopin_resource, int atu_uart_resource, int atu_iopin_resource,
			Headset::Controller *headset_controller, Navigation::Navigator *navigator, DataStorage::FS *data_storage_fs, Power::Battery *power_battery);
	~Dispatcher();
	void startServicing(const Multiradio::voice_channels_table_t &voice_channels_table);
	VoiceServiceInterface* getVoiceServiceInterface();
	void DspReset();

private:
    friend AleService;
	friend VoiceServiceInterface;
    friend AleFxn;

	void processDspStartup();
	bool processHeadsetPttStateChange(bool new_state);
	void processHeadsetSmartCurrentChannelChange(int new_channel_number, voice_channel_t new_channel_type);
	void setupVoiceMode(Headset::Controller::Status headset_status);
	void setSmartChannelMicLevel(voice_channel_t type);
	voice_emission_t getVoiceEmissionFromFrequency(uint32_t frequency);
	void setVoiceChannel();
	bool changeVoiceChannel(int number, voice_channel_t type);
	void updateVoiceChannel(bool user_request_frequency);
	void saveAnalogHeadsetChannel();
	void setVoiceDirection(bool ptt_state);
	bool isVoiceMode();
	bool isVoiceChannelTunable();
	void processDspSetRadioCompletion();
	void startIdle();
	void startVoiceTx();
	void prepareTuningTx();
	void processAtuModeChange(AtuController::Mode new_mode);
	void processAtuRequestTx(bool enable);

	void latencyGui();

	DspController *dsp_controller;
	AtuController *atu_controller;
	Headset::Controller *headset_controller;
	Navigation::Navigator *navigator;
    AleService *ale_service;
	VoiceServiceInterface *voice_service;
	voice_channels_table_t voice_channels_table;
	voice_channels_table_t::iterator voice_channel;
	uint32_t voice_manual_frequency;
	voice_emission_t voice_manual_emission_type;
	voice_channel_speed_t voice_manual_channel_speed;
	DataStorage::FS *data_storage_fs;
	Power::Battery *power_battery;

    uint32_t prevFrequency = 0;
    uint8_t stationAddress = 1;

    QmTimer *latency_draw;

    void initAle();
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_DISPATCHER_H_ */
