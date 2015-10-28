/**
  ******************************************************************************
  * @file    qmiopin_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMIOPIN_P_H_
#define QMIOPIN_P_H_

#include "qm.h"
#include "../../qmcore/src/qmobject_p.h"

class QmIopinPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmIopin)
public:
	QmIopinPrivate(QmIopin *q);
	virtual ~QmIopinPrivate();
};

#endif /* QMIOPIN_P_H_ */
