/**
  ******************************************************************************
  * @file    matrixkeyboardinterface.cpp
  * @author  Petr Dmitriev
  * @date    18.12.2015
  *
  ******************************************************************************
  */

#include <qdebug.h>
#include "matrixkeyboardinterface.h"
#include "hardware_resources.h"

MatrixKeyboardInterface::MatrixKeyboardInterface(int keysNumber) :
    keys_number(keysNumber)
{
    Q_ASSERT(keysNumber > 0);
}

MatrixKeyboardInterface::~MatrixKeyboardInterface()
{
}

MatrixKeyboardInterface *MatrixKeyboardInterface::getInstance(int hw_resource) {
    MatrixKeyboardInterface *instance = qobject_cast<MatrixKeyboardInterface *>(QtHwEmu::getResourceInterface(hw_resource));
    Q_ASSERT(instance);
    return instance;
}

MatrixKeyboardInterface *MatrixKeyboardInterface::createInstance(int hw_resource, int keysNumber) {
    MatrixKeyboardInterface *instance = new MatrixKeyboardInterface(keysNumber);
    QtHwEmu::acquireResource(hw_resource, instance);
    return instance;
}

void MatrixKeyboardInterface::destroyInstance(MatrixKeyboardInterface *instance) {
    QtHwEmu::releaseResource(instance);
    delete instance;
}

int MatrixKeyboardInterface::keysNumber() {
    return keys_number;
}

void MatrixKeyboardInterface::setKeyStateChanged(int id, bool state) {
    Q_EMIT keyStateChanged(id, state);
}

