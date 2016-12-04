#include <QTime>
#include <QDate>
#include <QTimer>
#include "rtc.h"

RTC::RTC(QObject *parent) : QObject(parent)
{
    time = new QTime(QTime::currentTime());
    date = new QDate(QDate::currentDate());
    timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, this, &RTC::processWakeup);
    timer->start(1000);
}

RTC::~RTC()
{
    delete time;
    delete date;
}

void RTC::setTime(QTime *time)
{
    this->time = time;
}

QTime RTC::getTime()
{
    return *time;
}

void RTC::setDate(QDate *date)
{
    this->date = date;
}

QDate RTC::getDate()
{
    return *date;
}

void RTC::processWakeup()
{
    Q_EMIT wakeup();
}
