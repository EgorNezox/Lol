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
//#endif /* PORT__TARGET_DEVICE_REV1 */

namespace Navigation {

class Navigator : public QmObject {
public:
	Navigator(int uart_resource, int reset_iopin_resource, int ant_flag_iopin_resource);
	~Navigator();

private:
//#if defined(PORT__TARGET_DEVICE_REV1)
	void processUartReceivedData();
	void processUartReceivedErrors(bool data_errors, bool overflow);
    void parsingData(uint8_t data[], uint8_t len);


	QmIopin *reset_iopin;
	QmIopin *ant_flag_iopin;
	QmUart *uart;
//#endif /* PORT__TARGET_DEVICE_REV1 */
};

} /* namespace Navigation */

#endif /* FIRMWARE_APP_NAVIGATION_NAVIGATOR_H_ */
