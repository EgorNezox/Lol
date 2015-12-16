/**
  ******************************************************************************
  * @file    qmtimer.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    11.09.2015
  *
  ******************************************************************************
  */

#ifndef QMTIMER_H_
#define QMTIMER_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmTimer)

/*! The QmTimer class provides repetitive and single-shot timers.
 *
 * The QmTimer class provides a high-level programming interface for
 * timers. To use it, create a QmTimer, connect its timeout() signal
 * to the appropriate slots, and call start(). From then on, it will
 * emit the timeout() signal at constant intervals.
 *
 * You can set a timer to time out only once by creating timer
 * with single_shot argument set to true.
 *
 * In multithreaded applications, you can use QmTimer in any thread
 * that has an event loop. To start an event loop from a non-main
 * thread, use QmThread::exec(). Qm uses the timer's
 * \l{QmObject::thread()}{thread affinity} to determine which thread
 * will emit the \l{QmTimer::}{timeout()} signal. Because of this, you
 * must start and stop the timer in its thread; it is not possible to
 * start a timer from another thread.
 *
 * As a special case, a QmTimer with a timeout of 0 will time out as
 * soon as all the events in the event queue have
 * been processed. This can be used to do heavy work when idle
 * (in background).
 *
 * \section1 Accuracy and Timer Resolution
 *
 * The accuracy of timers depends on the underlying operating system
 * and hardware. Most platforms support a resolution of 1 millisecond,
 * though the accuracy of the timer will not equal this resolution
 * in many real-world situations.
 *
 * Timer may time out later than expected if the system is busy or
 * unable to provide the requested accuracy. In such a case of timeout
 * overrun, Qm will emit timeout() only once, even if multiple timeouts have
 * expired, and then will resume the original interval.
 */
class QmTimer: public QmObject {
public:
	/*! Constructs a timer with the given \a parent. */
	QmTimer(QmObject *parent = 0);

	/*! Constructs a timer with the given \a parent.
	 *
	 * Parameter \a single_shot specifies whether this timer
	 * should be a single-shot timer.
	 * A single-shot timer fires only once, non-single-shot timers
	 * fire every interval milliseconds.
	 */
	QmTimer(bool single_shot, QmObject *parent = 0);

	/*! Destroys the timer. */
	virtual ~QmTimer();

	/*! Returns true if the timer is running; otherwise false. */
	bool isActive() const;

	/*! Enables or disables single-shot option.
	 *
	 * Timer will be stopped if \a enable differs from acting option.
	 */
	void setSingleShot(bool enable);

	/*! Sets the timeout interval in milliseconds.
	 *
	 * The default value for this property is 0.  A QmTimer with a timeout
	 * interval of 0 will time out as soon as all the events event queue
	 * have been processed.
	 */
	void setInterval(unsigned int msec);

	/*! Returns the timeout interval in milliseconds.
	 *
	 * \sa setInterval()
	 */
	unsigned int interval() const;

	/*!
	 * Starts or restarts the timer with a timeout interval of \a msec
	 * milliseconds.
	 *
	 * If the timer is already running, it will be
	 * \l{QmTimer::stop()}{stopped} and restarted.
	 *
	 * If it's a single-shot timer, it will be activated only once.
	 *
	 * \sa stop()
	 */
	void start(unsigned int msec);

	/*!
	 * Starts or restarts the timer with the timeout specified in \l interval.
	 *
	 * If the timer is already running, it will be
	 * \l{QmTimer::stop()}{stopped} and restarted.
	 *
	 * If it's a single-shot timer, it will be activated only once.
	 *
	 * \sa stop()
	 */
	void start();

	/*! Stops the timer.
	 *
	 * \sa start()
	 */
	void stop();

	/*!
	 * This signal is emitted when the timer times out.
	 *
	 * \sa interval(), start(), stop()
	 */
	sigc::signal<void> timeout;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmTimer)
	QM_DISABLE_COPY(QmTimer)
	friend class QmTimerPrivateAdapter;
};

#endif /* QMTIMER_H_ */
