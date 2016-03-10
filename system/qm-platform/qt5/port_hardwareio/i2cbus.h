/**
 ******************************************************************************
 * @file    i2cbus.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    26.12.2015
 *
 ******************************************************************************
 */

#ifndef I2CBUS_H_
#define I2CBUS_H_

#include <qobject.h>
#include <qbytearray.h>
#include <qmap.h>

class I2CDeviceInterface;

class I2CBus: public QObject {
	Q_OBJECT

public:
	static I2CBus* openInstance(int hw_resource);
	static void closeInstance(I2CBus *instance);

private:
	friend class QmI2CDevicePrivateAdapter;
	friend class QmSMBusHostPrivateAdapter;
	friend class I2CDeviceInterface;

	I2CBus();
	~I2CBus();
	static I2CBus* getInstance(int hw_resource);
	void registerSlave(uint8_t address, I2CDeviceInterface *instance);
	void unregisterSlave(I2CDeviceInterface *instance);
	void responseTransferDelayed(uint8_t address, bool ack, bool pec_present, const QByteArray &rx_data);
	void responseTransferDelayed(I2CDeviceInterface *slave, bool pec_present, const QByteArray &rx_data);
	void processHostNotify(I2CDeviceInterface *slave, uint16_t status);

	QMap<uint8_t, I2CDeviceInterface*> slaves;

#ifndef Q_MOC_RUN
private:
#else
Q_SIGNALS:
#endif
	void transferResponse(uint8_t address, bool ack, bool pec_present, const QByteArray &rx_data);
	void messageHostNotify(uint8_t address, uint16_t status);

private Q_SLOTS:
	void requestTransfer(uint8_t address, bool use_pec, const QByteArray &tx_data, uint32_t rx_size);
};

#endif /* I2CBUS_H_ */
