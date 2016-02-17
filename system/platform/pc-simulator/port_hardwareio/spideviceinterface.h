/**
 ******************************************************************************
 * @file    spideviceinterface.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    12.02.2016
 *
 ******************************************************************************
 */

#ifndef SPIDEVICEINTERFACE_H_
#define SPIDEVICEINTERFACE_H_

#include <qobject.h>

class SPIBus;

class SPIDeviceInterface: public QObject {
	Q_OBJECT

public:
	SPIDeviceInterface(int bus_hw_resource, int cs_hw_resource = -1);
	~SPIDeviceInterface();

Q_SIGNALS:
	void transferFullDuplex8bit(quint8 *rx_data, quint8 *tx_data, int count);
	void transferFullDuplex16bit(quint16 *rx_data, quint16 *tx_data, int count);

private:
	friend class SPIBus;

	SPIBus *bus;
};

#endif /* SPIDEVICEINTERFACE_H_ */
