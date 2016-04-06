/**
 ******************************************************************************
 * @file    dspcontroller.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    22.12.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_DSP_DSPCONTROLLER_H_
#define FIRMWARE_APP_DSP_DSPCONTROLLER_H_

#include "qmobject.h"

class QmTimer;
class QmIopin;

namespace Multiradio {

class DspTransport;
struct DspCommand;

class DspController : public QmObject
{
public:
    enum RadioMode {
        RadioModeOff = 0,
        RadioModeCarrierTx = 1,
        RadioModeUSB = 7,
        RadioModeFM = 9,
        RadioModeSazhenData = 11,
        RadioModePSWF = 20
    };
    enum RadioOperation {
        RadioOperationOff,
        RadioOperationRxMode,
        RadioOperationTxMode,
        RadioOperationCarrierTx
    };
    enum AudioMode {
        AudioModeOff = 0,
        AudioModeRadiopath = 1,
        AudioModeMicTest = 2,
        AudioModePlayLongSignal = 3,
        AudioModePlayShortSignal = 4
    };


    DspController(int uart_resource, int reset_iopin_resource, QmObject *parent);
    ~DspController();
    bool isReady();
    void startServicing();
    void setRadioParameters(RadioMode mode, uint32_t frequency);
    void setRadioOperation(RadioOperation operation);
    void setRadioSquelch(uint8_t value);
    void setAudioVolumeLevel(uint8_t volume_level);
    void setAGCParameters(uint8_t agc_mode,int RadioPath);
    void setPSWFParametres(int RadioPath, int LCODE, int RN_KEY,int COM_N,uint32_t FREQ);
    void setPswfMode();
    void transmitPswf();
    void parsingData();
    void *getContentPSWF();
    int pswf_mas[12];

    sigc::signal<void> started;
    sigc::signal<void> setRadioCompleted;
    sigc::signal<void> savePacketPswf;

private:
    friend struct DspCommand;

    enum Module {
        RxRadiopath,
        TxRadiopath,
        Audiopath,
        PSWFReceiverRx1,	//0x61
        PSWFReceiverRx2,	//0x63
        PSWFReceiverTx,		//0x60
        PSWFTransmitterRx,	//0x73
        PSWFTransmitterTx	//0x72

    };
    enum RxParameterCode {
        RxFrequency = 1,
        RxRadioMode = 2,
		RxSquelch = 4,
        AGCRX = 8
    };
    enum TxParameterCode {
        TxFrequency = 1,
        TxRadioMode = 2,
		TxPower = 4,
        AGCTX = 7
    };
    enum AudioParameterCode {
        AudioModeParameter = 0,
        AudioVolumeLevel = 2,
        AudioMicAmplify = 3
    };

    enum PSWF
    {
        PSWF_RX = 0,
        PSWF_TX = 1
    };

    union ParameterValue {
        uint32_t frequency;
        uint8_t power;
        RadioMode radio_mode;
        AudioMode audio_mode;
        uint8_t squelch;
        uint8_t volume_level;
        uint8_t mic_amplify;
        uint8_t agc_mode;
        uint8_t pswf_indicator;
    };


    struct PswfContent{
        uint8_t TYPE;
        uint32_t Frequency;
        uint8_t SNR;
        uint8_t R_ADR;
        uint8_t S_ADR;
        uint8_t COM_N;
        uint8_t L_CODE;
        uint8_t RN_KEY;
        uint8_t Conditional_Command;
    } ContentPSWF;




    void initResetState();
    void processStartup(uint16_t id, uint16_t major_version, uint16_t minor_version);
    void processStartupTimeout();
    bool startRadioOff();
    bool startRadioRxMode();
    bool startRadioTxMode();
    bool startRadioCarrierTx();
    void processRadioState();
    void syncNextRadioState();
    void processCommandTimeout();
    void processCommandResponse(bool success, Module module, int code, ParameterValue value);
    void syncPendingCommand();
    bool resyncPendingCommand();
    void sendCommand(Module module, int code, ParameterValue value);
    void processReceivedFrame(uint8_t address, uint8_t *data, int data_len);



    bool is_ready;
    QmIopin *reset_iopin;
    DspTransport *transport;
    QmTimer *startup_timer, *command_timer,*timer_tx_pswf;
    enum {
        radiostateSync,
        radiostateCmdModeOffRx,
        radiostateCmdModeOffTx,
		radiostateCmdTxPower,
        radiostateCmdRxFreq,
        radiostateCmdTxFreq,
        radiostateCmdRxOff,
        radiostateCmdTxOff,
        radiostateCmdRxMode,
        radiostateCmdTxMode,
        radiostateCmdCarrierTx,
        radiostateCmdPswfTx,
        radiostateCmdPswfRx
    } radio_state;
    RadioMode current_radio_mode;
    RadioOperation  current_radio_operation;
    uint32_t current_radio_frequency;
    DspCommand *pending_command;

    int command_30 = 0;
    int command_rx_30 = 0;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_DSP_DSPCONTROLLER_H_ */
