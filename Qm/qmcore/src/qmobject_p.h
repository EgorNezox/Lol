/**
  ******************************************************************************
  * @file    qmobject_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMOBJECT_P_H_
#define QMOBJECT_P_H_

#include <list>
#include "qm.h"
#include "qmmutex.h"

#ifdef QMCORE_PLATFORM_QT
class QObject;
#endif /* QMCORE_PLATFORM_QT */
class QmObject;
class QmEvent;
class QmThread;

struct QmObjectPrivate
{
	QmObjectPrivate(QmObject *q);
	virtual ~QmObjectPrivate();
	QmObject *q_ptr;
	QmThread *thread;
	bool pending_delete;
	bool deliverEvent(QmEvent *event);
	void moveToThread(QmThread* thread);
	void cleanup();
#ifdef QMCORE_PLATFORM_BMFREERTOS
	static void lockAndAssignOtherThreadRecursively(QmObject *object, QmThread* other_thread, bool is_root);
	static void unlockRecursively(QmObject *object, bool is_root);
	QmObject *parent;
	std::list<QmObject*> children;
	mutable QmMutex ta_mutex;
	bool drop_events;
#endif /* QMCORE_PLATFORM_BMFREERTOS */
#ifdef QMCORE_PLATFORM_QT
	static void assignOtherThreadRecursively(QmObject *object, QmThread* other_thread);
	QObject *qobject;
#endif /* QMCORE_PLATFORM_QT */
};

#endif /* QMOBJECT_P_H_ */
