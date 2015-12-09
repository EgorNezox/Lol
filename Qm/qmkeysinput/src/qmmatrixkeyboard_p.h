/**
  ******************************************************************************
  * @file    qmmatrixkeyboard_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @author  Petr Dmitriev
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#ifndef QMMATRIXKEYBOARD_P_H_
#define QMMATRIXKEYBOARD_P_H_

#include "../../system/platform/platform_hw_map.h"

#include <vector>
#include "qmtimer.h"
#include "../../qmcore/src/qmobject_p.h"
#include "qmmatrixkeyboard.h"

#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
#include "hal_gpio.h"
#include "../../qmcore/src/qm_core.h"
#endif

class QmMatrixKeyboardPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmMatrixKeyboard)
public:
	QmMatrixKeyboardPrivate(QmMatrixKeyboard *q);
	virtual ~QmMatrixKeyboardPrivate();
private:
	void init();
	void deinit();
	int hw_resource;
#ifdef QMHARDWAREIO_PLATFORM_STM32F2XX
	typedef enum keyboard_state {
		no_presses,
		single_press,
		double_press
	} keyboard_state_t;

	void scan();
	bool scanKBMatrix(uint8_t* pressesCounter, int8_t keysPressed[], uint8_t maxPresses);
	void scanKBColumn(int columnNumber, hal_gpio_level_t row[]);
	void noPressesHandler(keyboard_state_t prev_keyboard_state, int8_t curKeysPressed[], int8_t prevKeysPressed[]);
	void singlePressHandler(keyboard_state_t prev_keyboard_state, int8_t curKeysPressed[], int8_t prevKeysPressed[]);
	void doublePressHandler(keyboard_state_t prev_keyboard_state, int8_t curKeysPressed[], int8_t prevKeysPressed[]);
	void keyPressed(int8_t key_code);
	void keyReleased(int key_number, int8_t key_code);

	hal_gpio_pin_t* column_pins;
	hal_gpio_pin_t* row_pins;
	int column_count;
	int row_count;
	QmTimer* poll_timer;
	QmTimer* press_timer;

	bool* keyPressedLong;
	bool* longPressAction;
	uint8_t* curKeysPressedSequence;
#endif
};

#endif /* QMMATRIXKEYBOARD_P_H_ */
