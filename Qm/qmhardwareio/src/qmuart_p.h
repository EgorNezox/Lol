/**
  ******************************************************************************
  * @file    qmuart_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  *
  ******************************************************************************
  */

#ifndef QMUART_P_H_
#define QMUART_P_H_

#include "../../qmcore/src/qmobject_p.h"

class QmUartPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmUart)
public:
	QmUartPrivate(QmUart *q);
	virtual ~QmUartPrivate();
};

#endif /* QMUART_P_H_ */
