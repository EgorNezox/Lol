/**
  ******************************************************************************
  * @file    pushbuttonkey.h
  * @author  Petr Dmitriev
  * @date    05.12.2015
  *
  ******************************************************************************
  */

#ifndef PUSHBUTTONKEY_H
#define PUSHBUTTONKEY_H

#include <QPushButton>
#include "pushbuttonkeyinterface.h"

class PushButtonKey : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(QString hw_resource READ getHwResource WRITE assignHwResource)

public:
    PushButtonKey(QWidget *parent = 0, int hw_resource = -1);
    ~PushButtonKey();

    QString getHwResource();
    void assignHwResource(const QString &value);
    void assignHwResource(int hw_resource);

private Q_SLOTS:
    void processPBStateChanged();

private:
    PushbuttonkeyInterface* pbkey_interface;
};

#endif // PUSHBUTTONKEY_H
