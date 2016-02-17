/**
  ******************************************************************************
  * @file    pushbuttonkeyinterface.cpp
  * @author  Petr Dmitriev
  * @date    05.12.2015
  *
  ******************************************************************************
  */

#include <qdebug.h>
#include "pushbuttonkeyinterface.h"
#include "hardware_emulation.h"

PushbuttonkeyInterface::PushbuttonkeyInterface()
{
}

PushbuttonkeyInterface::~PushbuttonkeyInterface()
{
}

PushbuttonkeyInterface *PushbuttonkeyInterface::getInstance(int hw_resource) {
    PushbuttonkeyInterface *instance = qobject_cast<PushbuttonkeyInterface *>(QtHwEmu::getResourceInterface(hw_resource));
    Q_ASSERT(instance);
    return instance;
}

PushbuttonkeyInterface *PushbuttonkeyInterface::createInstance(int hw_resource) {
    PushbuttonkeyInterface *instance = new PushbuttonkeyInterface();
    QtHwEmu::acquireResource(hw_resource, instance);
    return instance;
}

void PushbuttonkeyInterface::destroyInstance(PushbuttonkeyInterface *instance)
{
    QtHwEmu::releaseResource(instance);
    delete instance;
}

void PushbuttonkeyInterface::setStateChanged()
{
    Q_EMIT stateChanged();
}
