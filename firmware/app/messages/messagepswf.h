/**
 ******************************************************************************
 * @file    messagepswf.h
 * @author  Smalev Sergey, PMR dept. software team, ONIIP, PJSC
 * @date    22.03.2016
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_MESSAGEPSWF_H_
#define FIRMWARE_APP_MESSAGEPSWF_H_

#include "dsp/dspcontroller.h"
#include "dsp/dsptransport.h"
#include "multiradio.h"

#include "qmobject.h"

class QmTimer;
class QmIopin;

namespace MessagesPSWF {

class MessageSendPswf;
struct PswfContent;


class MessagePswf : public QmObject
{
public:
    enum PswfMessageIndicator {
        TransmitPackage = 20,
        ReceivePackage = 30
    };

    enum UartDeviceAddress {
        TransmissionToDsp = 0x72,
        TransmissionFromDsp = 0x73,
        ReceivingToDsp = 0x62,
        ReceivingFromDsp = 0x63
    };


    MessagePswf(/*int uart_resource, int reset_iopin_resource,* QmObject *parent*/);
    ~MessagePswf();

    void  MessageSendPswf(UartDeviceAddress UartDevice, PswfMessageIndicator Indicator, float SNR,float FreqMin, int S_ADR, int R_ADR, int COM_N, float L_CODE,int isGPS);

    float CalcShiftFreq(int RN_KEY, int SEC_MLT, int DAY, int HRS, int MIN);

    float Calc_LCODE(int R_ADR, int S_ADR, int COM_N,int RN_KEY,int DAY, int HRS, int MIN,int SEC);

    int mod(int a,int b);

    uint8_t* CreateFrame(PswfContent Content);
    void     ParsingFrame(uint8_t * data,PswfContent content);

    int* getUserData();

    int* getGPSData();

    sigc::signal<void> setRadioCompleted;

private:
    friend struct PswfContent;

    //    enum Module {
    //        RxRadiopath,
    //        TxRadiopath
    //    };
    //    enum RxParameterCode {
    //        RxFrequency = 1,
    //        RxRadioMode = 2
    //    };
    //    enum TxParameterCode {
    //        TxFrequency = 1,
    //        TxRadioMode = 2
    //    };
    //    union ParameterValue {
    //        uint32_t frequency;
    //        RadioMode radio_mode;
    //    };

    //    void initResetState();
    //    void processStartup(uint16_t id, uint16_t major_version, uint16_t minor_version);
    //    void processStartupTimeout();
    //    bool startRadioOff();
    //    bool startRadioRxMode();
    //    bool startRadioTxMode();
    //    bool startRadioCarrierTx();
    //    void processRadioState();
    //    void syncNextRadioState();
    //    void processCommandTimeout();
    //    void processCommandResponse(bool success, Module module, int code, ParameterValue value);
    //    void syncPendingCommand();
    //    bool resyncPendingCommand();
    //    void sendCommand(Module module, int code, ParameterValue value);
    //    void processReceivedFrame(uint8_t address, uint8_t *data, int data_len);

    //    bool is_ready;
    //    QmIopin *reset_iopin;
    //    DspTransport *transport;
    //    QmTimer *startup_timer, *command_timer;
    //    enum {
    //        radiostateSync,
    //        radiostateCmdModeOffRx,
    //        radiostateCmdModeOffTx,
    //        radiostateCmdRxFreq,
    //        radiostateCmdTxFreq,
    //        radiostateCmdRxOff,
    //        radiostateCmdTxOff,
    //        radiostateCmdRxMode,
    //        radiostateCmdTxMode,
    //        radiostateCmdCarrierTx
    //    } radio_state;
    //    RadioMode current_radio_mode;
    //    RadioOperation current_radio_operation;
    //    uint32_t current_radio_frequency;
    //    DspCommand *pending_command;
};

} /* namespace MessagePswf */

#endif /* FIRMWARE_APP_MESSAGEPSWF_H_ */
