/*
 * voicemail.h
 *
 *  Created on: 18.01.2017
 *      Author: Alex
 */

#ifndef FIRMWARE_APP_MRD_ALESERVICE_H_
#define FIRMWARE_APP_MRD_ALESERVICE_H_

#include "qmobject.h"
#include "qmtimestamp.h"
#include "qmtimer.h"
#include "qmabstimer.h"
#include "multiradio.h"
#include "../dsp/dspcontroller.h"
#include "../navigation/navigator.h"
#include "../headset/controller.h"
#include "ale_const.h"
#include "ale_main.h"
#include "ale_data_transport.h"
#include "continious_timer.h"
#include "ale_fxn.h"

namespace Multiradio {

class Dispatcher;
class AleMain;
class AleDataTransport;

class AleService : public QmObject
{
public:
	int AleState1;
	int AleVmProgress1;

	void initAle(ale_call_freqs_t call_freqs, ale_call_freqs_t work_freqs, int8s own_adress, bool probe_on);
    void setManualTimeMode(bool mode);
    void setProbeMode(bool probe_on);
    //Status getStatus();
	void startAleRx();
    void startAleTxVoiceMail(uint8_t address, voice_message_t message);
	AleResult stopAle();
	int getAleState();//AleState getAleState();
	void setAleState(bool caller, int8s superphase);
	uint8_t getAleVmProgress();
	uint8_t getAleRxAddress();
	voice_message_t getAleRxVmMessage();
		
	void aleprocessModemPacketTransmitted(DspController::ModemPacketType type);
	void aleprocessModemPacketReceived(DspController::ModemPacketType type, uint8_t snr, uint8_t errors, DspController::ModemBandwidth bandwidth, uint8_t* data, int data_len);
	void aleprocessModemPacketStartedRxPackHead(uint8_t snr, uint8_t errors, DspController::ModemBandwidth bandwidth, uint8_t param_signForm, uint8_t param_packCode, uint8_t* data, int data_len);
	
	void aleprocess1PPS(int hrs ,int min , int sec );
    void setAleFreqs(ale_call_freqs_t callFreqs,ale_call_freqs_t workFreqs);
	
    void set_freq(long freq);
    void set_tx(int8s mode);
    void set_rx(int8s mode);
    void set_wait_msg(int8s msg);
    void set_caller_mode(bool mode);
    int8s get_packet_num();
    int8s set_packet_num(int8s num_msg_head);
    void get_msg_fragment(int8s num, int8s* data);
    void set_timer(int32s interval);
    int32s get_timer_counter();
    void start_timer(int32s interval);
    void stop_timer();
    bool get_timer_state();

    //sigc::signal<void, Status/*new_status*/> statusChanged;
    sigc::signal<void, uint8_t/*subdevice_code*/, uint8_t/*error_code*/> dspHardwareFailed;
	sigc::signal<void, int/*new_state*/> aleStateChanged;
	sigc::signal<void, uint8_t/*new_value*/> aleVmProgressUpdated;

private:
	friend class Dispatcher;
    friend class AleMain;
    friend class VoiceService;




    ale_data temp_ale;
    ext_ale_settings ale_settings;
    ContTimer* timer;
    AleMain* ale_main;
    AleDataTransport* ale_data_transport;
    AleFxn* ale_fxn;
    //AleState current_state;

    AleService(Dispatcher* dispatcher);
    ~AleService();
	void printDebugVmMessage(int groups, int packets, voice_message_t &message);
	void setAleVmProgress(uint8_t value);
	bool startAleSession();
	void stopAleSession();
	
	void aleglobaltimerExpired( bool error);

	Dispatcher *dispatcher;
    OldAleData ale;
    void get_msg_size();
    void startNextPacketTxCycle();
    void setPacketRxPhase();
    bool processPacketReceivedPacket(uint8_t *data);
    void aleprocessTimerRxPacketTxLinkReleaseExpired();
    void aleprocessTX_modem(int8_t packet_type, int8_t *data, int8_t length);
    void aleprocessModemPacketFailedRx(DspController::ModemPacketType type);
    void forwardDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code);
    void timer_fxn();
    void aleprocessModemPacketFailedTx();
};

}

#endif /* FIRMWARE_APP_MRD_ALESERVICE_H_ */
