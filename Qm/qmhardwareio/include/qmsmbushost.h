/**
  ******************************************************************************
  * @file    qmsmbushost.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    08.12.2015
  *
  ******************************************************************************
  */

#ifndef QMSMBUSHOST_H_
#define QMSMBUSHOST_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmSMBusHost)

/*! The QmSMBusHost class provides functions to control SMBus host operation.
 */
class QmSMBusHost: public QmObject {
public:
	/*! Constructs an SMBus host with the given \a parent.
	 *
	 * Parameter \a hw_resource specifies platform-identified instance of I2C bus.
	 */
	QmSMBusHost(int hw_resource, QmObject *parent = 0);

	/*! Destroys the SMBus host. */
	virtual ~QmSMBusHost();

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmSMBusHost)
	QM_DISABLE_COPY(QmSMBusHost)
};

#endif /* QMSMBUSHOST_H_ */
