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

/*! The QmI2CDevice class provides functions to access I2C bus devices(slaves).
 */
class QmI2CDevice: public QmObject {
public:
	/*! Constructs an i2c device with the given \a parent.
	 *
	 * Parameter \a hw_resource specifies platform-identified instance of device.
	 */
	QmI2CDevice(int hw_resource, QmObject *parent = 0);

	/*! Destroys the i2c device. */
	virtual ~QmI2CDevice();

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmI2CDevice)
	QM_DISABLE_COPY(QmI2CDevice)
};

#endif /* QMI2CDEVICE_H_ */
