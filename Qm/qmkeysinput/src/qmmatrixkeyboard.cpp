/**
  ******************************************************************************
  * @file    qmmatrixkeyboard.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  * Dummy
  *
  ******************************************************************************
  */

#include "qmmatrixkeyboard.h"
#include "qmmatrixkeyboard_p.h"

QmMatrixKeyboard::QmMatrixKeyboard(int hw_resource, QmObject* parent) :
	QmObject(*new QmMatrixKeyboardPrivate(this), parent)
{
	QM_D(QmMatrixKeyboard);
	d->hw_resource = hw_resource;
	d->init();
}

QmMatrixKeyboard::~QmMatrixKeyboard() {
	QM_D(QmMatrixKeyboard);
	d->deinit();
}

bool QmMatrixKeyboard::isKeyPressed(int id) {
	QM_D(QmMatrixKeyboard);
	return d->isKeyPressed(id);
}
