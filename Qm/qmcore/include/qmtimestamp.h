/**
  ******************************************************************************
  * @file    qmtimestamp.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
  */

#ifndef QMTIMESTAMP_H_
#define QMTIMESTAMP_H_

class QmTimestampPrivate;

/*! The QmTimestamp class is used in conjunction with QmAbsTimer.
 *
 * Object stores current timestamp on set() call.
 * Timestamps are taken from monotonic clock (timer or system timer specific to platform)
 * with millisecond resulution.
 * After QmAbsTimer is being started with valid QmTimestamp object,
 * it will timeout relative to time stored in the object.
 *
 * \sa QmAbsTimer
 */
class QmTimestamp {
public:
	/*! Constructs an invalid timestamp object. */
	QmTimestamp();

	/*! Destroys the timestamp object. */
	~QmTimestamp();

	/*! Captures current time and stores it in the object.
	 *
	 * Object becomes valid.
	 * Captured time replaces previous value.
	 *
	 * \sa invalidate()
	 */
	void set();

	/*! Invalidates previously captured time in the object.
	 *
	 * \sa set()
	 */
	void invalidate();

	/* Shifts time stored in the object by \a msec milliseconds.
	 *
	 * Positive value increments time into future and negative value decrements time into past.
	 * Applying operation to invalid object causes undefined behavior.
	 *
	 * \sa set()
	 */
	void shift(long int msec);

private:
	friend class QmTimestampPrivateAdapter;
	friend class QmAbsTimer;
	QmTimestampPrivate *impl;
};

#endif /* QMTIMESTAMP_H_ */
