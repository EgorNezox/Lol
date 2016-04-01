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

#include "navigator.h"

namespace Navigation {

Navigator::Navigator(int uart_resource, int reset_iopin_resource, int ant_flag_iopin_resource) {
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

Coord_Date *Navigator::getCoordDate()
{
    return &CoordDate;
}

//#if defined(PORT__TARGET_DEVICE_REV1)
void Navigator::processUartReceivedData() {
	uint8_t data[1024];
	int64_t data_read = 0;
	while ((data_read = uart->readData(data, 1024 - 1))) {
		data[data_read] = '\0';
		qmDebugMessage(QmDebug::Dump, (char*)data);
	}


    //void (*ptr)(int data);
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
    char *res;

    res = strstr((const char *)data,search);

    if (strlen(res) > 0)
    {
        index = strlen((const char *)data) - strlen((const char *)res);
        int index_time = index + strlen("$GPRMC,");

        for(int i = 0;i<strlen("hhmmss.sss");i++)
            CoordDate.time[i] = data[index_time+i]; //получил время

        int index_lat  = index_time + strlen("hhmmss.sss,A,");
        int index_long = index_lat + strlen("GGMM.MM,P,");

        for(int i = 0;i<strlen("GGMM.MM,P");i++)
             CoordDate.latitude[i] = data[index_lat+i]; // получили широту

        for(int i = 0;i<strlen("gggmm.mm,J");i++)
            CoordDate.longitude[i] = data[index_long+i]; // получили долготу

        int index_date =  index_long + strlen(",v.v,b.b,");

        for(int i = 0; i <strlen("ddmmyy"); i++)
            CoordDate.data[i] = data[index_date + i]; // получили дату

        // CoordinateUpdated(); // вызов сигнала опроса структуры

    }
}
//#endif /* PORT__TARGET_DEVICE_REV1 */

} /* namespace Navigation */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(navigation, LevelDefault)
#include "qmdebug_domains_end.h"
