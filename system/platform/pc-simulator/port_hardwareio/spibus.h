/**
 ******************************************************************************
 * @file    spibus.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    12.02.2016
 *
 ******************************************************************************
 */

#ifndef SPIBUS_H_
#define SPIBUS_H_

#include <qobject.h>
#include <qmap.h>

class SPIDeviceInterface;

class SPIBus: public QObject {
	Q_OBJECT

public:
	static SPIBus* openInstance(int bus_hw_resource);
	static void closeInstance(SPIBus *instance);

private:
	friend class QmSPIBus;
	friend class QmSPIDevicePrivateAdapter;
	friend class SPIDeviceInterface;

	SPIBus();
	~SPIBus();
	static SPIBus* getInstance(int bus_hw_resource);
	void registerSlave(int cs_hw_resource, SPIDeviceInterface *instance);
	void unregisterSlave(SPIDeviceInterface *instance);

	bool enabled;
	QMultiMap<int, SPIDeviceInterface*> slaves;

private Q_SLOTS:
	void transferFullDuplex8bit(int cs_hw_resource, quint8 *rx_data, quint8 *tx_data, int count);
	void transferFullDuplex16bit(int cs_hw_resource, quint16 *rx_data, quint16 *tx_data, int count);
};

#endif /* SPIBUS_H_ */
