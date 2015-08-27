/**
  ******************************************************************************
  * @file    qmobject.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMOBJECT_H_
#define QMOBJECT_H_

#include <list>

#include "qm.h"
#include "sigc++/signal.h"
#include "sigc++/trackable.h"

struct QmObjectPrivate;
class QmObject;
class QmEvent;
class QmThread;

typedef std::list<QmObject*> QmObjectList;

/*! The QmObject class is the base class of all Qm objects.
 *
 * QObject is the heart of the Qm Object Model. The central
 * feature in this model is a very powerful mechanism for seamless
 * object communication called \e{signals and slots}. You can
 * connect a signal to a slot using libsigc++ library.
 * QmObject is derived from sigc::trackable class thus making it possible
 * to create slots from its methods and connect signals to them.
 * Signals can be defined as members of QmObjects.
 * For further details see libsigc++ documentation.
 *
 * QmObjects organize themselves in object trees.
 * When you create a QmObject with another object as
 * parent, the object will automatically add itself to the parent's
 * children() list. The parent takes ownership of the object; i.e.,
 * it will automatically delete its children in its destructor.
 *
 * When an object is deleted, it emits a destroyed() signal. You can
 * catch this signal to avoid dangling references to QmObjects.
 *
 * QmObjects can receive events through event().
 *
 * \section1 Thread Affinity
 *
 * A QmObject instance is said to have a \e{thread affinity}, or that
 * it \e{lives} in a certain thread. When a QmObject receives a \e{posted event},
 * the event handler will run in the thread that the object lives in.
 *
 * \note If a QmObject has no thread affinity (that is, if thread()
 * returns zero), or if it lives in a thread that has no running event
 * loop, then it cannot receive posted events.
 *
 * By default, a QmObject lives in the thread in which it is created.
 * An object's thread affinity can be queried using thread() and
 * changed using moveToThread().
 *
 * All QmObjects must live in the same thread as their parent. Consequently:
 *
 * \list
 * \li setParent() will fail if the two QmObjects involved live in
 * 		different threads.
 * \li When a QmObject is moved to another thread, all its children
 * 		will be automatically moved too.
 * \li moveToThread() will fail if the QmObject has a parent.
 * \li If QmObjects are created within QmThread::run(), they cannot
 * 		become children of the QmThread object because the QmThread does
 * 		not live in the thread that calls QmThread::run().
 * \endlist
 *
 * \note A QmObject's member variables \e{do not} automatically become
 * its children. The parent-child relationship must be set by either
 * passing a pointer to the child's \l{QmObject()}{constructor}, or by
 * calling setParent(). Without this step, the object's member variables
 * will remain in the old thread when moveToThread() is called.
 *
 * \target No copy constructor
 * \section1 No Copy Constructor or Assignment Operator
 *
 * QmObject has neither a copy constructor nor an assignment operator.
 * This is by design.
 * The main consequence is that you should use pointers to QmObject
 * (or to your QmObject subclass) where you might otherwise be tempted
 * to use your QmObject subclass as a value. For example, without a
 * copy constructor, you can't use a subclass of QmObject as the value
 * to be stored in one of the container classes. You must store
 * pointers.
 *
 * \reentrant
 */
class QmObject : public sigc::trackable
{
public:

	/*! Constructs an object with parent object \a parent.
	 *
	 * The parent of an object may be viewed as the object's owner.
	 * The destructor of a parent object destroys all child objects.
	 * Setting \a parent to 0 constructs an object with no parent.
	 *
	 * \sa parent()
	 */
	explicit QmObject(QmObject *parent = 0);

	/*! Destroys the object, deleting all its child objects.
	 *
	 * All signals to and from the object are automatically disconnected, and
	 * any pending posted events for the object are removed from the event
	 * queue. However, it is often safer to use deleteLater() rather than
	 * deleting a QmObject subclass directly.
	 *
	 * \warning All child objects are deleted. If any of these objects
	 * are on the stack or global, sooner or later your program will
	 * crash. You are not recommended holding pointers to child objects from
	 * outside the parent. If you still do, the destroyed() signal gives
	 * you an opportunity to detect when an object is destroyed.
	 *
	 * \warning You must not delete the QmObject
	 * directly if it exists in a different thread than the one currently
	 * executing. Use deleteLater() instead, which will cause the event
	 * loop to delete the object after all pending events have been
	 * delivered to it.
	 *
	 * \sa deleteLater()
	 */
	virtual ~QmObject();

	/*! Returns a list of child objects.
	 *
	 * The first child added is the first (beginning, front) object in
	 * the list and the last child added is the last (ending, back)
	 * object in the list, i.e. new children are appended at the end.
	 *
	 * \sa parent(), setParent()
	 */
	QmObjectList children() const;

	/*! Makes the object a child of \a parent.
	 *
	 * \sa parent(), children()
	 */
	void setParent(QmObject *parent);

	/*! Returns a pointer to the parent object.
	 *
	 * \sa children()
	 */
	QmObject *parent() const;

	/*! Schedules this object for deletion.
	 *
	 * The object will be deleted when control returns to the event
	 * loop. If the event loop is not running when this function is
	 * called (e.g. deleteLater() is called on an object before
	 * QmApplication::exec()), the object will be deleted once the
	 * event loop is started. If deleteLater() is called after the main event loop
	 * has stopped, the object will not be deleted.
	 * If deleteLater() is called on an object that lives in a
	 * thread with no running event loop, the object will be destroyed when the
	 * thread finishes.
	 *
	 * \b{Note:} It is safe to call this function more than once; when the
	 * first deferred deletion event is delivered, any pending events for the
	 * object are removed from the event queue.
	 *
	 * \sa destroyed()
	 */
	void deleteLater();

	/*! Returns the thread in which the object lives.
	 *
	 * \sa moveToThread()
	 */
	QmThread *thread() const;

	/*!
	 * Changes the thread affinity for this object and its children. The
	 * object cannot be moved if it has a parent. Event processing will
	 * continue in the \a thread.
	 *
	 * If \a thread is zero, all event processing for this object
	 * and its children stops.
	 *
	 * \warning This function is \e not thread-safe; the current thread
	 * must be same as the current thread affinity. In other words, this
	 * function can only "push" an object from the current thread to
	 * another thread, it cannot "pull" an object from any arbitrary
	 * thread to the current thread.
	 *
	 * \sa thread()
	 */
	void moveToThread(QmThread *thread);

	/*!
	 * This signal is emitted immediately before the object is
	 * destroyed, and can not be blocked.
	 *
	 * First argument is object to be destroyed.
	 * All the objects's children are destroyed immediately after this
	 * signal is emitted.
	 *
	 * \sa deleteLater()
	 */
	sigc::signal<void, QmObject*> destroyed;

protected:

	/*!
	 * This virtual function receives events to an object and should
	 * return true if the event \a event was recognized and processed.
	 *
	 * The event() function can be reimplemented to customize the
	 * behavior of an object.
	 *
	 * \sa QmApplication::sendEvent(), QmApplication::postEvent()
	 */
	virtual bool event(QmEvent *event);

protected:
	QmObject(QmObjectPrivate &, QmObject *parent = 0);
	QmObjectPrivate *d_ptr;
private:
	QM_DECLARE_PRIVATE(QmObject)
	QM_DISABLE_COPY(QmObject)
	friend class QmApplication;
	friend class QmApplicationPrivate;
};

#endif /* QMOBJECT_H_ */
