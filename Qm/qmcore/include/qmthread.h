/**
  ******************************************************************************
  * @file    qmthread.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMTHREAD_H_
#define QMTHREAD_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmThread)
class QmEvent;

/*! The QmThread class provides a platform-independent way to
 * manage threads.
 *
 * A QmThread object manages one thread of control within the
 * program. QmThreads begin executing in run(). By default, run() starts the
 * event loop by calling exec() and runs a Qm event loop inside the thread.
 *
 * You can use worker objects by moving them to the thread using
 * QmObject::moveToThread.
 * Another way to make code run in a separate thread, is to subclass QmThread
 * and reimplement run().
 *
 * It is important to remember that a QmThread instance \e {lives in}
 * the old thread that instantiated it, not in the
 * new thread that calls run(). This means that all of QmThread's
 * slots will execute in the old thread. Thus, a developer who wishes to
 * invoke slots in the new thread must use the worker-object approach; new
 * slots should not be implemented directly into a subclassed QmThread.
 *
 * When subclassing QmThread, keep in mind that the constructor executes in
 * the old thread while run() executes in the new thread. If a member
 * variable is accessed from both functions, then the variable is accessed
 * from two different threads. Check that it is safe to do so.
 *
 * \note Care must be taken when interacting with objects across different
 * threads. See QmMutex and QmMutexLocker for details.
 *
 * \section1 Managing Threads
 *
 * QmThread will notify you via a signal when the thread is
 * started() and finished(), or you can use
 * isRunning() to query the state of the thread.
 *
 * You can stop the thread by calling exit() or quit().
 */
class QmThread : public QmObject
{
public:
	enum Priority {
		LowPriority,
		NormalPriority,
		HighPriority,
	};

	/*!
	 * Constructs a new QmThread to manage a new thread with optional \a name.
	 * The \a parent takes ownership of the QmThread. The thread does not begin
	 * executing until start() is called.
	 *
	 * \sa start()
	 */
	QmThread(const char * const name = "", QmObject *parent = 0);

	/*! Destroys the QmThread.
	 *
	 * Note that deleting a QmThread object will not stop the execution
	 * of the thread it manages. Deleting a running QmThread (i.e.
	 * isRunning() returns \c true) will probably result in a program
	 * crash. Wait for the finished() signal before deleting the
	 * QmThread.
	 */
	virtual ~QmThread();

	/*!
	 * Returns a pointer to a QmThread which manages the currently
	 * executing thread.
	 */
	static QmThread* currentThread();

	/*!
	 * Begins execution of the thread by calling run(). The
	 * operating system will schedule the thread according to the \a
	 * priority parameter. If the thread is already running, this
	 * function does nothing.
	 *
	 * The effect of the \a priority parameter is dependent on the
	 * operating system's scheduling policy. In particular, the \a priority
	 * will be ignored on systems that do not support thread priorities.
	 *
	 * \sa run()
	 */
	virtual void start(Priority priority = NormalPriority);

	/*!
	 * Returns \c true if the thread is running; otherwise returns \c false.
	 */
	bool isRunning();

	/*!
	 * This signal is emitted from the associated thread when it starts executing,
	 * before the run() function is called.
	 *
	 * \sa finished()
	 */
	sigc::signal<void> started;

	/*!
	 * This signal is emitted from the associated thread right before it finishes executing.
	 *
	 * When this signal is emitted, the event loop has already stopped running.
	 *
	 * \sa started()
	 */
	sigc::signal<void> finished;

protected:
	QmThread(QmThreadPrivate &, const char * const name, QmObject *parent);

	virtual bool event(QmEvent *event);

	/*!
	 * The starting point for the thread. After calling start(), the
	 * newly created thread calls this function. The default
	 * implementation simply calls exec().
	 *
	 * You can reimplement this function to facilitate advanced thread
	 * management. Returning from this method will end the execution of
	 * the thread.
	 *
	 * \sa start()
	 */
	virtual void run();

	/*!
	 * Enters the event loop and waits until exit() is called, returning the value
	 * that was passed to exit(). The value returned is 0 if exit() is called via
	 * quit().
	 *
	 * This function is meant to be called from within run(). It is necessary to
	 * call this function to start event handling.
	 *
	 * \sa quit(), exit()
	 */
	int exec();

	/*!
	 * Tells the thread's event loop to exit with a return code.
	 *
	 * After calling this function, the thread leaves the event loop and
	 * returns from the call to exec(). The exec() function returns \a return_code.
	 *
	 * By convention, a \a return_code of 0 means success, any non-zero value
	 * indicates an error.
	 *
	 * Note that unlike the C library function of the same name, this
	 * function \e does return to the caller -- it is event processing
	 * that stops.
	 *
	 * No event loop will be started anymore in this thread until
	 * exec() has been called again.
	 *
	 * \sa quit(), exec()
	 */
	virtual void exit(int return_code = 0);

	/*!
	 * Tells the thread's event loop to exit with return code 0 (success).
	 * Equivalent to calling exit(0).
	 *
	 * \sa exit(), exec()
	 */
	void quit();

private:
	QM_DECLARE_PRIVATE(QmThread)
	QM_DISABLE_COPY(QmThread)
	friend class QmThreadPrivateAdapter;
	friend class QmObjectPrivate;
	friend class QmEventLoop;
	friend class QmApplication;
};

#endif /* QMTHREAD_H_ */
