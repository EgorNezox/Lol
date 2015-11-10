/**
  ******************************************************************************
  * @file    qmpushbuttonkey_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMPUSHBUTTONKEY_P_H_
#define QMPUSHBUTTONKEY_P_H_

#include "../../qmcore/src/qmobject_p.h"

class QmPushButtonKeyPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmPushButtonKey)
public:
	QmPushButtonKeyPrivate(QmPushButtonKey *q);
	virtual ~QmPushButtonKeyPrivate();
};

#endif /* QMPUSHBUTTONKEY_P_H_ */
