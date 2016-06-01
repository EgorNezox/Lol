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
#include "qmthread.h"
#include <string.h>
#include <string>
#include <list>
#include <cstring>
#include <vector>
#include <math.h>
#include "navigator.h"

namespace Navigation {

using namespace std;

Navigator::Navigator(int uart_resource, int reset_iopin_resource, int ant_flag_iopin_resource, int sync_pulse_iopin_resource) {
//#if defined(PORT__TARGET_DEVICE_REV1)
	reset_iopin = new QmIopin(reset_iopin_resource, this);
	reset_iopin->writeOutput(QmIopin::Level_Low);
	QmUart::ConfigStruct uart_config;
	uart_config.baud_rate = 57600;
	uart_config.stop_bits = QmUart::StopBits_1;
	uart_config.parity = QmUart::Parity_None;
	uart_config.flow_control = QmUart::FlowControl_None;
	uart_config.rx_buffer_size = 1024 * 8;
	uart_config.tx_buffer_size = 1024;
	uart_config.io_pending_interval = 300;
	uart = new QmUart(uart_resource, &uart_config, this);
	uart->dataReceived.connect(sigc::mem_fun(this, &Navigator::processUartReceivedData));
	uart->rxError.connect(sigc::mem_fun(this, &Navigator::processUartReceivedErrors));
	uart->open();
	reset_iopin->writeOutput(QmIopin::Level_High);
	ant_flag_iopin = new QmIopin(ant_flag_iopin_resource, this);
	qmDebugMessage(QmDebug::Dump, "ant_flag_iopin = %d", ant_flag_iopin->readInput());
	sync_pulse_iopin = new QmIopin(sync_pulse_iopin_resource, this);
	sync_pulse_iopin->setInputTriggerMode(QmIopin::InputTrigger_Rising);
	sync_pulse_iopin->inputTrigger.connect(sigc::mem_fun(this, &Navigator::processSyncPulse));
//#else
//	QM_UNUSED(uart_resource);
//	QM_UNUSED(reset_iopin_resource);
//#endif /* PORT__TARGET_DEVICE_REV1 */

    for(int i = 0;i<11;i++){
      CoordDate.longitude[i] = 0;
      CoordDate.latitude[i] = 0;
    }

    CoordDate.longitude[11] = 0;

    for(int i = 0;i<10;i++){
        CoordDate.data[i] = 0;
        CoordDate.time[i] = 0;
    }

    QmThread::msleep(500);
    const char * const config_sentences = "$PORZB,ZDA,1*3B\r\n" "$POPPS,P,S,U,1,1000,,*06\r\n";
    uart->writeData((uint8_t *)config_sentences, strlen(config_sentences));
}

Navigator::~Navigator() {
//#if defined(PORT__TARGET_DEVICE_REV1)
	uart->close();
    //#endif /* PORT__TARGET_DEVICE_REV1 */
}

Coord_Date Navigator::getCoordDate()
{
    return CoordDate;
}

int Navigator::Calc_LCODE(int R_ADR, int S_ADR, int COM_N, int RN_KEY, int DAY, int HRS, int MIN, int SEC)
{
    int L_CODE = (R_ADR + S_ADR + COM_N + RN_KEY + SEC + MIN + HRS + DAY) % 100;
    return L_CODE;
}

//#if defined(PORT__TARGET_DEVICE_REV1)
void Navigator::processUartReceivedData() {
	uint8_t data[1024];
	int64_t data_read = 0;
	while ((data_read = uart->readData(data, 1024 - 1))) {
		data[data_read] = '\0';
		parsingData(data);
//		qmDebugMessage(QmDebug::Dump, "uart dump %s", (char*)data);
	}
	qmDebugMessage(QmDebug::Dump, "ant_flag_iopin = %d", ant_flag_iopin->readInput());
}

void Navigator::processUartReceivedErrors(bool data_errors, bool overflow) {
	if (data_errors)
		qmDebugMessage(QmDebug::Info, "uart rx data errors");
	if (overflow)
		qmDebugMessage(QmDebug::Info, "uart rx overflow");
    uart->readData(0, uart->getRxDataAvailable()); // flush received chunks
}

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
            if (zda != NULL)
            zda = strstr((const char*)zda,(const char*)"$GPZDA");
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

//        if (parse_elem.size() > 0)
//            memcpy(&CoordDate.time,parse_elem.at(0).c_str(),parse_elem.at(0).size());

        if ((parse_elem.size() > 2) && (parse_elem.at(1).compare("V") != 0))
        { // проверка по статусу gps
            if (parse_elem.size() > 3){
                memcpy(&CoordDate.latitude,parse_elem.at(2).c_str(),parse_elem.at(2).size());
                memcpy(&CoordDate.latitude[parse_elem.at(2).size()],parse_elem.at(3).c_str(),parse_elem.at(3).size());
            }
            if (parse_elem.size() > 5){
                memcpy(&CoordDate.longitude,parse_elem.at(4).c_str(),parse_elem.at(4).size());
                memcpy(&CoordDate.longitude[parse_elem.at(4).size()],parse_elem.at(5).c_str(),parse_elem.at(5).size());
            }
            if (parse_elem.size() > 8)
                memcpy(&CoordDate.data,parse_elem.at(8).c_str(),parse_elem.at(8).size());
        }
        qmDebugMessage(QmDebug::Dump, "parsing result:  %s %s %s %s", (char*)CoordDate.time,(char*)CoordDate.data,(char*)CoordDate.latitude,(char*)CoordDate.longitude);

}

void Navigator::processSyncPulse() {
	syncPulse();
}
//#endif /* PORT__TARGET_DEVICE_REV1 */

} /* namespace Navigation */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(navigation, LevelDefault)
#include "qmdebug_domains_end.h"
