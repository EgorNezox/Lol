/**
  ******************************************************************************
  * @file    qmmatrixkeyboard.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMMATRIXKEYBOARD_H_
#define QMMATRIXKEYBOARD_H_

#include "qmobject.h"

QM_FORWARD_PRIVATE(QmMatrixKeyboard)

/*! The QmMatrixKeyboard class provides input from classic matrix keyboard.
 *
 * Keys identifiers are simple integers, their values are interpreted by class user
 * based on platform definitions for given instance of keyboard.
 */
class QmMatrixKeyboard: public QmObject {
public:
	enum PressType {
		PressSingle,
		PressLong
	};

	/*! Constructs an matrix keyboard with the given \a parent.
	 *
	 * Parameter \a hw_resource specifies platform-identified instance of keyboard.
	 */
	QmMatrixKeyboard(int hw_resource, QmObject *parent = 0);

	/*! Destroys the matrix keyboard. */
	virtual ~QmMatrixKeyboard();

	/*! Returns keys quantity of the matrix keyboard.
	 *
	 * Parameter \a hw_resource specifies platform-identified instance of keyboard.
	 **/
	static int keysNumber(int hw_resource);

	/*! Returns true if key with identifier \a id is pressed; otherwise false. */
	bool isKeyPressed(int id);

	/*!
	 * This signal is emitted when state change detected on any key.
	 *
	 * First(int) argument is key identifier.
	 * Second(bool) argument is new state (true - pressed, false - not pressed).
	 */
	sigc::signal<void, int, bool> keyStateChanged;

	/*!
	 * This signal is emitted when action performed on any key.
	 *
	 * First(int) argument is key identifier.
	 * Second(PressType) argument is type of action.
	 */
	sigc::signal<void, int, PressType> keyAction;

protected:
	virtual bool event(QmEvent *event);

private:
	QM_DECLARE_PRIVATE(QmMatrixKeyboard)
	QM_DISABLE_COPY(QmMatrixKeyboard)
};

#endif /* QMMATRIXKEYBOARD_H_ */
