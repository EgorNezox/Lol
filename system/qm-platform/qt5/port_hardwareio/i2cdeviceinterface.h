/**
 ******************************************************************************
 * @file    i2cdeviceinterface.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    26.12.2015
 *
 ******************************************************************************
 */

#ifndef I2CDEVICEINTERFACE_H_
#define I2CDEVICEINTERFACE_H_

#include <qobject.h>

class I2CBus;

class I2CDeviceInterface: public QObject {
	Q_OBJECT

public:
	enum State {
		stateIdle,
		statePendingEmpty,
		statePendingRx,
		statePendingTx,
		statePendingTxRx
	};

	I2CDeviceInterface(int bus_hw_resource, uint8_t address);
	~I2CDeviceInterface();

	State readTransfer(bool &use_pec, QByteArray &tx_data, uint32_t &rx_size);
	void writeRx(bool use_pec, const QByteArray &data);

Q_SIGNALS:
	void transferPending();

public Q_SLOTS:
	void setAddressAck(bool enabled);
	void notifyHost(uint16_t status);

private:
	friend class I2CBus;

	I2CBus *bus;
	bool address_ack;
	State state;
	bool transfer_uses_pec;
	QByteArray transfer_tx_data;
	uint32_t transfer_rx_size;

	bool processTransfer(bool use_pec, const QByteArray &tx_data, uint32_t rx_size);
};

#endif /* I2CDEVICEINTERFACE_H_ */
