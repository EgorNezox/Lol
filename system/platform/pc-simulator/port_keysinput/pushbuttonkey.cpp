/**
  ******************************************************************************
  * @file    pushbuttonkey.cpp
  * @author  Petr Dmitriev
  * @date    05.12.2015
  *
  ******************************************************************************
  */

#include <qdebug.h>
#include "pushbuttonkey.h"
#include "hardware_emulation.h"

PushButtonKey::PushButtonKey(QWidget *parent, int hw_resource) :
    QPushButton(parent), pbkey_interface(0)
{
    if (hw_resource != -1)
        assignHwResource(hw_resource);
}

PushButtonKey::~PushButtonKey() {
    if (pbkey_interface)
        PushbuttonkeyInterface::destroyInstance(pbkey_interface);
}

QString PushButtonKey::getHwResource() {
    return QString();
}

void PushButtonKey::assignHwResource(const QString &value) {
    assignHwResource(QtHwEmu::convertToPlatformHwResource(value));
}

void PushButtonKey::assignHwResource(int value) {
    Q_ASSERT(pbkey_interface == 0);
    pbkey_interface = PushbuttonkeyInterface::createInstance(value);
    QObject::connect(this, &QAbstractButton::pressed, this, &PushButtonKey::processPBStateChanged);
    QObject::connect(this, &QAbstractButton::released, this, &PushButtonKey::processPBStateChanged);
}

void PushButtonKey::processPBStateChanged()
{
    pbkey_interface->setStateChanged();
}
