/**
  ******************************************************************************
  * @file    qmpushbuttonkey.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMPUSHBUTTONKEY_H_
#define QMPUSHBUTTONKEY_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmPushButtonKey)

/*! The QmPushButtonKey class provides input from push-button key.
 *
 * Class provides software debouncing of hardware input signal.
 */
class QmPushButtonKey: public QmObject {
public:
	/*! Constructs an push-button key with the given \a parent.
	 *
	 * Parameter \a hw_resource specifies platform-identified instance of key.
	 */
	QmPushButtonKey(int hw_resource, QmObject *parent = 0);

	/*! Destroys the push-button key. */
	virtual ~QmPushButtonKey();

	/*! Returns true if key is pressed; otherwise false. */
	bool isPressed();

	/*!
	 * This signal is emitted when key state change detected.
	 */
	sigc::signal<void> stateChanged;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmPushButtonKey)
	QM_DISABLE_COPY(QmPushButtonKey)
};

#endif /* QMPUSHBUTTONKEY_H_ */
