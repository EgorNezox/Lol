/**
  ******************************************************************************
  * @file    pushbuttonkeyinterface.h
  * @author  Petr Dmitriev
  * @date    05.12.2015
  *
  ******************************************************************************
  */

#ifndef PUSHBUTTONKEYINTERFACE_H
#define PUSHBUTTONKEYINTERFACE_H

#include <QObject>
#include <QMetaType>

class PushbuttonkeyInterface : public QObject
{
    Q_OBJECT

public:
    static PushbuttonkeyInterface* getInstance(int hw_resource);
    static PushbuttonkeyInterface* createInstance(int hw_resource);
    static void destroyInstance(PushbuttonkeyInterface *instance);

public Q_SLOTS:
    void setStateChanged();

#ifndef Q_MOC_RUN
private:
#else
Q_SIGNALS:
#endif
    void stateChanged();

private:
    friend class QmPushButtonKeyPrivateAdapter;

    PushbuttonkeyInterface();
    virtual ~PushbuttonkeyInterface();
};

#endif // PUSHBUTTONKEYINTERFACE_H
