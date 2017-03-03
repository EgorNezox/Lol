/*
 * voicemail.cpp
 *
 *  Created on: 18.01.2017
 *      Author: Alex
 */


#include <math.h>
#include <string.h>
#include <stdio.h>
#define QMDEBUGDOMAIN mrd_aleservice
#include "qmdebug.h"
#include "qmcrc.h"
#include "qmendian.h"

#include "dispatcher.h"
#include "voiceserviceinterface.h"
#include "dsp/dspcontroller.h"

#include "aleservice.h"

namespace Multiradio {

typedef QmCrc<uint32_t, 32, 0x04c11db7, 0xffffffff, true, 0xffffffff> CRC32;

AleService::AleService(Dispatcher *dispatcher) :
	QmObject(dispatcher),
	dispatcher(dispatcher)
{
    dispatcher->dsp_controller->hardwareFailed.connect(sigc::mem_fun(this, &AleService::forwardDspHardwareFailure));
    //if (dispatcher->navigator != 0)
    dispatcher->dsp_controller->vm1PpsPulse.connect(sigc::mem_fun(this, &AleService::aleprocess1PPS));
    dispatcher->dsp_controller->transmittedModemPacket.connect(sigc::mem_fun(this, &AleService::aleprocessModemPacketTransmitted));
    //dispatcher->dsp_controller->failedTxModemPacket.connect(sigc::mem_fun(this, &MainServiceInterface::aleprocessModemPacketFailedTx));
    dispatcher->dsp_controller->failedTxModemPacket.connect(sigc::mem_fun(this, &AleService::aleprocessModemPacketFailedTx));
    dispatcher->dsp_controller->receivedModemPacket.connect(sigc::mem_fun(this, &AleService::aleprocessModemPacketReceived));
    dispatcher->dsp_controller->startedRxModemPacket_packHead.connect(sigc::mem_fun(this, &AleService::aleprocessModemPacketStartedRxPackHead));
    dispatcher->dsp_controller->failedRxModemPacket.connect(sigc::mem_fun(this, &AleService::aleprocessModemPacketFailedRx));
    timer = new ContTimer();
    timer->timerTimeout.connect(sigc::mem_fun(this, &AleService::timer_fxn));
    ale_fxn = new AleFxn( timer, dispatcher, &temp_ale, &ale_settings, &ale);
    ale_main = new AleMain( timer, &temp_ale, &ale_settings , ale_fxn );
    ale_data_transport = new AleDataTransport( timer, &temp_ale, &ale_settings , ale_fxn );
}

AleService::~AleService()
{
    delete timer;
    delete ale_main;
    delete ale_data_transport;
}

void AleService::aleprocessModemPacketFailedTx()
{
    // send to log
}

void AleService::setAleState(bool caller, int8s superphase)
{

}

void AleService::timer_fxn()
{
    if(ale_settings.superphase==0)

        return;
    if(ale_settings.caller)
//	CALLER LOGIC
    {
        switch(ale_settings.superphase)
        {
/* CALL */	case 1:
                ale_main->call_tx_mgr();
                break;
/*LINK SET*/case 2:
                ale_main->link_set_tx_mgr();
                break;
/*SH_PROBE*/case 3:
                ale_main->short_probe_tx_mgr();
                break;
/*SH_QUAL*/	case 4:
                ale_main->short_sound_qual_tx_mgr();
                break;
/*L_PROBE*/	case 5:
                break;
/* L_QUAL */case 6:
                break;
/*MSG_HEAD*/case 7:
                ale_data_transport->msg_head_tx_mgr();
                break;
/* DATA */	case 9:
                ale_data_transport->data_tx_mgr();
                break;
/*DATA+LR*/	case 10:
                ale_data_transport->data_end_tx_mgr();
                break;
        }
    }
    else
//	RESPONDER_LOGIC
    {
        switch(ale_settings.superphase)
        {
/* CALL */	case 1:
                ale_main->call_rx_mgr();
                break;
/*LINK SET*/case 2:
                ale_main->link_set_rx_mgr();
                break;
/*SH_PROBE*/case 3:
                ale_main->short_probe_rx_mgr();
                break;
/*SH_QUAL*/	case 4:
                ale_main->short_sound_qual_rx_mgr();
                break;
/*L_PROBE*/	case 5:
                break;
/* L_QUAL */case 6:
                break;
/*MSG_HEAD*/case 7:
                ale_data_transport->msg_head_rx_mgr();
                break;
            case 8:
                //ale_data_transport->msg_head_rx_error_mgr();  // THIS CODE IS IN MODE 7
                break;
/* DATA */	case 9:
                ale_data_transport->data_rx_mgr();
                break;
/*DATA+LR*/	case 10:
                //data_end_rx_mgr();
                break;
        }
        if((ale_settings.phase==((ale_fxn->get_packet_num()-1)*3+2))&&(!temp_ale.pause_state))
        {
        	ale_fxn->ale_log("ALL DATA RECEIVED");
        	ale.vm_progress=100;
        }
    }
    setAleState(ale_settings.caller,ale_settings.superphase);
}

void AleService::initAle(ale_call_freqs_t call_freqs, ale_call_freqs_t work_freqs, int8s own_adress)
{
    ale_settings.own_adress=own_adress;
    ale_settings.call_freq_num=call_freqs.size();
    ale_settings.work_freq_num=work_freqs.size();
    for(int8s i=0;i<ale_settings.call_freq_num;i++)
        ale_settings.call_freq[i]=call_freqs[i];
    for(int8s i=0;i<ale_settings.work_freq_num;i++)
        ale_settings.work_freq[i]=work_freqs[i];
    ale.ManualTimeMode=false;
    ale.probe_on=false;
}

void AleService::setManualTimeMode(bool mode)
{
	ale.ManualTimeMode=mode;
}

void AleService::setProbeMode(bool probe_on)
{
   ale.probe_on=probe_on;
}

#if 0 
void AleServiceInterface::forwardDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code) {
	dspHardwareFailed.emit(subdevice_code, error_code);
}
#endif
void AleService::printDebugVmMessage(int groups, int packets, voice_message_t &message) {
	qmDebugMessage(QmDebug::Info, "voice message: %d groups, %d packets", groups, packets);
	if (qmDebugIsVerbose()) {
		char *message_data_dump = new char[message.size()*3+1];
		message_data_dump[0] = 0;
		for (unsigned int i = 0; i < message.size(); i++)
			sprintf(message_data_dump + i*3, " %02X", message[i]);
		qmDebugMessage(QmDebug::Dump, "voice message data: %s", message_data_dump);
		delete[] message_data_dump;
	}
}


void AleService::startAleRx() {
	qmDebugMessage(QmDebug::Info, "starting ALE rx");
	if (!startAleSession())
		return;
	ale.f_state = alefunctionRx;
	ale.result = AleResultNone;
    //setAlePhase(ALE_RX_SETUP);
    //setAleState(AleState_RX_SCANNING);
	ale.vm_size = 0;
	ale.vm_fragments.clear();
    ale_main->start_fxn((int8s)!ale.ManualTimeMode, false, 0 , false);
}

void AleService::startAleTxVoiceMail(uint8_t address, voice_message_t message) {
	qmDebugMessage(QmDebug::Info, "starting ALE tx voice mail (address = %02u)", address);
	if (!startAleSession())
		return;
	ale.f_state = alefunctionTx;
	ale.result = AleResultNone;
	ale.address = address;
//	setAlePhase(ALE_TX_SETUP);
//	setAleState(AleState_TX_CALLING);
	ale.supercycle = 1;
	ale.cycle = 1;
	int message_size_mismatch = message.size() % 9;
	if (message_size_mismatch > 0)
		message.resize((message.size() + (9 - message_size_mismatch)), 0);
	int message_bits_size = message.size()*8;
	ale.vm_size = message_bits_size/72;
	ale.vm_f_count = ceilf((float)message_bits_size/490);
	ale.vm_fragments.resize(ale.vm_f_count);
	for (unsigned int i = 0; i < ale.vm_fragments.size(); i++) {
        ale.vm_fragments[i].num_data[0] = (i & 0x3F) << 2;
		for (unsigned int j = 1; j < sizeof(ale.vm_fragments[0].num_data); j++)
			ale.vm_fragments[i].num_data[j] = 0;
	}
	for (int m_bit_i = 0; m_bit_i < message_bits_size; m_bit_i++) {
		int f_i = m_bit_i / 490;
		int f_bit_i = 6 + (m_bit_i % 490);
		int f_byte_i = f_bit_i / 8;
		int f_byte_bit = 7 - (f_bit_i % 8);
		int m_byte_bit = 7 - (m_bit_i % 8);
		if ((message[m_bit_i/8] & (1 << m_byte_bit)) != 0)
			ale.vm_fragments[f_i].num_data[f_byte_i] |= (1 << f_byte_bit);
	}
	for (unsigned int i = 0; i < ale.vm_fragments.size(); i++) {
		CRC32 f_crc;
        f_crc.update(&(ale.vm_fragments[i].num_data[0]), sizeof(ale.vm_fragments[i].num_data));
		qmToBigEndian(f_crc.result(), (uint8_t *)&(ale.vm_fragments[i].crc));
	}
    //  COPY DATA TO NEW STRUCT
    ale_settings.data72bit_length=ale.vm_size;
    ale_settings.data490bit_length=ale.vm_f_count;
    for (unsigned int i = 0; i < ale.vm_fragments.size(); i++) {
        for(unsigned int j=0;j<66;j++)
            ale_settings.data[i*66+j]=ale.vm_fragments[i].num_data[j];
    }
    //  COPY END
	printDebugVmMessage(ale.vm_size, ale.vm_f_count, message);
	
    ale_main->start_fxn( !ale.ManualTimeMode, true, address, ale.probe_on );
}

AleResult AleService::stopAle() {
	stopAleSession();
    ale_main->stop_fxn();
	return AleResultNone;
}

int AleService::getAleState() {	//AleState AleService::getAleState() {		//	REWRITE !!!!
	return ale_settings.ale_state;//return ale.state;
}

uint8_t Multiradio::AleService::getAleVmProgress() {				//	CHECK !!!
	if(ale.vm_progress!=100)
		return ale.vm_progress;
	ale.vm_progress=0;
    return 100;
}

uint8_t AleService::getAleRxAddress() {
	return ale.address;
}

voice_message_t AleService::getAleRxVmMessage() {
	ale.vm_size=ale_settings.data72bit_length;
	ale.vm_f_idx=ale_settings.data490bit_length;
	ale.vm_f_count=ale_settings.data490bit_length;
	if (ale.vm_size == 0)
		return voice_message_t();
	int message_bits_size;
	if (ale.vm_f_idx == ale.vm_f_count) {
		message_bits_size = ale.vm_size*72;
	} else {
		message_bits_size = ale.vm_f_idx*490;
		if (!((message_bits_size % 72) == 0))
			message_bits_size += 72 - (message_bits_size % 72);
	}
	voice_message_t vm_rx_message(message_bits_size/8, 0);
	int message_bits_offset = 0;
	for (int f_i = 0; f_i < ale.vm_f_idx; f_i++) {
		int f_bits_size = (f_i == (ale.vm_f_count - 1))?(ale.vm_size*72 - (ale.vm_f_count - 1)*490):(490);
		for (int f_bit_i = 6; f_bit_i < (6 + f_bits_size); f_bit_i++) {
			int f_byte_i = f_bit_i / 8;
			int f_byte_bit = 7 - (f_bit_i % 8);
			int m_byte_i = message_bits_offset / 8;
			int m_byte_bit = 7 - (message_bits_offset % 8);
			ale.vm_fragments[f_i].num_data[f_byte_i]=ale_settings.data_packs[f_i][f_byte_i];
			if ((ale.vm_fragments[f_i].num_data[f_byte_i] & (1 << f_byte_bit)) != 0)
				vm_rx_message[m_byte_i] |= (1 << m_byte_bit);
			message_bits_offset++;
		}
	}
    return vm_rx_message;
}

void AleService::setAleVmProgress(uint8_t value) {
	if (ale.vm_progress != value) {
		qmDebugMessage(QmDebug::Info, "ale vm progress = %u", value);
		ale.vm_progress = value;
		aleVmProgressUpdated(value);
	}
}

bool AleService::startAleSession() {
	qmDebugMessage(QmDebug::Info, "ale session start");
	dispatcher->dsp_controller->setRadioOperation(DspController::RadioOperationOff);
	dispatcher->atu_controller->setMinimalActivityMode(true);
	dispatcher->power_battery->setMinimalActivityMode(true);
	return true;
}

void AleService::stopAleSession() {
	qmDebugMessage(QmDebug::Info, "ale session stop");
	dispatcher->atu_controller->setMinimalActivityMode(false);
	dispatcher->power_battery->setMinimalActivityMode(false);
	dispatcher->navigator->setMinimalActivityMode(false);
}

void AleService::aleprocessModemPacketTransmitted(DspController::ModemPacketType type) {
    ale_main->modem_packet_transmitter_complete();
}

// ������� ��������� ����� � ������������
void AleService::aleprocessModemPacketReceived(DspController::ModemPacketType type, uint8_t snr, uint8_t errors, DspController::ModemBandwidth bandwidth, uint8_t* data, int data_len) {
//	int8_t snr_db_value = convertSnrFromPacket(snr);
//	qmDebugMessage(QmDebug::Info, "ale received modem packet (type = %u, bandwidth = %u) with SNR = %d dB and errors %u%%", type, bandwidth, snr_db_value, errors);
    //	FOR ALE_MAIN WORKING
    ale_main->modem_packet_receiver((int8s)type, snr, errors, (int8s)bandwidth,(int8s*)data, data_len);
    return;
}
// ������� ��������� ����� � ���������� ����������� � �������� ���������
void AleService::aleprocessModemPacketStartedRxPackHead(uint8_t snr, uint8_t errors, DspController::ModemBandwidth bandwidth, uint8_t param_signForm, uint8_t param_packCode, uint8_t* data, int data_len) {
//	int8_t snr_db_value = convertSnrFromPacket(snr);
//	qmDebugMessage(QmDebug::Info, "ale received modem VM packHead (bandwidth = %u, signForm = %u, param_packCode = %u) with SNR = %d dB and errors %u%%", bandwidth, param_signForm, param_packCode, snr_db_value, errors);
    //	FOR ALE_MAIN WORKING
    int8s data2[2] = {param_signForm, param_packCode};
    ale_main->modem_packet_receiver( PACK_HEAD, snr, errors, (int8s)bandwidth, data2, 2 );
	return;
}

void AleService::aleprocess1PPS(	int hrs ,int min , int sec )
{
    ale_main->fxn_1pps(hrs, min, sec);
    if(AleState1!=ale_settings.ale_state)
    {
    	AleState1=ale_settings.ale_state;
    	aleStateChanged(AleState1);
    	ale_fxn->ale_log("INTERFACE CHANGED, STATE %u",AleState1);
    }
}

//void AleService::get_msg_size()
//{
//	return msghead_packet.msgSize;
//}

#if 0
void AleService::startNextPacketTxCycle() {
//	int sform = (ale.vm_sform_p > ale.vm_sform_c)?ale.vm_sform_p:ale.vm_sform_c;
	int sform = ale.vm_sform_p;
    //ale.timerTxPacketSync->start(ale.tPacketSync, TIMER_VALUE_tDataCycle(sform));
    //ale.tPacketSync.shift(TIMER_VALUE_tDataCycle(sform));
}

void AleService::setPacketRxPhase() {
	qmDebugMessage(QmDebug::Info, "ale vm set packet phase");
    //ale.tPacketSync.shift(ale.vm_msg_cycle*TIMER_VALUE_tDataCycle(-1));
    //ale.vm_sform_c = ALE_VM_INITIAL_SFORM;
	ale.vm_sform_p = ale.vm_sform_c;
	ale.vm_f_idx = 0;
	ale.vm_last_result = true;
	ale.vm_ack_count = 0;
	ale.vm_nack_count = 0;
    //setAlePhase(ALE_RX_VM_RX_PACKET);
	ale.result = AleResultVoiceMail;
    //stopVmMsgRxTimers();
}
#endif

bool AleService::processPacketReceivedPacket(uint8_t *data) {
	AleVmPacket *packet = (AleVmPacket *)data;
	CRC32 crc;
	crc.update(packet->num_data, sizeof(packet->num_data));
	if (!(crc.result() == qmFromBigEndian<uint32_t>((uint8_t *)&(packet->crc)))) {
		qmDebugMessage(QmDebug::Info, "ale rx vm packet with bad CRC");
		return false;
	}
	uint8_t num = (packet->num_data[0] >> 2) & 0x3F;
	if(!(num == ale.vm_f_idx)) {
		if (num == (ale.vm_f_idx - 1)) {
			qmDebugMessage(QmDebug::Info, "ale rx vm packet repeated");
			return true;
		} else {
			qmDebugMessage(QmDebug::Info, "ale rx vm packet with bad number");
			return false;
		}
	}
	std::copy(packet->num_data, packet->num_data + sizeof(packet->num_data), ale.vm_fragments[ale.vm_f_idx].num_data);
	ale.vm_f_idx++;
	setAleVmProgress((int)(100*ale.vm_f_idx/ale.vm_f_count));
	return true;
}

void AleService::aleprocessModemPacketFailedRx(DspController::ModemPacketType type) {
    if (ale.phase == ALE_RX_VM_RX_PACKET) {
        QM_ASSERT(type == DspController::modempacket_packHead);
        qmDebugMessage(QmDebug::Info, "ale VM packet data rx failed");
        ale.vm_packet_result = false;
        //startRxPacketResponse();
    }
}

void AleService::forwardDspHardwareFailure(uint8_t subdevice_code, uint8_t error_code) {
    dspHardwareFailed.emit(subdevice_code, error_code);
}

} /* namespace Multiradio */

//void AleService::aleStateChanged(uint8_t subdevice_code, uint8_t error_code) {
//    dspHardwareFailed.emit(subdevice_code, error_code);
//}

//void AleService::aleVmProgressUpdated(uint8_t subdevice_code, uint8_t error_code) {
//    dspHardwareFailed.emit(subdevice_code, error_code);
//}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(mrd_aleservice, LevelDefault)
#include "qmdebug_domains_end.h"
