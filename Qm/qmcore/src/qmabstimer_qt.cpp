/**
  ******************************************************************************
  * @file    qmabstimer_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
  */

#include <qtimer.h>

#include "qmobject_p.h"
#include "qmabstimer.h"
#include "qmabstimer_p.h"
#include "qmtimestamp.h"
#include "qmtimestamp_p.h"

class QmAbsTimerPrivateAdapter : public QTimer
{
public:
	QmAbsTimerPrivateAdapter(QmAbsTimerPrivate *qmabstimerprivate) :
		qmabstimerprivate(qmabstimerprivate)
	{
		QObject::connect(this, &QmAbsTimerPrivateAdapter::timeout, this, &QmAbsTimerPrivateAdapter::processTimeout);
	}
	~QmAbsTimerPrivateAdapter() {}
	QmAbsTimerPrivate *qmabstimerprivate;
public Q_SLOTS:
	void processTimeout() {
		QmAbsTimer * const q = qmabstimerprivate->q_func();
		qmabstimerprivate->is_active = false;
		q->timeout.emit();
	}
};

QmAbsTimerPrivate::QmAbsTimerPrivate(QmAbsTimer *q) :
	QmObjectPrivate(q),
	is_active(false), qt_adapter(new QmAbsTimerPrivateAdapter(this))
{
}

QmAbsTimerPrivate::~QmAbsTimerPrivate()
{
	delete qt_adapter;
}

void QmAbsTimerPrivate::init() {
	qt_adapter->setSingleShot(true);
}

void QmAbsTimerPrivate::deinit()
{
}

bool QmAbsTimer::start(const QmTimestamp &timestamp, unsigned int msec) {
	QM_D(QmAbsTimer);
	if (!timestamp.impl->isValid())
		return false;
	if (isActive())
		stop();
	d->is_active = true;
	d->qt_adapter->start(qMax((qint64)0, (qint64)msec - timestamp.impl->getValue()));
	return true;
}

void QmAbsTimer::stop() {
	QM_D(QmAbsTimer);
	d->qt_adapter->stop();
	d->is_active = false;
}

bool QmAbsTimer::event(QmEvent* event) {
	return QmObject::event(event);
}
