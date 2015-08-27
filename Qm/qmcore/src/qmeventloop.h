/**
  ******************************************************************************
  * @file    qmeventloop.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMEVENTLOOP_H_
#define QMEVENTLOOP_H_

#include "qmobject.h"
#include "qmeventdispatcher.h"

class QmThread;

class QmEventLoop: public QmObject {
public:
	QmEventLoop();
	virtual ~QmEventLoop();

	int exec();
	void exit(int return_code = 0);

private:
	QmEventDispatcher *dispatcher;
	bool executing;
	QmThread *running_thread;
	int return_code;
};

#endif /* QMEVENTLOOP_H_ */
