/**
  ******************************************************************************
  * @file    qmmutex.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMMUTEX_H_
#define QMMUTEX_H_

class QmMutexPrivate;

/*! The QmMutex class provides access serialization between threads.
 *
 * The purpose of a QmMutex is to protect an object, data structure or
 * section of code so that only one thread can access it at a time. It is
 * usually best to use a mutex with a QmMutexLocker since this makes
 * it easy to ensure that locking and unlocking are performed
 * consistently.
 *
 * When you call lock() in a thread, other threads that try to call
 * lock() in the same place will block until the thread that got the
 * lock calls unlock().
 *
 * \threadsafe
 *
 * \sa QmMutexLocker
 */
class QmMutex {
public:
	enum RecursionMode {
		NonRecursive,	/*!< In this mode, a thread can lock the same mutex
							multiple times and the mutex won't be unlocked
							until a corresponding number of unlock() calls
							have been made.
							*/
		Recursive		/*!< In this mode, a thread may only lock a mutex once. */
	};

	/*! Constructs a new mutex. The mutex is created in an unlocked state.
	 *
	 * If \a mode is QmMutex::Recursive, a thread can lock the same mutex
	 * multiple times and the mutex won't be unlocked until a
	 * corresponding number of unlock() calls have been made. Otherwise
	 * a thread may only lock a mutex once. The default is
	 * QmMutex::NonRecursive.
	 */
	QmMutex(RecursionMode mode = NonRecursive);

	virtual ~QmMutex();

	/*! Locks the mutex. If another thread has locked the mutex then this
	 * call will block until that thread has unlocked it.
	 *
	 * Calling this function multiple times on the same mutex from the
	 * same thread is allowed if this mutex is a
	 * \l{QmMutex::Recursive}{recursive mutex}. If this mutex is a
	 * \l{QmMutex::NonRecursive}{non-recursive mutex}, this function will
	 * \e dead-lock when the mutex is locked recursively.
	 *
	 * \sa unlock()
	 */
	void lock();

	/*! Unlocks the mutex. Attempting to unlock a mutex in a different
	 * thread to the one that locked it results in an error. Unlocking a
	 * mutex that is not locked results in undefined behavior.
	 *
	 * \sa lock()
	 */
	void unlock();

private:
	QmMutexPrivate *impl;
};

#endif /* QMMUTEX_H_ */
