/**
  ******************************************************************************
  * @file    qmmatrixkeyboard_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include <QDebug>
#include "qmmatrixkeyboard.h"
#include "qmmatrixkeyboard_p.h"

QmMatrixKeyboardPrivateAdapter::QmMatrixKeyboardPrivateAdapter(QmMatrixKeyboardPrivate *qmmatrixkeyboardprivate)
{
    interface = MatrixKeyboardInterface::getInstance(qmmatrixkeyboardprivate->hw_resource);
    QObject::connect(interface, &MatrixKeyboardInterface::keyStateChanged, this, &QmMatrixKeyboardPrivateAdapter::processKeyStateChanged);
}

QmMatrixKeyboardPrivateAdapter::~QmMatrixKeyboardPrivateAdapter()
{

}

void QmMatrixKeyboardPrivateAdapter::processKeyStateChanged(int id, bool state)
{
    qmmatrixkeyboardprivate->processKeyStateChanged(id, state);
}

QmMatrixKeyboardPrivate::QmMatrixKeyboardPrivate(QmMatrixKeyboard *q) :
    QmObjectPrivate(q),
    hw_resource(-1), matrixkb_adapter(0), keys_count(16)
{
}

QmMatrixKeyboardPrivate::~QmMatrixKeyboardPrivate()
{
}

void QmMatrixKeyboardPrivate::init()
{
    matrixkb_adapter = new QmMatrixKeyboardPrivateAdapter(this);
}

void QmMatrixKeyboardPrivate::deinit()
{
    delete matrixkb_adapter;
}

void QmMatrixKeyboardPrivate::processKeyStateChanged(int id, bool state)
{
    qDebug() << "QmMatrixKeyboardPrivate::processKeyStateChanged(" << id << ", " << state << ")";
}

bool QmMatrixKeyboard::event(QmEvent* event) {
	return QmObject::event(event);
}
