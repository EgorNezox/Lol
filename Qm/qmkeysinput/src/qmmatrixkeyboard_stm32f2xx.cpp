/**
  ******************************************************************************
  * @file    qmmatrixkeyboard_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include "qmmatrixkeyboard.h"
#include "qmmatrixkeyboard_p.h"
#include "qmevent.h"

QmMatrixKeyboardPrivate::QmMatrixKeyboardPrivate(QmMatrixKeyboard *q) :
	QmObjectPrivate(q)
{
}

QmMatrixKeyboardPrivate::~QmMatrixKeyboardPrivate()
{
}

bool QmMatrixKeyboard::event(QmEvent* event) {
	if (event->type() == QmEvent::None) {
		//TODO: ...
		return true;
	}
	return QmObject::event(event);
}
