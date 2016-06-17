/*
 * navigator.h
 *
 *  Created on: 13 янв. 2016 г.
 *      Author: usrpetr
 */

#ifndef FIRMWARE_APP_NAVIGATION_NAVIGATOR_H_
#define FIRMWARE_APP_NAVIGATION_NAVIGATOR_H_

//#if defined(PORT__TARGET_DEVICE_REV1)
class QmIopin;
class QmUart;
class QmTimer;
//#endif /* PORT__TARGET_DEVICE_REV1 */

#include <thread>
#include <chrono>

namespace Navigation {


struct Coord_Date
{
    uint8_t data[10];
    uint8_t time[10];
    uint8_t latitude[11];
    uint8_t longitude[12];
};

class Navigator : public QmObject {
public:
	Navigator(int uart_resource, int reset_iopin_resource, int ant_flag_iopin_resource, int sync_pulse_iopin_resource);
	~Navigator();

     sigc::signal<void> CoordinateUpdated; // обновили дату,время и координаты
     sigc::signal<void> syncPulse;
     Coord_Date getCoordDate();

     int Calc_LCODE(int R_ADR, int S_ADR, int COM_N, int RN_KEY, int DAY, int HRS, int MIN,int SEC);
     int Calc_LCODE_RETR(int RP_ADR,int R_ADR, int S_ADR, int COM_N, int RN_KEY, int DAY, int HRS, int MIN, int SEC);
     int Calc_LCODE_SMS(int R_ADR, int S_ADR, int WZN, int RN_KEY, int DAY, int HRS, int MIN,int SEC);
     int Calc_LCODE_SMS_call(int R_ADR, int S_ADR, int CYC_N, int RN_KEY, int DAY, int HRS, int MIN,int SEC);

private:
//#if defined(PORT__TARGET_DEVICE_REV1)
 	void processConfig();
	void processUartReceivedData();
	void processUartReceivedErrors(bool data_errors, bool overflow);
    void parsingData(uint8_t data[]);
    void processSyncPulse(bool overflow);


    Coord_Date CoordDate;

	QmIopin *reset_iopin;
	QmIopin *ant_flag_iopin;
	QmIopin *sync_pulse_iopin;
	QmUart *uart;
	QmTimer *config_timer;
//#endif /* PORT__TARGET_DEVICE_REV1 */
};

} /* namespace Navigation */

#endif /* FIRMWARE_APP_NAVIGATION_NAVIGATOR_H_ */
