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

class Thread : public QThread {
	Q_OBJECT
public:
	Thread(QmThreadPrivate *qmprivatethread) :
		qmprivatethread(qmprivatethread)
	{
		QObject::connect(this, &Thread::finished, this, &Thread::processFinish);
	}
	~Thread() {}
	QmThreadPrivate *qmprivatethread;
	void run() {
		qmprivatethread->run();
	}
	int exec() {
		return QThread::exec();
	}
public Q_SLOTS:
	void processFinish() {
		qmprivatethread->processFinish();
	}
};

static QMutex registered_threads_mutex;
static QMap<QThread*,QmThread*> registered_threads;
static bool main_thread_registered = false;

QmThreadPrivate::QmThreadPrivate(QmThread *q) :
	QmObjectPrivate(q),
	is_main(false), qthread(0), running(false)
{
}

QmThreadPrivate::~QmThreadPrivate()
{
}

void QmThreadPrivate::init(const char * const name) {
	QM_Q(QmThread);
	QM_UNUSED(name);
	if (!main_thread_registered) {
		is_main = true;
		qthread = QThread::currentThread();
		main_thread_registered = true;
	} else {
		qthread = new Thread(this);
	}
	registered_threads_mutex.lock();
	registered_threads[qthread] = q;
	registered_threads_mutex.unlock();
}

void QmThreadPrivate::run() {
	QM_Q(QmThread);
	q->run();
}

void QmThreadPrivate::processFinish() {
	QM_Q(QmThread);
	running = false;
	q->finished.emit();
}

QmThread::QmThread(const char * const name, QmObject *parent) :
	QmObject(*new QmThreadPrivate(this), parent)
{
	QM_D(QmThread);
	d->init(name);
}

QmThread::QmThread(QmThreadPrivate& dd, const char * const name, QmObject* parent) :
	QmObject(dd, parent)
{
	QM_D(QmThread);
	d->init(name);
}

QmThread::~QmThread() {
	QM_D(QmThread);
	QM_ASSERT(!d->running);
	registered_threads_mutex.lock();
	registered_threads.remove(d->qthread);
	registered_threads_mutex.unlock();
	if (!d->is_main)
		delete d->qthread;
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
	d->qthread->start(qt_priority);
	started.emit();
}

bool QmThread::isRunning() {
	QM_D(QmThread);
	return d->qthread->isRunning();
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
	return static_cast<Thread*>(d->qthread)->exec();
}

void QmThread::exit(int return_code) {
	QM_D(QmThread);
	QM_ASSERT(!d->is_main);
	d->qthread->exit(return_code);
}

void QmThread::quit() {
	exit(0);
}

#include "qmthread_qt.moc"
