/**
  ******************************************************************************
  * @file    qmmatrixkeyboard_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "qmmatrixkeyboard.h"
#include "qmmatrixkeyboard_p.h"

QmMatrixKeyboardPrivate::QmMatrixKeyboardPrivate(QmMatrixKeyboard *q) :
	QmObjectPrivate(q)
{
}

QmMatrixKeyboardPrivate::~QmMatrixKeyboardPrivate()
{
}

void QmMatrixKeyboardPrivate::init()
{
}

void QmMatrixKeyboardPrivate::deinit()
{
}

bool QmMatrixKeyboard::event(QmEvent* event) {
	return QmObject::event(event);
}
