/**
  ******************************************************************************
  * @file    qmrtc_qt.cpp
  * @author  Petr Dmitriev
  * @date    23.11.2016
  *
  ******************************************************************************
 */

#include "qmrtc_p.h"


QmRtcPrivateAdapter::QmRtcPrivateAdapter(QmRtcPrivate *qmrtcprivate) :
    qmrtcprivate(qmrtcprivate)
{
    interface = RtcInterface::getInstance(qmrtcprivate->hw_resource);
    QObject::connect(interface, &RtcInterface::wakeup, this, &QmRtcPrivateAdapter::processWakeup);
//    QObject::connect(this, &QmRtcPrivateAdapter::writeOutputLevel, interface, &RtcInterface::assignOutputLevel);
}

QmRtcPrivateAdapter::~QmRtcPrivateAdapter()
{
}

void QmRtcPrivateAdapter::processWakeup()
{
    QmRtc * const q = qmrtcprivate->q_func();
    q->wakeup.emit();
}

QmRtcPrivate::QmRtcPrivate(QmRtc *q) :
    QmObjectPrivate(q),
    hw_resource(-1), rtc_adapter(0)
{
}

QmRtcPrivate::~QmRtcPrivate() {

}

void QmRtcPrivate::init() {
    rtc_adapter = new QmRtcPrivateAdapter(this);
}

void QmRtcPrivate::deinit() {
    delete rtc_adapter;
}

void QmRtc::setTime(Time& time) {
//    hal_rtc_time_t hal_time;
//    hal_time.hours = time.hours;
//    hal_time.minutes = time.minutes;
//    hal_time.seconds = time.seconds;
//    hal_rtc_set_time(hal_time);
}

void QmRtc::setDate(Date& date) {
//    hal_rtc_date_t hal_date;
//    hal_date.weekday = date.weekday;
//    hal_date.day = date.day;
//    hal_date.month = date.month;
//    hal_date.year = date.year;
//    hal_rtc_set_date(hal_date);
}

QmRtc::Time QmRtc::getTime() {
//    hal_rtc_time_t hal_time = hal_rtc_get_time();
//    Time time;
//    time.hours = hal_time.hours;
//    time.minutes = hal_time.minutes;
//    time.seconds = hal_time.seconds;
//    return time;
}

QmRtc::Date QmRtc::getDate() {
//    hal_rtc_date_t hal_date = hal_rtc_get_date();
//    Date date;
//    date.weekday = hal_date.weekday;
//    date.day = hal_date.day;
//    date.month = hal_date.month;
//    date.year = hal_date.year;
//    return date;
}
