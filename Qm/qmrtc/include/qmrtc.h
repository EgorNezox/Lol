/**
  ******************************************************************************
  * @file    qmrtc.h
  * @author  Petr Dmitriev
  * @date    23.11.2016
  *
  ******************************************************************************
 */

#ifndef QM_QMRTC_INCLUDE_QMRTC_H_
#define QM_QMRTC_INCLUDE_QMRTC_H_

#include <stdint.h>
#include "qmobject.h"

QM_FORWARD_PRIVATE(QmRtc)

class QmRtc: public QmObject {
public:
	struct Time {
		uint8_t hours;
		uint8_t minutes;
		uint8_t seconds;
	};

	struct Date {
		uint8_t weekday; /* 1 - Monday, ..., 7 - Sunday */
		uint8_t day;
		uint8_t month;
		uint8_t year;
	};

	QmRtc(int hw_resource, QmObject *parent = 0);
	virtual ~QmRtc();

	void setTime(Time& time);
	void setDate(Date& date);
	Time getTime();
	Date getDate();

	sigc::signal<void> wakeup;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmRtc)
	QM_DISABLE_COPY(QmRtc)
	friend class QmRtcPrivateAdapter;
};



#endif /* QM_QMRTC_INCLUDE_QMRTC_H_ */
