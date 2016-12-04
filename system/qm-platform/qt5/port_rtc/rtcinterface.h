#ifndef RTCINTERFACE_H
#define RTCINTERFACE_H

#include <QObject>
#include "rtc.h"
#include "port_hardwareio/i2cbus.h"

class RtcInterface : public QObject
{
    Q_OBJECT
public:
    static RTC* openInstance(int hw_resource);
    static void closeInstance(I2CBus *instance);

    void setTime(QTime time);
    QTime getTime();
    void setDate(QDate date);
    QDate getDate();

Q_SIGNALS:
    void wakeup();

private Q_SLOTS:
    void processWakeup()
    {

    }

private:
    friend class QmRtcPrivateAdapter;

    RtcInterface(QObject *parent);
    virtual ~RtcInterface();


};

#endif // RTCINTERFACE_H
