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
 * Timestamps are taken from monotonic clock (timer or system timer specific to platform).
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
	 * Object becomes valid and after QmAbsTimer is being started with this object,
	 * it will timeout relative to current time.
	 *
	 * \sa invalidate()
	 */
	void set();

	/*! Invalidates previously captured time in the object.
	 *
	 * \sa set()
	 */
	void invalidate();

private:
	friend class QmTimestampPrivateAdapter;
	friend class QmAbsTimer;
	QmTimestampPrivate *impl;
};

#endif /* QMTIMESTAMP_H_ */
