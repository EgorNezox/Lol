/*
 * navigator.cpp
 *
 *  Created on: 13 янв. 2016 г.
 *      Author: usrpetr
 */

#define QMDEBUGDOMAIN	navigation
#include "qmdebug.h"
#include "qmiopin.h"
#include "qmuart.h"
#include "qmtimer.h"
#include "qmthread.h"
#include "qmtimer.h"
#include <string.h>
#include <list>
#include <cstring>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include "stdio.h"
#include "navigator.h"
#include "../../../system/init.h"

#define NEW_GPS_PARSING 0
#define GPS_CORRECT 0
#define GPS_CORRECT_VALUE 1

namespace Navigation {

using namespace std;

Navigator::Navigator(int uart_resource, int reset_iopin_resource, int ant_flag_iopin_resource, int sync_pulse_iopin_resource) {
//#if defined(PORT__TARGET_DEVICE_REV1)
	reset_iopin = new QmIopin(reset_iopin_resource, this);
	reset_iopin->writeOutput(QmIopin::Level_Low);
	QmUart::ConfigStruct uart_config;
	uart_config.baud_rate = 115200;
	uart_config.stop_bits = QmUart::StopBits_1;
	uart_config.parity = QmUart::Parity_None;
	uart_config.flow_control = QmUart::FlowControl_None;
	uart_config.rx_buffer_size = 1024 * 8;
	uart_config.tx_buffer_size = 1024;
    uart_config.io_pending_interval = 100;
	uart = new QmUart(uart_resource, &uart_config, this);
	uart->dataReceived.connect(sigc::mem_fun(this, &Navigator::processUartReceivedData));
	uart->rxError.connect(sigc::mem_fun(this, &Navigator::processUartReceivedErrors));
	reset_iopin->writeOutput(QmIopin::Level_High);
	ant_flag_iopin = new QmIopin(ant_flag_iopin_resource, this);
	//qmDebugMessage(QmDebug::Dump, "ant_flag_iopin = %d", ant_flag_iopin->readInput());
	sync_pulse_iopin = new QmIopin(sync_pulse_iopin_resource, this);
	sync_pulse_iopin->inputTriggerOnce.connect(sigc::mem_fun(this, &Navigator::processSyncPulse));


	setMinimalActivityMode(false);

    CoordDate.status = false;

    config_timer = new QmTimer(true, this);
    config_timer->timeout.connect(sigc::mem_fun(this, &Navigator::processConfig));
    config_timer->start(3000); //tested on receivers versions 3.1, 4.1

}

Navigator::~Navigator() {
//#if defined(PORT__TARGET_DEVICE_REV1)
	uart->close();
    //#endif /* PORT__TARGET_DEVICE_REV1 */
}

Coord_Date Navigator::getCoordDate()
{
#ifndef PORT__PCSIMULATOR
    return CoordDate;
#endif
}

int Navigator::Calc_LCODE(int R_ADR, int S_ADR, int COM_N, int RN_KEY, int DAY, int HRS, int MIN, int SEC)
{
    int L_CODE = (R_ADR + S_ADR + COM_N + RN_KEY + SEC + MIN + HRS + DAY) % 100;
    return L_CODE;
}

int Navigator::Calc_LCODE_SMS_call(int R_ADR, int S_ADR, int CYC_N, int RN_KEY, int DAY, int HRS, int MIN, int SEC)
{
    int L_CODE = (R_ADR + S_ADR + CYC_N + RN_KEY + SEC + MIN + HRS + DAY) % 100;
    return L_CODE;
}


int Navigator::Calc_LCODE_RETR(int RP_ADR,int R_ADR, int COM_N, int RN_KEY, int DAY, int HRS, int MIN, int SEC)
{
    int L_CODE = (RP_ADR + R_ADR  + COM_N + RN_KEY + SEC + MIN + HRS + DAY) % 100;
    return L_CODE;
}

int Navigator::Calc_LCODE_SMS(int R_ADR, int S_ADR, int WZN, int RN_KEY, int DAY, int HRS, int MIN, int SEC)
{
    int L_CODE = (R_ADR + S_ADR + WZN + RN_KEY + SEC + MIN + HRS + DAY) % 100;
    return L_CODE;
}


//#if defined(PORT__TARGET_DEVICE_REV1)
void Navigator::processUartReceivedData() {
   // qmDebugMessage(QmDebug::Warning, "processUartReceivedData() start");
	//qmDebugMessage(QmDebug::Dump, "ant_flag_iopin = %d", ant_flag_iopin->readInput());
	//qmDebugMessage(QmDebug::Warning, "processUartReceivedData() end");
    return;
}

void Navigator::processUartReceivedErrors(bool data_errors, bool overflow) {
//	if (data_errors)
//		qmDebugMessage(QmDebug::Warning, "uart rx data errors");
//	if (overflow)
//		qmDebugMessage(QmDebug::Warning, "uart rx overflow");
    uart->readData(0, uart->getRxDataAvailable()); // flush received chunks
}

#if NEW_GPS_PARSING

void Navigator::parsingData(uint8_t data[])
{
    // $GPRMC,hhmmss.sss,A,GGMM.MM,P,gggmm.mm,J,v.v,b.b,ddmmyy,x.x,n,m*hh<CR><LF>
    // $GPZDA,172809.456,12,07,1996,00,00*45

	char *str_data = (char*) data;
	char *rmc = nullptr;
	char *zda = nullptr;
	float val = 0;
	char valid;

	if (str_data != NULL)
	{
	  rmc = strstr((const char*)str_data,(const char*)"$GPRMC");
	  zda = strstr((const char*)str_data,(const char*)"$GPZDA");
	}

	if (zda != nullptr)
	sscanf(zda,"$GPZDA,%6c",(char*)&CoordDate.time);
	if (rmc != nullptr)
	sscanf(rmc,"$GPRMC,%f,%c,%11c,%12c",&val,&valid,(char*)&CoordDate.latitude,(char*)&CoordDate.longitude);

	if (valid == 'A'){
		PswfSignal(true);
		CoordDate.status = true;
	}
	else{
		PswfSignal(false);
		CoordDate.status = false;
	}

    qmDebugMessage(QmDebug::Dump, "parsing result:  %s %s %s %s",
    (char*)CoordDate.time,
	(char*)CoordDate.data,
	(char*)CoordDate.latitude,
	(char*)CoordDate.longitude);

}

#else

void Navigator::parsingData(uint8_t data[])
{
    // $GPRMC,hhmmss.sss,A,GGMM.MM,P,gggmm.mm,J,v.v,b.b,ddmmyy,x.x,n,m*hh<CR><LF>
    // $GPZDA,172809.456,12,07,1996,00,00*45

    char* rmc = (char*)data;
    char* rmc_dubl = nullptr;
    char* zda = (char*)data;
    char* zda_dubl = nullptr;

    while (rmc != NULL){
        rmc = strstr((const char*)rmc,(const char*)"$GPRMC");
        isZda = false;
        if (zda != NULL){
        	isZda = true;
            zda = strstr((const char*)zda,(const char*)"$GPZDA");
        } else
        	isZda = false;
        if (rmc != NULL){
            rmc_dubl = rmc;
            *rmc++;
        }
        if (zda != NULL){
            zda_dubl = zda;
            *zda++;
        }
    }

    if (zda_dubl == nullptr)
        return;

    char zda_text[6];
    for(int i = 0;i<6;i++){
        zda_text[i] = zda_dubl[i+7];
    }

    memcpy(&CoordDate.time,&zda_text,6);

#if GPS_CORRECT

    char c_sec[3] = {0,0,0};
    char c_min[3] = {0,0,0};
    char c_hour[3] = {0,0,0};
    memcpy(&c_hour[0],&CoordDate.time[0],2);
    memcpy(&c_min[0],&CoordDate.time[2],2);
    memcpy(&c_sec[0],&CoordDate.time[4],2);
    uint8_t sec = atoi(c_sec);
    uint8_t min = atoi(c_min);
    uint8_t hour = atoi(c_hour);

    sec += GPS_CORRECT_VALUE;
    if (sec >= 60) {
        sec %= 60;
        min++;
        if (min >= 60) {
            min %= 60;
            hour++;
            if (hour >= 24) {
                hour %= 24;
                min = 0;
                sec = 0;
            }
        }
    }
    CoordDate.time[0] = hour/10 + 48;
    CoordDate.time[1] = hour%10 + 48;
    CoordDate.time[2] = min/10 + 48;
    CoordDate.time[3] = min%10 + 48;
    CoordDate.time[4] = sec/10 + 48;
    CoordDate.time[5] = sec%10 + 48;

#endif

    char* zda_data[3]; char* dat;
    dat = strstr((const char*)zda_dubl,(const char*)",");
    if (dat != NULL){
        dat++; if (dat == NULL) return;
        for(int i = 0; i<3;i++)
        {
            dat = strstr((const char*)dat,(const char*)","); if (dat == NULL) return;
            dat++;
            if (dat == NULL) return;
            zda_data[i] = dat;
            if (i < 2)
                memcpy(&CoordDate.data[i*2],zda_data[i],2); else
                memcpy(&CoordDate.data[4],zda_data[i],4);
        }

    }

    if (rmc_dubl == nullptr)
        return;

    char *param_start = rmc_dubl;
    char *param_end    = rmc_dubl;

    std::vector<std::string> parse_elem;
    std::string str;

    while (param_start != NULL){
        param_start = strstr((const char*)param_start,(const char*)",");
        if (param_start != NULL) {
            *param_start++;
            param_end =   strstr((const char*)param_start,(const char*)",");
            if (param_end != NULL){
                str.clear();
                str.append(param_start);
                int ind1 = strlen((const char*)param_start);
                int ind2 = strlen((const char*)param_end);
                ind1 = ind1 - ind2;
                str = str.substr(0, ind1);
                if (strstr(str.c_str(),"\r\n") != 0) break;
                else
                    parse_elem.push_back(str);

            } else{
                str.clear();
                if (strstr(str.c_str(),"\r\n") == 0)
                    str.append(param_start);
                parse_elem.push_back(str);
                break;
            }

        }

    }

    if ((parse_elem.size() > 2) && (parse_elem.at(1).compare("A") == 0))
    { // проверка по статусу gps
        CoordDate.status = true;

        if (parse_elem.size() > 3){
            memcpy(&CoordDate.latitude,parse_elem.at(2).c_str(),parse_elem.at(2).size());
            memcpy(&CoordDate.latitude[parse_elem.at(2).size()],parse_elem.at(3).c_str(),parse_elem.at(3).size());
            redactCoordForSpec(CoordDate.latitude,5);

        }
        if (parse_elem.size() > 5){
            memcpy(&CoordDate.longitude,parse_elem.at(4).c_str(),parse_elem.at(4).size());
            memcpy(&CoordDate.longitude[parse_elem.at(4).size()],parse_elem.at(5).c_str(),parse_elem.at(5).size());
            redactCoordForSpec(CoordDate.longitude,6);
        }

        PswfSignal(true);
    }

    else {
    	PswfSignal(false);
    	CoordDate.status = false;
    }
    qmDebugMessage(QmDebug::Dump, "parsing result:  %s %s %s %s", (char*)CoordDate.time,(char*)CoordDate.data,(char*)CoordDate.latitude,(char*)CoordDate.longitude);

}

void Navigator::redactCoordForSpec(uint8_t *input, int val){
	    char mas[4];
	    double value = 0;
	    float sec_msec = 0; uint16_t a1,a2; char param1, param2;

	    memcpy(&mas,&input[val],4);
	    value = atoi(mas);
	    sec_msec = (value / 10000) * 60;
	    a1 = (uint16_t)sec_msec;
	    sec_msec -= a1;
	    a2 = (uint16_t)(round(sec_msec*100));
	    int cnt = val;
	    for(int i  = 0; i<2;i++)
	    {
	    	if ( i % 2 != 0) {param1 = (char)(a2 / 10) + '0'; param2 = (char)(a2 % 10) + '0'; }
	    	else {param1 = (char)(a1 / 10) + '0';param2 = (char)(a1 % 10) + '0';}

	    	memcpy(&input[cnt],&param1,1);
	    	memcpy(&input[cnt + 1],&param2,1);
	    	cnt += 2;
	    }
}

#endif

void Navigator::processConfig() {

//	const char * const pps = "$PSTMGETPAR,1301,0.005*\r\n"; // "$PSTMNMEAONOFF,0\r\n"; //"$PSTMRESTOREPAR\r\n";
//	uart->writeData((uint8_t *)pps, strlen(pps));

//	const char * const start =     "$PSTMSETPAR,1201,01184360*\r\n"; // "$PSTMNMEAONOFF,0\r\n"; //"$PSTMRESTOREPAR\r\n";
//    uart->writeData((uint8_t *)start, strlen(start));

		const char * const pps = "$PSTMGETPAR,1301,0.005000*03\r\n"; // "$PSTMNMEAONOFF,0\r\n"; //"$PSTMRESTOREPAR\r\n";
		uart->writeData((uint8_t *)pps, strlen(pps));

    const char * const start2 = "$PSTMSETPAR,1201,0x01000040*54\r\n"; // "$PSTMNMEAONOFF,0\r\n"; //"$PSTMRESTOREPAR\r\n";
   uart->writeData((uint8_t *)start2, strlen(start2));

   const char * const start3 = "$PSTMSETPAR,1210,0x00000000*51\r\n"; // "$PSTMNMEAONOFF,0\r\n"; //"$PSTMRESTOREPAR\r\n";
  uart->writeData((uint8_t *)start3, strlen(start3));

  const char * const start4 = "$PSTMSETPAR,1211,0x00000000*50\r\n"; // "$PSTMNMEAONOFF,0\r\n"; //"$PSTMRESTOREPAR\r\n";
 uart->writeData((uint8_t *)start4, strlen(start4));

 const char * const start1 =     "$PSTMSAVEPAR\r\n"; // "$PSTMNMEAONOFF,0\r\n"; //"$PSTMRESTOREPAR\r\n";
     uart->writeData((uint8_t *)start1, strlen(start1));

//    const char * const start1 =    "$PSTMGETPAR,1301*\r\n"; // "$PSTMNMEAONOFF,0\r\n"; //"$PSTMRESTOREPAR\r\n";
//        uart->writeData((uint8_t *)start1, strlen(start1));

//    const char * const config_sentences ="$PORZB,ZDA,1*3B\r\n" "$POPPS,P,S,U,1,1000,,*06\r\n"; // "$PKON1,0,2,, ,0000,A*68\r\n" "$PONME,2,4,1*42\r\n"
//    qmDebugMessage(QmDebug::Dump, "processConfig()\n%s", config_sentences);
//    uart->writeData((uint8_t *)config_sentences, strlen(config_sentences));
}

void Navigator::set1PPSModeCorrect(bool value)
{
	pps_correct = value;
}

bool Navigator::get1PPSModeCorrect()
{
	return pps_correct;
}


void Navigator::processSyncPulse(bool overflow)
{
    static uint16_t to_mode_time = 0;
    ++to_mode_time;

    #ifdef PORT__TARGET_DEVICE_REV1
    int i  = get_tim1value();
    #endif
    int res = 0;
    bool param = (to_mode_time % 50 == 0);
    if (CoordDate.status)
    {
#ifdef PORT__TARGET_DEVICE_REV1
        res = tune_frequency_generator(i, param);
#endif
        //qmDebugMessage(QmDebug::Warning, " =====> Correct  DAC: %i FROM DELTA %i", res, i);
    }
    if (param)
        to_mode_time = 0;


//	if (overflow)
//		qmDebugMessage(QmDebug::Warning, "sync pulse overflow detected !!!");

	uint8_t data[1024];
	int16_t data_read = 0;
	data_read = uart->readData(data, 1024 - 1);
    //qmDebugMessage(QmDebug::Warning, "data_read = %d", data_read);
	if (data_read > 0)
	{
		data[data_read] = '\0';
		parsingData(data);
	}
	syncPulse();



}

void Navigator::coldStart()
{
//	const char * const config_sentences = "$PORST,F*20\r\n";
//    qmDebugMessage(QmDebug::Dump, "processConfig()\n%s", config_sentences);
//    uart->writeData((uint8_t *)config_sentences, strlen(config_sentences));
//
//    QmThread::msleep(5000);
//
//    const char * const start_sentences = "$PORZB,ZDA,1*3B\r\n" "$POPPS,P,S,U,1,1000,,*06\r\n";
//    qmDebugMessage(QmDebug::Dump, "processConfig()\n%s", config_sentences);
//    uart->writeData((uint8_t *)start_sentences, strlen(start_sentences));

	const char * const config_sentences = "$PSTM,8\r\n";
	uart->writeData((uint8_t *)config_sentences, strlen(config_sentences));
}

void Navigator::setMinimalActivityMode(bool enabled) {
	minimal_activity_mode = enabled;
	if (enabled) {
		uart->close();
		uart->readData(0, uart->getRxDataAvailable()); // flush received chunks
		sync_pulse_iopin->setInputTriggerMode(QmIopin::InputTrigger_Disabled);
	} else {
	    for(int i = 0;i<11;i++){
	      CoordDate.longitude[i] = 0;
	      CoordDate.latitude[i] = 0;
	    }
	    CoordDate.longitude[11] = 0;
	    for(int i = 0;i<10;i++){
	        CoordDate.data[i] = 0;
	        CoordDate.time[i] = 0;
	    }
		uart->open();
		sync_pulse_iopin->setInputTriggerMode(QmIopin::InputTrigger_Rising, QmIopin::TriggerOnce);
	}
}

} /* namespace Navigation */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(navigation, LevelOff)
#include "qmdebug_domains_end.h"
