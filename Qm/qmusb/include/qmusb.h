/**
  ******************************************************************************
  * @file    qmrtc.h
  * @author  Petr Dmitriev
  * @date    23.11.2016
  *
  ******************************************************************************
 */

#ifndef QM_QMUSB_INCLUDE_QMUSB_H_
#define QM_QMUSB_INCLUDE_QMUSB_H_

#include <stdint.h>
#include "qmobject.h"

QM_FORWARD_PRIVATE(QmUsb)

class QmUsb: public QmObject {
public:
	QmUsb(int hw_resource, QmObject *parent = 0);
	virtual ~QmUsb();

	uint8_t* getbuffer();
	bool 	 getrtc();
	bool     getdtr();
	uint16_t getLen();
	void resetLen();
	bool getStatus();

	int countPrev = 0;

	sigc::signal<void> usbwakeup;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmUsb)
	QM_DISABLE_COPY(QmUsb)
};



#endif /* QM_QMRTC_INCLUDE_QMRTC_H_ */
