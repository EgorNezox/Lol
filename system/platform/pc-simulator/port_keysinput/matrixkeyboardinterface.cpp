/**
  ******************************************************************************
  * @file    matrixkeyboardinterface.cpp
  * @author  Petr Dmitriev
  * @date    18.12.2015
  *
  ******************************************************************************
  */

#include <QDebug>
#include "matrixkeyboardinterface.h"
#include "hardware_emulation.h"

MatrixKeyboardInterface::MatrixKeyboardInterface()
{
}

MatrixKeyboardInterface::~MatrixKeyboardInterface()
{
}

MatrixKeyboardInterface *MatrixKeyboardInterface::getInstance(int hw_resource) {
    MatrixKeyboardInterface *instance = qobject_cast<MatrixKeyboardInterface *>(QtHwEmu::getResourceInterface(hw_resource));
    Q_ASSERT(instance);
    return instance;
}

MatrixKeyboardInterface *MatrixKeyboardInterface::createInstance(int hw_resource) {
    MatrixKeyboardInterface *instance = new MatrixKeyboardInterface();
    QtHwEmu::acquireResource(hw_resource, instance);
    return instance;
}

void MatrixKeyboardInterface::destroyInstance(MatrixKeyboardInterface *instance) {
    QtHwEmu::releaseResource(instance);
    delete instance;
}

void MatrixKeyboardInterface::setKeyStateChanged(int id, bool state) {
    qDebug() << "MatrixKeyboardInterface::setKeyStateChanged(" << id << ", " << state << ")";
    Q_EMIT keyStateChanged(id, state);
}

