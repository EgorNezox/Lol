/**
  ******************************************************************************
  * @file    qmeventloop.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#include "qmdebug.h"
#include "qmthread.h"
#include "qmthread_p.h"
#include "qmeventdispatcher.h"
#include "qmeventloop.h"

QmEventLoop::QmEventLoop() :
	QmObject(),
	dispatcher(0),
	executing(false), running_thread(0), return_code(-1)
{
}

QmEventLoop::~QmEventLoop()
{
	QM_ASSERT(!executing);
}

int QmEventLoop::exec() {
	QM_ASSERT(!executing); // nested loop execution isn't supported
	running_thread = QmThread::currentThread();
	QM_ASSERT(running_thread);
	dispatcher = running_thread->d_func()->event_dispatcher;
	executing = true;
	while (executing)
		dispatcher->processEvents();
	return return_code;
}

void QmEventLoop::exit(int return_code) {
	QM_ASSERT(running_thread == QmThread::currentThread()); // must be called from thread context which runs exec()
	QM_ASSERT(executing); // must be called within exec() execution
	this->return_code = return_code;
	executing = false;
	dispatcher->interrupt();
}
