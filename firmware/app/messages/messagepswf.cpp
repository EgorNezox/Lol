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
#include <math.h>
#include <QThread>


#define COMMAND_RECEIVE  0x50
#define COMMAND_TRANSMIT 0x80

using namespace Multiradio;

namespace MessagesPSWF {

struct PswfContent{
    int addr;
    int R_ADR;
    int indicator;
    int SNR;
    int S_ADR;
    int COM_N;
    int L_CODE;
    float Frequency;
    int Conditional_Command;
} Content;


enum TypeMess
{
    Individual = 0,
    Group  = 1
};


struct MessPSWF
{
    int DAY;
    int HRS;
    int MIN;
    int SEC;
    int COM_N;
    int S_ADR;

};


DspTransport *transport;

void MessagePswf::MessageSendPswf( UartDeviceAddress UartDevice,
                                   PswfMessageIndicator Indicator,
                                   float SNR,
                                   float FreqMin,
                                   int S_ADR,
                                   int R_ADR,
                                   int COM_N,
                                   float L_CODE,
                                   int isGPS)
{
           int *data;

            if ((Indicator == TransmitPackage))
            {

                uint8_t frame[4];
                frame[0] = 0x80;
                frame[1] = 0x2;
                frame[2] = 0x2;
                frame[3] = 0x20;

                transport->transmitFrame(0x72,frame,4); // отправка запроса на ПП� Ч


                // Заполняем структуру для передачи
                Content.R_ADR = R_ADR;
                Content.S_ADR = S_ADR;
                Content.indicator = Indicator;
                Content.SNR = SNR;
                Content.COM_N = COM_N;

                // отправка последовательности с помощью функции DSP Command

                uint8_t *packet;

                for(int i = 0; i < 30; i++)
                {

                    if (isGPS) data = getGPSData(); else data = getUserData();

                    Content.L_CODE    = Calc_LCODE(R_ADR,S_ADR,COM_N,0,data[0],data[1],data[2],data[3]); // L_CODE тестовый

                    Content.Frequency = FreqMin + CalcShiftFreq( 0, 0, data[0], data[1], data[2] );            //расчет параметра смещения рабочей частоты

                    packet = CreateFrame(Content);

                    transport->transmitFrame(0x72,packet,11);

                    QThread::msleep(1000); // задержка 1 с


                }

            }
            else
            {

                    uint8_t *frame;
                    frame[0] = 0x50;
                    frame[1] = 0x2;
                    frame[2] = 0x2;
                    frame[3] = 0x20;

                    transport->transmitFrame(0x50,frame,4); // отправка запроса на ПП� Ч

                    frame[2] = 0x1;

                    char freq[4];
                    sprintf(freq,"%f",Content.Frequency);

                    for(int i = 0; i < 4; i++)
                        frame[i+3] = freq[i];

                    transport->transmitFrame(0x50,frame,4); // установка частоты

                    uint8_t *data;

                    // выполняем прием последовательности
                    transport->receivedFrame(0x63,data,11);

            }
}


// функция-заглушка получения текущей даты с GPS
int* MessagePswf::getGPSData()
{
   int* data;

   data[0] = 0; // DAY
   data[1] = 0; // HOURS
   data[2] = 0; // MIN
   data[3] = 0; // SEC

   return data;
}

// функция-заглушка получения даты из пользовательского интерфейса
int* MessagePswf::getUserData()
{
    int* data;

    data[0] = 0; // DAY
    data[1] = 0; // HOURS
    data[2] = 0; // MIN
    data[3] = 0; // SEC

    return data;
}



void MessagePswf::ParsingFrame(uint8_t * data,PswfContent content)
{
    content.addr  = data[0];
    content.indicator = data[1];
    content.SNR  = data[2];

    char freq[4];
    for(int i = 0;i<4;i++) freq[i] = data[i+3];
    content.Frequency = atof(freq);
    content.S_ADR = data[7];
    content.R_ADR = data[8];
    content.Conditional_Command = data[9];
    content.L_CODE = data[10];
}


// функция заполения пакета
uint8_t* MessagePswf::CreateFrame(PswfContent Content)
{
      uint8_t *a;
      a[0] = (uint8_t) Content.addr;
      a[1] = (uint8_t) Content.indicator;
      a[2] = (uint8_t) Content.SNR;

      char freq[4];
      sprintf(freq,"%f", Content.Frequency);

      for(int i = 0; i < 4; i++)
          a[i+3] = freq[i];

      a[7]  = (uint8_t) Content.S_ADR;
      a[8]  = (uint8_t) Content.R_ADR;
      a[9]  = (uint8_t) Content.Conditional_Command;
      a[10] = (uint8_t) Content.L_CODE;

      return a;
}



// функция вычисления остатка от деления
int MessagePswf::mod(int a,int b)
{
    int res = a - b*floor(a/b);
    return res;
}

// функция вычисления смещения рабочей частоты
float MessagePswf::CalcShiftFreq(int RN_KEY, int SEC_MLT, int DAY, int HRS, int MIN)
{

    int TOT_W = 6671000; // ширина разрешенных участков
    float FR_SH = mod(RN_KEY + 230*SEC_MLT + 19*MIN + 31*HRS + 37*DAY, TOT_W);
    return FR_SH;
}

// функция вычисления проверочного значения пакета L_CODE
float MessagePswf::Calc_LCODE(int R_ADR, int S_ADR, int COM_N, int RN_KEY, int DAY, int HRS, int MIN,int SEC)
{
    float L_CODE = mod((R_ADR + S_ADR + COM_N + RN_KEY + SEC + MIN + HRS + DAY), 100);
    return L_CODE;
}

}
//Multiradio::DspController::sendCommand(Multiradio::DspController::Module::RxRadiopath, 2, 20);
