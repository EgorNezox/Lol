/**
  ******************************************************************************
  * @file    qmi2cdevice.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.11.2015
  *
  ******************************************************************************
  */

#ifndef QMI2CDEVICE_H_
#define QMI2CDEVICE_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmI2CDevice)

/*! The QmI2CDevice class provides functions to access I2C/SMBus bus devices(slaves) with fixed address.
 */
class QmI2CDevice: public QmObject {
public:
	enum TransferResult {
		transferSuccess,			/*!< успешная передача */
		transferErrorAborted,		/*!< прервано (например, при истечении таймаута) */
		transferErrorPEC,			/*!< несовпадение контрольной суммы PEC при Rx */
		transferErrorBus,			/*!< ошибка во время коммуникации на шине (misplaced START/STOP, arbitration lost) */
		transferErrorAddressNACK,	/*!< устройство не ответило на адресацию */
		transferErrorDataNACK		/*!< устройство не подтвердило байт данных (при Tx) */
	};

	/*! Constructs a device with the given \a parent.
	 *
	 * Parameter \a bus_hw_resource specifies platform-identified instance of a bus device connected to.
	 * Parameter \a address specifies 7-bit slave address of device (default = "SMBus Device Default Address").
	 */
	QmI2CDevice(int bus_hw_resource, uint8_t address = 0x61, QmObject *parent = 0);

	/*! Destroys the device. */
	virtual ~QmI2CDevice();

	void setTransferTimeout(int msec);

	bool startEmptyTransfer();

	bool startRxTransfer(bool use_pec, uint32_t size);

	bool startTxTransfer(bool use_pec, uint8_t *data, uint32_t size);

	bool startTxRxTransfer(bool use_pec, uint8_t *tx_data, uint32_t tx_size, uint32_t rx_size);

	int64_t readRxData(uint8_t *buffer, uint32_t size, int offset = 0);

	sigc::signal<void, TransferResult/*result*/> transferCompleted;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmI2CDevice)
	QM_DISABLE_COPY(QmI2CDevice)
};

#endif /* QMI2CDEVICE_H_ */
