/**
  ******************************************************************************
  * @file    keyboard.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    30.10.2015
  *
  ******************************************************************************
 */

#ifndef FIRMWARE_APP_UI_KEYBOARD_H_
#define FIRMWARE_APP_UI_KEYBOARD_H_

namespace Ui {

enum matrix_keyboard_key {
	matrixkbkeyEnter,
	matrixkbkeyBack,
	matrixkbkeyUp,
	matrixkbkeyDown,
	matrixkbkeyLeft,
	matrixkbkeyRight,
	matrixkbkey0,
	matrixkbkey1,
	matrixkbkey2,
	matrixkbkey3,
	matrixkbkey4,
	matrixkbkey5,
	matrixkbkey6,
	matrixkbkey7,
	matrixkbkey8,
	matrixkbkey9,
	matrixkbKeysCount
};

enum aux_keyboard_key {
	auxkbkeyChNext,
	auxkbkeyChPrev,
	auxkbKeysCount
};

struct matrix_keyboard_t {
	int resource;
	int key_id[matrixkbKeysCount];
};

struct aux_keyboard_t {
	int key_iopin_resource[auxkbKeysCount];
};

} /* namespace Ui */

#endif /* FIRMWARE_APP_UI_KEYBOARD_H_ */
