/**
  ******************************************************************************
  * @file    qmthread_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include <QMap>
#include <QMutex>
#include <QThread>

#define QMDEBUGDOMAIN QmCore
#include "qm.h"
#include "qmdebug.h"
#include "qmevent.h"
#include "qmobject_p.h"
#include "qmthread.h"
#include "qmthread_p.h"
#include "qmapplication.h"

class QmThreadPrivateAdapter : public QThread {
	Q_OBJECT
public:
	QmThreadPrivateAdapter(QmThreadPrivate *qmthreadprivate) :
		qmthreadprivate(qmthreadprivate)
	{
		QObject::connect(this, &QmThreadPrivateAdapter::finished, this, &QmThreadPrivateAdapter::processFinish);
	}
	~QmThreadPrivateAdapter() {}
	QmThreadPrivate *qmthreadprivate;
	void run() {
		QmThread * const q = qmthreadprivate->q_func();
		q->run();
	}
	int exec() {
		return QThread::exec();
	}
public Q_SLOTS:
	void processFinish() {
		QmThread * const q = qmthreadprivate->q_func();
		qmthreadprivate->running = false;
		q->finished.emit();
	}
};

static QMutex registered_threads_mutex;
static QMap<QThread*,QmThread*> registered_threads;
static bool main_thread_registered = false;

QmThreadPrivate::QmThreadPrivate(QmThread *q) :
	QmObjectPrivate(q),
	is_main(false), qt_adapter(0), running(false)
{
}

QmThreadPrivate::~QmThreadPrivate()
{
}

QmThread::QmThread(const char * const name, QmObject *parent) :
	QmThread(*new QmThreadPrivate(this), name, parent)
{
}

QmThread::QmThread(QmThreadPrivate& dd, const char * const name, QmObject* parent) :
	QmObject(dd, parent)
{
	QM_UNUSED(name);
	QM_D(QmThread);
	if (!main_thread_registered) {
		d->is_main = true;
		d->qt_adapter = QThread::currentThread();
		main_thread_registered = true;
	} else {
		d->qt_adapter = new QmThreadPrivateAdapter(d);
	}
	registered_threads_mutex.lock();
	registered_threads[d->qt_adapter] = this;
	registered_threads_mutex.unlock();
}

QmThread::~QmThread() {
	QM_D(QmThread);
	QM_ASSERT(!d->running);
	registered_threads_mutex.lock();
	registered_threads.remove(d->qt_adapter);
	registered_threads_mutex.unlock();
	if (!d->is_main)
		delete d->qt_adapter;
}

QmThread* QmThread::currentThread() {
	QmThread *t = 0;
	if (main_thread_registered) {
		registered_threads_mutex.lock();
		t = registered_threads.value(QThread::currentThread());
		registered_threads_mutex.unlock();
	}
	return t;
}

void QmThread::start(Priority priority) {
	QM_D(QmThread);
	QThread::Priority qt_priority = QThread::IdlePriority;
	switch (priority) {
	case LowPriority: qt_priority = QThread::LowestPriority; break;
	case NormalPriority: qt_priority = QThread::NormalPriority; break;
	case HighPriority: qt_priority = QThread::HighestPriority; break;
	default: QM_ASSERT(0); break;
	}
	if (d->running) {
		qmDebugMessage(QmDebug::Warning, "QmThread::start(0x%p): already running", this);
		return;
	}
	d->running = true;
	d->qt_adapter->start(qt_priority);
	started.emit();
}

bool QmThread::isRunning() {
	QM_D(QmThread);
	return d->qt_adapter->isRunning();
}

void QmThread::run() {
	exec();
}

bool QmThread::event(QmEvent* event) {
	return QmObject::event(event);
}

int QmThread::exec() {
	QM_D(QmThread);
	QM_ASSERT(!d->is_main);
	return static_cast<QmThreadPrivateAdapter*>(d->qt_adapter)->exec();
}

void QmThread::exit(int return_code) {
	QM_D(QmThread);
	QM_ASSERT(!d->is_main);
	d->qt_adapter->exit(return_code);
}

void QmThread::quit() {
	exit(0);
}

#include "qmthread_qt.moc"
