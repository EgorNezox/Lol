/**
  ******************************************************************************
  * @file    qmabstimer.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
  */

#ifndef QMABSTIMER_H_
#define QMABSTIMER_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmAbsTimer)

class QmTimestamp;

/*! The QmAbsTimer class provides single-shot timers which timeouts relative to given timestamp.
 *
 * Usage of this class is similar to QmTimer except that this timer is a single shot timer and it has no interval.
 *
 * \sa QmTimer, QmTimestamp
 */
class QmAbsTimer: public QmObject {
public:
	/*! Constructs a timer with the given \a parent. */
	QmAbsTimer(QmObject *parent = 0);

	/*! Destroys the timer. */
	virtual ~QmAbsTimer();

	/*! Returns true if the timer is running; otherwise false.
	 *
	 * Note that timer is running after started with expired timeout (timestamp+msec is in the past) until timeout event processed.
	 */
	bool isActive() const;

	/*!
	 * Starts or restarts the timer with a timeout interval of \a msec
	 * milliseconds relative to \a timestamp time.
	 * If interval already expired (timestamp+msec is in the past), it will timeout immediately when control returns to event loop.
	 *
	 * If the timer is already running, it will be
	 * \l{QmTimer::stop()}{stopped} and restarted.
	 *
	 * Returns \c true if timer was started; otherwise returns \c false (timestamp isn't valid).
	 *
	 * \sa stop()
	 */
	bool start(QmTimestamp *timestamp, unsigned int msec);

	/*! Stops the timer.
	 *
	 * \sa start()
	 */
	void stop();

	/*!
	 * This signal is emitted when the timer times out.
	 *
	 * \sa start(), stop()
	 */
	sigc::signal<void> timeout;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmAbsTimer)
	QM_DISABLE_COPY(QmAbsTimer)
	friend class QmAbsTimerPrivateAdapter;
};

#endif /* QMABSTIMER_H_ */
