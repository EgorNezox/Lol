#include "rtcinterface.h"
#include "hardware_resources.h"

RtcInterface::RtcInterface(QObject *parent) : QObject(parent)
{

}

RtcInterface::~RtcInterface()
{

}

/*RtcInterface *RtcInterface::getInstance(int hw_resource) {
    RtcInterface *instance = qobject_cast<RtcInterface *>(QtHwEmu::getResourceInterface(hw_resource));
    Q_ASSERT(instance);
    return instance;
}

RtcInterface *RtcInterface::createInstance(int hw_resource) {
    RtcInterface *instance = new RtcInterface();
    QtHwEmu::acquireResource(hw_resource, instance);
    return instance;
}

void RtcInterface::destroyInstance(RtcInterface *instance) {
    QtHwEmu::releaseResource(instance);
    delete instance;
}

void RtcInterface::setTime(QTime time)
{

}

QTime RtcInterface::getTime()
{

}

void RtcInterface::setDate(QDate date)
{

}

QDate RtcInterface::getDate()
{

}
*/
