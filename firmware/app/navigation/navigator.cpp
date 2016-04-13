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
#include <string.h>
#include <string>
#include <list>
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
	uart_config.tx_buffer_size = 0;
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
    int L_CODE = (R_ADR + S_ADR + COM_N + RN_KEY + SEC + MIN + HRS + DAY)% 100;
    return L_CODE;
}

//#if defined(PORT__TARGET_DEVICE_REV1)
void Navigator::processUartReceivedData() {
	uint8_t data[1024];
	int64_t data_read = 0;
	while ((data_read = uart->readData(data, 1024 - 1))) {
		data[data_read] = '\0';
		qmDebugMessage(QmDebug::Dump, (char*)data);
	}

    parsingData(data);


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
    // вид строки
    // $GPRMC,hhmmss.sss,A,GGMM.MM,P,gggmm.mm,J,v.v,b.b,ddmmyy,x.x,n,m*hh<CR><LF>

    int index = 0;
    char *search = "$GPRMC";
    //char *res;


    std::list<std::string> str;

    std::string res((const char *)data);

    int index_end = 0;

    while(index <= index_end)
    {
        int probel = res.find('\r\n');
        int index = res.find(',');

        if (probel < index)
            res = res.substr(probel+1,res.length());
        else
        {
            index_end = res.find_last_of(',');
            str.push_back(res.substr(0,index));
            res = res.substr(index+1,res.length());
        }

    }

    int counter = 0;
    bool rmc = false;


    std::string s;

    for( auto iter = str.begin(); iter != str.end(); iter++)
    {
        if (*iter == "$GPRMC")
            rmc = true;

        if (rmc)
         counter++;

        if (counter == 2)
        {
            s = *iter;
            memcpy(CoordDate.time,s.c_str(),10);
            s.clear();
        }

        if (counter == 4)
        {
            s.append(*iter);
            memcpy(&CoordDate.latitude[1],s.c_str(),11);

        }

        if (counter == 5)
        {
            s = *iter;
            memcpy(&CoordDate.latitude[0],s.c_str(),1);
            s.clear();
        }

        if (counter == 6)
        {
            s.append(*iter);
            memcpy(CoordDate.longitude,s.c_str(),11);
            s.clear();
        }


        if (counter == 7)
        {
            s = *iter;
            memcpy(CoordDate.longitude,s.c_str(),1);
            s.clear();
        }

        if (counter == 10)
        {
            s = *iter;
            memcpy(CoordDate.data,s.c_str(),10);
            s.clear();
        }


    }
}

void Navigator::processSyncPulse() {
	syncPulse();
}
//#endif /* PORT__TARGET_DEVICE_REV1 */

} /* namespace Navigation */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(navigation, LevelDefault)
#include "qmdebug_domains_end.h"
