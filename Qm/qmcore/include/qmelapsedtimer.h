/**
 ******************************************************************************
 * @file    qmelapsedtimer.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    20.02.2016
 *
 ******************************************************************************
 */

#ifndef QMELAPSEDTIMER_H_
#define QMELAPSEDTIMER_H_

#include <stdint.h>

class QmElapsedTimerPrivate;

class QmElapsedTimer {
public:
	QmElapsedTimer();
	~QmElapsedTimer();

	void start();
	int64_t restart();
	void invalidate();
	bool isValid() const;
	int64_t elapsed() const;
	bool hasExpired(int64_t timeout) const;

private:
	QmElapsedTimerPrivate *d_ptr;
};

#endif /* QMELAPSEDTIMER_H_ */
