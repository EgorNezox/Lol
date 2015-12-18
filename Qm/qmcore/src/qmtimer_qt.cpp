/**
  ******************************************************************************
  * @file    qmtimer_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include <QTimer>

#include "qmobject_p.h"
#include "qmtimer.h"
#include "qmtimer_p.h"

class QmTimerPrivateAdapter : public QTimer
{
public:
	QmTimerPrivateAdapter(QmTimerPrivate *qmtimerprivate) :
		qmtimerprivate(qmtimerprivate)
	{
		QObject::connect(this, &QmTimerPrivateAdapter::timeout, this, &QmTimerPrivateAdapter::processTimeout);
	}
	~QmTimerPrivateAdapter() {}
	QmTimerPrivate *qmtimerprivate;
public Q_SLOTS:
	void processTimeout() {
		QmTimer * const q = qmtimerprivate->q_func();
		qmtimerprivate->is_active = !isSingleShot();
		q->timeout.emit();
	}
};

QmTimerPrivate::QmTimerPrivate(QmTimer *q) :
	QmObjectPrivate(q),
	is_active(false), interval_value(0), qt_adapter(new QmTimerPrivateAdapter(this))
{
}

QmTimerPrivate::~QmTimerPrivate()
{
	delete qt_adapter;
}

void QmTimerPrivate::init(bool single_shot) {
	qt_adapter->setSingleShot(single_shot);
}

void QmTimerPrivate::deinit()
{
}

void QmTimer::start() {
	QM_D(QmTimer);
	if (isActive())
		stop();
	d->is_active = true;
	d->qt_adapter->start(d->interval_value);
}

void QmTimer::stop() {
	QM_D(QmTimer);
	d->qt_adapter->stop();
	d->is_active = false;
}

bool QmTimer::event(QmEvent* event) {
	return QmObject::event(event);
}
