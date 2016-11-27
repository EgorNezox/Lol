#ifndef RTC_H
#define RTC_H

#include <QObject>

class QTime;
class QDate;
class QTimer;
class RtcInterface;

class RTC : public QObject
{
    Q_OBJECT
public:
    static RTC* openInstance(int hw_resource);
    static void closeInstance(RTC *instance);

    void setTime(QTime time);
    QTime getTime();
    void setDate(QDate date);
    QDate getDate();

Q_SIGNALS:
    void wakeup();

private Q_SLOTS:
    void processWakeup();

private:
    friend class QmRtcPrivateAdapter;

    RTC(QObject *parent = 0);
    ~RTC();
    static I2CBus* getInstance(int hw_resource);

    QTime* time;
    QDate* date;
    QTimer* timer;
    RtcInterface* interface;
};

#endif // RTC_H
