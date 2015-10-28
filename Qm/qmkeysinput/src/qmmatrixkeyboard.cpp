/**
  ******************************************************************************
  * @file    qmmatrixkeyboard.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  * Dummy
  * TODO: implement QmMatrixKeyboard class
  *
  ******************************************************************************
  */

#include "qmmatrixkeyboard.h"
#include "qmmatrixkeyboard_p.h"

QmMatrixKeyboard::QmMatrixKeyboard(int hw_resource, QmObject* parent) :
	QmObject(*new QmMatrixKeyboardPrivate(this), parent)
{
    QM_UNUSED(hw_resource);
}

QmMatrixKeyboard::~QmMatrixKeyboard() {
}

bool QmMatrixKeyboard::isKeyPressed(int id) {
	QM_UNUSED(id);
	return false;
}
