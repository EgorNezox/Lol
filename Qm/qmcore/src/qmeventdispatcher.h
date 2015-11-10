/**
  ******************************************************************************
  * @file    qmeventdispatcher.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMEVENTDISPATCHER_H_
#define QMEVENTDISPATCHER_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmEventDispatcher)

class QmEventDispatcher : public QmObject
{
public:
	QmEventDispatcher(QmObject *parent = 0);
	virtual ~QmEventDispatcher();

	void queueEvent(QmObject* receiver, QmEvent* event);
	void processEvents(QmObject *receiver = 0, int event_type = 0);
	void removeQueuedEvents(QmObject *receiver = 0, int event_type = 0);
	void blockProcessing();
	void unblockProcessing();
	static void moveQueuedEvents(QmObject *receiver_parent, QmEventDispatcher *source, QmEventDispatcher *target);
	void wakeUp();
	void interrupt();

private:
	QM_DECLARE_PRIVATE(QmEventDispatcher)
};

#endif /* QMEVENTDISPATCHER_H_ */
