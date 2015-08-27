/**
  ******************************************************************************
  * @file    qmmutexlocker.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    29.09.2015
  *
  ******************************************************************************
  */

#ifndef QMMUTEXLOCKER_H_
#define QMMUTEXLOCKER_H_

#include "qmmutex.h"

/*! The QmMutexLocker class is a convenience class that simplifies
 * locking and unlocking mutexes.
 *
 * Locking and unlocking a QmMutex in complex functions and
 * statements or in exception handling code is error-prone and
 * difficult to debug. QmMutexLocker can be used in such situations
 * to ensure that the state of the mutex is always well-defined.
 *
 * QmMutexLocker should be created within a function where a
 * QmMutex needs to be locked. The mutex is locked when QmMutexLocker
 * is created. You can unlock and relock the mutex with \c unlock()
 * and \c relock(). If locked, the mutex will be unlocked when the
 * QmMutexLocker is destroyed.
 *
 * \threadsafe
 */
class QmMutexLocker {
public:

	/*! Constructs a QmMutexLocker and locks \a mutex. The mutex will be
	 * unlocked when the QmMutexLocker is destroyed.
	 *
	 * \sa QmMutex::lock()
	 */
	QmMutexLocker(QmMutex *mutex);

	/*! Destroys the QMutexLocker and unlocks the mutex that was locked
	 * in the constructor.
	 *
	 * \sa QmMutex::unlock()
	 */
	virtual ~QmMutexLocker();

	/*! Relocks an unlocked mutex locker.
	 *
	 * \sa unlock()
	 */
	void relock();

	/*! Unlocks this mutex locker. You can use \c relock() to lock
	 * it again. It does not need to be locked when destroyed.
	 *
	 * \sa relock()
	 */
	void unlock();

private:
	QmMutex *mutex;
};

#endif /* QMMUTEXLOCKER_H_ */
