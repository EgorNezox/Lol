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

#include "system_hw_io.h"
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

int QmMatrixKeyboard::keysNumber(int hw_resource) {
	int column_count = 0;
	int row_count = 0;
	stm32f2_get_matrixkeyboard_pins(hw_resource, NULL, &column_count, NULL, &row_count);
	return column_count * row_count;
}

bool QmMatrixKeyboard::isKeyPressed(int id) {
	QM_D(QmMatrixKeyboard);
	return d->isKeyPressed(id);
}
