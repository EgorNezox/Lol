/**
  ******************************************************************************
  * @file    qmmatrixkeyboard_stm32f2xx.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @author  Petr Dmitriev
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#define QMDEBUGDOMAIN QmMatrixKeyboard

#define KEYBOARD_MAX_PRESSES			2
#define KEYPRESS_LONG_TIME				1000 //ms
#define KEYBOARD_POLLING_DELAY			50 //ms

#include "system_hw_io.h"

#include "hal_timer.h"
#include "qmdebug.h"
#include "qmmatrixkeyboard.h"
#include "qmmatrixkeyboard_p.h"
#include "qmevent.h"

QmMatrixKeyboardPrivate::QmMatrixKeyboardPrivate(QmMatrixKeyboard *q) :
	QmObjectPrivate(q),
	hw_resource(-1), column_pins(NULL), row_pins(NULL)
{
	poll_timer = new QmTimer();
	poll_timer->setInterval(KEYBOARD_POLLING_DELAY);
	poll_timer->timeout.connect(sigc::mem_fun(this, &QmMatrixKeyboardPrivate::scan));
	press_timer = new QmTimer[KEYBOARD_MAX_PRESSES];
	for (int i = 0; i < KEYBOARD_MAX_PRESSES; ++i) {
		press_timer[i].setSingleShot(true);
		press_timer[i].setInterval(KEYPRESS_LONG_TIME);
	}

	keyPressedLong = new bool[KEYBOARD_MAX_PRESSES];
	longPressAction = new bool[KEYBOARD_MAX_PRESSES];
	curKeysPressedSequence = new uint8_t[KEYBOARD_MAX_PRESSES];
}

QmMatrixKeyboardPrivate::~QmMatrixKeyboardPrivate()
{
	delete poll_timer;
	delete[] press_timer;
	delete[] keyPressedLong;
	delete[] longPressAction;
	delete[] curKeysPressedSequence;
}

void QmMatrixKeyboardPrivate::init() {
	stm32f2_get_matrixkeyboard_pins(hw_resource, &column_pins, &column_count, &row_pins, &row_count);
	stm32f2_ext_pins_init(hw_resource);

	for (int i = 0; i < KEYBOARD_MAX_PRESSES; ++i) {
		keyPressedLong[i] = false;
		longPressAction[i] = false;
		curKeysPressedSequence[i] = 0;
	}

	poll_timer->start();
}

void QmMatrixKeyboardPrivate::deinit() {
	poll_timer->stop();
	stm32f2_ext_pins_deinit(hw_resource);
	delete[] column_pins;
	delete[] row_pins;
	column_count = 0;
	row_count = 0;
}

void QmMatrixKeyboardPrivate::scan() {
	keyboard_state_t keyboard_state = no_presses;
	int8_t curKeysPressed[KEYBOARD_MAX_PRESSES] = {0, 0};
	int8_t prevKeysPressed[KEYBOARD_MAX_PRESSES] = {0, 0};
	uint8_t pressesCounter = 0;

	if (scanKBMatrix(&pressesCounter, curKeysPressed, KEYBOARD_MAX_PRESSES)) {
		switch (pressesCounter) {
		case 0:
			noPressesHandler(keyboard_state, curKeysPressed, prevKeysPressed);
			keyboard_state = no_presses;
			break;
		case 1:
			singlePressHandler(keyboard_state, curKeysPressed, prevKeysPressed);
			keyboard_state = single_press;
			break;
		case 2:
			doublePressHandler(keyboard_state, curKeysPressed, prevKeysPressed);
			keyboard_state = double_press;
			break;
		default:
			QM_ASSERT(0);
		}
	}
}

bool QmMatrixKeyboardPrivate::scanKBMatrix(uint8_t* pressesCounter, int8_t keysPressed[], uint8_t maxPresses)
{
	*pressesCounter = 0;
	for (int col = 0; col < column_count; ++col) {
		hal_gpio_level_t matrix[row_count];
		scanKBColumn(col, matrix);
		for (int row = 0; row < row_count; ++row) {
			if (hgpioLow == matrix[row]) {
				if (*pressesCounter >= maxPresses) {
					return false;
				}
				keysPressed[*pressesCounter] = row * row_count + col;
				++(*pressesCounter);
			}
		}
	}
	return true;
}

void QmMatrixKeyboardPrivate::scanKBColumn(int columnNumber, hal_gpio_level_t row[])
{
	QM_ASSERT(columnNumber < column_count);
	for (int i = 0; i < column_count; ++i)
		hal_gpio_set_output(column_pins[i], hgpioHigh);
	hal_gpio_set_output(column_pins[columnNumber], hgpioLow);
	hal_timer_delay(5);
	for (int i = 0; i < row_count; ++i)
		row[i] = hal_gpio_get_input(row_pins[i]);
}

void QmMatrixKeyboardPrivate::noPressesHandler(keyboard_state_t prev_keyboard_state, int8_t curKeysPressed[], int8_t prevKeysPressed[])
{
	switch (prev_keyboard_state) {
	case no_presses:
		return;
	case single_press:
		press_timer[0].stop();
		keyReleased(0, curKeysPressed[0]);
		break;
	case double_press:
		press_timer[0].stop();
		press_timer[1].stop();
		keyReleased(0, curKeysPressed[0]);
		keyReleased(1, curKeysPressed[1]);
		break;
	}
	prevKeysPressed[0] = -1;
	prevKeysPressed[1] = -1;
}

void QmMatrixKeyboardPrivate::singlePressHandler(keyboard_state_t prev_keyboard_state, int8_t curKeysPressed[], int8_t prevKeysPressed[])
{
	switch (prev_keyboard_state) {
	case no_presses:
		press_timer[0].start();
		curKeysPressedSequence[0] = curKeysPressed[0];
		keyPressed(curKeysPressed[0]);
		break;
	case single_press:
		return;
	case double_press: {
		int8_t releasedKeyCode = 0;
		if (prevKeysPressed[0] == curKeysPressed[0]) {
			releasedKeyCode = prevKeysPressed[1];
		} else if (prevKeysPressed[1] == curKeysPressed[0]) {
			releasedKeyCode = prevKeysPressed[0];
		}
		int8_t releasedKeyNumber = 0;
		if (releasedKeyCode != curKeysPressedSequence[0]) {
			releasedKeyNumber = 1;
		}
		press_timer[1].stop();
		keyReleased(releasedKeyNumber, releasedKeyCode);
		break;
	}
	}
	prevKeysPressed[0] = curKeysPressed[0];
}

void QmMatrixKeyboardPrivate::doublePressHandler(keyboard_state_t prev_keyboard_state, int8_t curKeysPressed[], int8_t prevKeysPressed[])
{
	switch (prev_keyboard_state) {
	case no_presses:
		press_timer[0].start();
		press_timer[1].start();
		curKeysPressedSequence[0] = curKeysPressed[0];
		curKeysPressedSequence[1] = curKeysPressed[1];
		keyPressed(curKeysPressed[0]);
		keyPressed(curKeysPressed[1]);
		break;
	case single_press: {
		press_timer[1].start();
		int8_t pressedKeyCode = 0;
		if (prevKeysPressed[0] == curKeysPressed[0]) {
			pressedKeyCode = curKeysPressed[1];
		} else if (prevKeysPressed[0] == curKeysPressed[1]) {
			pressedKeyCode = curKeysPressed[0];
		} else {
			QM_ASSERT(0);
		}
		curKeysPressedSequence[1] = pressedKeyCode;
		keyPressed(pressedKeyCode);
		break;
	}
	case double_press:
		return;
	}
	prevKeysPressed[0] = curKeysPressed[0];
	prevKeysPressed[1] = curKeysPressed[1];
}

void QmMatrixKeyboardPrivate::keyPressed(int8_t key_code)
{
	qmDebugMessage(QmDebug::Dump, "key pressed: id %d", key_code);
}

void QmMatrixKeyboardPrivate::keyReleased(int key_number, int8_t key_code)
{
	QmMatrixKeyboard::PressType press_type = QmMatrixKeyboard::PressSingle;
	if (keyPressedLong[key_number]) {
		keyPressedLong[key_number] = 0;
		press_type = QmMatrixKeyboard::PressLong;
	}
	qmDebugMessage(QmDebug::Dump, "key released: id %d, type %d", key_code, press_type);
}

bool QmMatrixKeyboard::event(QmEvent* event) {
	if (event->type() == QmEvent::None) {
		//TODO: ...
		return true;
	}
	return QmObject::event(event);
}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(QmMatrixKeyboard, LevelVerbose)
#include "qmdebug_domains_end.h"
