/**
  ******************************************************************************
  * @file    qmapplication.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMAPPLICATION_H_
#define QMAPPLICATION_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmApplication)

/*! The QmApplication class provides an event loop for Qm
 *
 * For application that uses Qm, there should be exactly one QmApplication object.
 *
 * QmApplication contains the main event loop, where all events
 * from the underlying system (e.g., timer and device i/o events) and
 * other sources are processed and dispatched. It also handles the
 * application's initialization and finalization.
 *
 * \section1 The Event Loop and Event Handling
 *
 * The event loop is started with a call to exec().
 *
 * In general, you are recommended to create a QmApplication
 * object in your \c qmMain() function as early as
 * possible. exec() will not return until the event loop exits.
 *
 * Several static convenience functions are also provided.
 * Events can be sent or posted using sendEvent(), postEvent(), and
 * sendPostedEvents(). Pending events can be removed with
 * removePostedEvents().
 */
class QmApplication : public QmObject
{
public:
	/*! Constructs a Qm kernel application */
	QmApplication();

	/*! Destroys the QmApplication object */
	virtual ~QmApplication();

	/*!
	 * Enters the main event loop and waits until exit (bare-metal port has no means to exit).
	 *
	 * It is necessary to call this function to start event handling.
	 * The main event loop receives events from the underlying system and
	 * dispatches these to the application objects.
	 *
	 * To make an application perform idle processing (by executing a
	 * special function whenever there are no pending events), use a
	 * QmTimer with 0 timeout.
	 */
	static void exec();

	/*
	 * Sends event \a event directly to receiver \a receiver.
	 * Returns the value that was returned from the event handler.
	 *
	 * The event is \e not deleted when the event has been sent. The normal
	 * approach is to create the event on the stack.
	 *
	 * \sa postEvent(), sendPostedEvents()
	 */
	static bool sendEvent(QmObject *receiver, QmEvent *event);

	/*
	 * Adds the event \a event, with the object \a receiver as the
	 * receiver of the event, to an event queue and returns immediately.
	 *
	 * The event must be allocated on the heap since the post event queue
	 * will take ownership of the event and delete it once it has been
	 * posted.  It is \e {not safe} to access the event after
	 * it has been posted.
	 *
	 * When control returns to the main event loop, all events that are\
	 * stored in the queue will be sent to event handlers.
	 *
	 * \threadsafe
	 *
	 * \sa sendEvent(), sendPostedEvents()
	 */
	static void postEvent(QmObject *receiver, QmEvent *event);

	/*
	 * Immediately dispatches all events which have been previously queued
	 * with QmApplication::postEvent() and which are for the object \a receiver
	 * and have the event type \a event_type.
	 *
	 * If \a receiver is null, the events of \a event_type are sent for all
	 * objects. If \a event_type is 0, all the events are sent for \a receiver.
	 *
	 * \note This method must be called from the same thread as its QObject parameter, \a receiver.
	 *
	 * \sa postEvent()
	 */
	static void sendPostedEvents(QmObject *receiver = 0, int event_type = 0);

	/*
	 * Removes all events of the given \a event_type that were posted
	 * using postEvent() for \a receiver.
	 *
	 * The events are \e not dispatched, instead they are removed from
	 * the queue. You should never need to call this function. If you do
	 * call it, be aware that killing events may cause inconsistent behavior in \a receiver
	 * (for example, not being deleted and cause memory leak).
	 *
	 * If \a receiver is null, the events of \a event_type are removed for
	 * all objects. If \a event_type is 0, all the events are removed for
	 * \a receiver.
	 */
	static void removePostedEvents(QmObject *receiver = 0, int event_type = 0);

private:
	QM_DECLARE_PRIVATE(QmApplication)
	QM_DISABLE_COPY(QmApplication)
	static QmApplication *self;
};

#endif /* QMAPPLICATION_H_ */
