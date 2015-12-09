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

class CoreTimer : public QTimer
{
public:
	CoreTimer(QmTimerPrivate *qmprivatetimer) :
		qmprivatetimer(qmprivatetimer)
	{
		QObject::connect(this, &CoreTimer::timeout, this, &CoreTimer::processTimeout);
	}
	~CoreTimer() {}
	QmTimerPrivate *qmprivatetimer;
public Q_SLOTS:
	void processTimeout() {
		qmprivatetimer->processTimeout();
	}
};

QmTimerPrivate::QmTimerPrivate(QmTimer *q) :
	QmObjectPrivate(q),
	qtimer(new CoreTimer(this))
{
}

QmTimerPrivate::~QmTimerPrivate()
{
	delete qtimer;
}

void QmTimerPrivate::init(bool single_shot) {
	qtimer->setSingleShot(single_shot);
}

void QmTimerPrivate::deinit()
{
}

void QmTimerPrivate::processTimeout() {
	QM_Q(QmTimer);
	q->is_active = !qtimer->isSingleShot();
	q->timeout.emit();
}

void QmTimer::start() {
	QM_D(QmTimer);
	if (isActive())
		stop();
	is_active = true;
	d->qtimer->start(interval_value);
}

void QmTimer::stop() {
	QM_D(QmTimer);
	d->qtimer->stop();
	is_active = false;
}

void QmTimer::setSingleShot(bool enable) {
	QM_D(QmTimer);
	if (d->qtimer->isSingleShot() == enable)
		return;
	stop();
	d->qtimer->setSingleShot(enable);
}

bool QmTimer::event(QmEvent* event) {
	return QmObject::event(event);
}
