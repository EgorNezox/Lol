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

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmRtc)

class QmRtc: public QmObject {
public:
	QmRtc(int hw_resource, QmObject *parent = 0);
	virtual ~QmRtc();

	void init();
	void deinit();

//	void setTime();
//	void setDate();
//	void getTime();
//	void getDate();

	sigc::signal<void> pps;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmRtc)
	QM_DISABLE_COPY(QmRtc)
	friend class QmRtcPrivateAdapter;
};



#endif /* QM_QMRTC_INCLUDE_QMRTC_H_ */
