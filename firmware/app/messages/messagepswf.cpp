/**
 ******************************************************************************
 * @file    messagepswf.cpp
 * @author  Smalev Sergey, PMR dept. software team, ONIIP, PJSC
 * @date    22.03.2016
 *
 ******************************************************************************
 */

#include "qm.h"
#define QMDEBUGDOMAIN	dspcontroller
#include "qmdebug.h"
#include "qmendian.h"
#include "qmtimer.h"
#include "qmthread.h"
#include "qmiopin.h"

#include "messagepswf.h"
#include "dsp/dspcontroller.h"
#include "dsp/dsptransport.h"
#include "multiradio.h"

namespace MessagesPSWF {

struct PswfContent{
    int R_ADR;
    int S_ADR;
    int COM_N;
    int L_CODE;
};

float WorkingFrequency;


void MessageSendPswf(UartDeviceAddress UartDevice, PswfMessageIndicator Indicator, float SNR, float WorkingFrequency, int S_ADR, int R_ADR, int COM_N, int L_CODE)
{
    ;
};



;}
//Multiradio::DspController::sendCommand(Multiradio::DspController::Module::RxRadiopath, 2, 20);
