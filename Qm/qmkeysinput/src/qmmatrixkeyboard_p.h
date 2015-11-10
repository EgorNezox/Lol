/**
  ******************************************************************************
  * @file    qmmatrixkeyboard_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMMATRIXKEYBOARD_P_H_
#define QMMATRIXKEYBOARD_P_H_

#include "../../qmcore/src/qmobject_p.h"

class QmMatrixKeyboardPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmMatrixKeyboard)
public:
	QmMatrixKeyboardPrivate(QmMatrixKeyboard *q);
	virtual ~QmMatrixKeyboardPrivate();
};

#endif /* QMMATRIXKEYBOARD_P_H_ */
