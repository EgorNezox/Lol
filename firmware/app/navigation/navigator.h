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
#include "../../app/datastorage/fs.h"


namespace Navigation {

struct Coord_Date
{
	uint8_t data[10];
	uint8_t time[10];
	uint8_t latitude[11];
	uint8_t longitude[12];
    bool   status;
};

class Navigator : public QmObject {
public:
	Navigator(int uart_resource, int reset_iopin_resource, int ant_flag_iopin_resource, int sync_pulse_iopin_resource);
	~Navigator();

	void setFlash(DataStorage::FS *flash);

	sigc::signal<void> CoordinateUpdated;
	sigc::signal<void> syncPulse;
	sigc::signal<void,bool> PswfSignal;
	Coord_Date getCoordDate();
	void setMinimalActivityMode(bool enabled);
	int setGeneratorAbsValue(int val);
	int getGeneratorDacValue();
	void set1PPSModeCorrect(bool value);
	bool get1PPSModeCorrect();
	void coldStart();
	void setGPSTuneFlag(bool isGen);

	void initCorrector();
	int tuneGen(int val);

	int Calc_LCODE(int R_ADR, int S_ADR, int COM_N, int RN_KEY, int DAY, int HRS, int MIN,int SEC);
	int Calc_LCODE_RETR(int RP_ADR,int R_ADR, int COM_N, int RN_KEY, int DAY, int HRS, int MIN, int SEC);
	int Calc_LCODE_SMS(int R_ADR, int S_ADR, int WZN, int RN_KEY, int DAY, int HRS, int MIN,int SEC);
	int Calc_LCODE_SMS_call(int R_ADR, int S_ADR, int CYC_N, int RN_KEY, int DAY, int HRS, int MIN,int SEC);
	bool isZda = false;

	int resGenDac = 0;
private:
    DataStorage::FS *flash = 0;

    void processConfig();
	void processUartReceivedData();
	void processUartReceivedErrors(bool data_errors, bool overflow);
	void parsingData(uint8_t data[]);
	void processSyncPulse(bool overflow);
	void redactCoordForSpec(uint8_t *input, int val);

	Coord_Date CoordDate;

	QmIopin *reset_iopin;
	QmIopin *ant_flag_iopin;
	QmIopin *sync_pulse_iopin;
	QmUart *uart;
	QmTimer *config_timer;

	bool minimal_activity_mode;
	bool pps_correct;
	bool isTune;
};

} /* namespace Navigation */

#endif /* FIRMWARE_APP_NAVIGATION_NAVIGATOR_H_ */
