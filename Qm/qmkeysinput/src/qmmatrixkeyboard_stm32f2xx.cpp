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
	press_timer[0].timeout.connect(sigc::mem_fun(this, &QmMatrixKeyboardPrivate::timer0Finished));
	press_timer[1].timeout.connect(sigc::mem_fun(this, &QmMatrixKeyboardPrivate::timer1Finished));

	keyPressedLong = new bool[KEYBOARD_MAX_PRESSES];
	longPressAction = new bool[KEYBOARD_MAX_PRESSES];
	curKeysPressedSequence = new uint8_t[KEYBOARD_MAX_PRESSES];
	keyboard_state = no_presses;
	curKeysPressed = new int8_t[KEYBOARD_MAX_PRESSES];
	prevKeysPressed = new int8_t[KEYBOARD_MAX_PRESSES];
	pressesCounter = 0;
}

QmMatrixKeyboardPrivate::~QmMatrixKeyboardPrivate()
{
	delete poll_timer;
	delete[] press_timer;
	delete[] keyPressedLong;
	delete[] longPressAction;
	delete[] curKeysPressedSequence;
	delete[] curKeysPressed;
	delete[] prevKeysPressed;
}

void QmMatrixKeyboardPrivate::init() {
	stm32f2_get_matrixkeyboard_pins(hw_resource, &column_pins, &column_count, &row_pins, &row_count);
	stm32f2_ext_pins_init(hw_resource);

	for (int i = 0; i < KEYBOARD_MAX_PRESSES; ++i) {
		keyPressedLong[i] = false;
		longPressAction[i] = false;
		curKeysPressedSequence[i] = 0;
		curKeysPressed[i] = 0;
		prevKeysPressed[i] = 0;
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
	if (scanKBMatrix(KEYBOARD_MAX_PRESSES)) {
		switch (pressesCounter) {
		case 0:
			noPressesHandler(keyboard_state);
			keyboard_state = no_presses;
			break;
		case 1:
			singlePressHandler(keyboard_state);
			keyboard_state = single_press;
			break;
		case 2:
			doublePressHandler(keyboard_state);
			keyboard_state = double_press;
			break;
		default:
			QM_ASSERT(0);
		}
	}

	for (int i = 0; i < KEYBOARD_MAX_PRESSES; ++i) {
		if (longPressAction[i]) {
			longPressAction[i] = 0;
			//TODO: emit keyAction
			qmDebugMessage(QmDebug::Dump, "keyAction %d %d", curKeysPressedSequence[i], QmMatrixKeyboard::PressLong);
		}
	}
}

/*!
 * @brief Сканирует матрицу клавиатуры и устанавливает коды нажатых клавиш и их количество
 * @param maxPresses максимальное количество одновременно нажатых клавиш
 * @retval true в случае успешного сканирования
 * 		   false в случае ошибки
 **/
bool QmMatrixKeyboardPrivate::scanKBMatrix(uint8_t maxPresses)
{
	pressesCounter = 0;
	for (int col = 0; col < column_count; ++col) {
		hal_gpio_level_t matrix[row_count];
		scanKBColumn(col, matrix);
		for (int row = 0; row < row_count; ++row) {
			if (hgpioLow == matrix[row]) {
				if (pressesCounter >= maxPresses) {
					return false;
				}
				curKeysPressed[pressesCounter] = row * row_count + col;
				++pressesCounter;
			}
		}
	}
	return true;
}

/*!
 * @brief Сканирует столбец columnNumber и возвращает массив значений строк для этого столбца
 * @param columnNumber номер столбца
 * @param row массив значений строк для возврата результата
 **/
void QmMatrixKeyboardPrivate::scanKBColumn(int columnNumber, hal_gpio_level_t row[])
{
	QM_ASSERT(columnNumber < column_count);
	for (int i = 0; i < column_count; ++i) {
		hal_gpio_set_output(column_pins[i], hgpioHigh);
	}
	hal_gpio_set_output(column_pins[columnNumber], hgpioLow);
	hal_timer_delay(5);
	for (int i = 0; i < row_count; ++i) {
		row[i] = hal_gpio_get_input(row_pins[i]);
	}
}

void QmMatrixKeyboardPrivate::noPressesHandler(keyboard_state_t prev_keyboard_state)
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

void QmMatrixKeyboardPrivate::singlePressHandler(keyboard_state_t prev_keyboard_state)
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

void QmMatrixKeyboardPrivate::doublePressHandler(keyboard_state_t prev_keyboard_state)
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
	//TODO: emit keyStateChanged
	qmDebugMessage(QmDebug::Dump, "keyStateChanged %d %d", key_code, 1);
}

void QmMatrixKeyboardPrivate::keyReleased(int key_number, int8_t key_code)
{
	//TODO: emit keyStateChanged
	qmDebugMessage(QmDebug::Dump, "keyStateChanged %d %d", key_code, 0);
	if (keyPressedLong[key_number]) {
		keyPressedLong[key_number] = 0;
	} else {
		//TODO: emit keyAction
		qmDebugMessage(QmDebug::Dump, "keyAction %d %d", key_code, QmMatrixKeyboard::PressSingle);
	}
}

void QmMatrixKeyboardPrivate::timer0Finished()
{
	keyPressedLong[0] = 1;
	longPressAction[0] = 1;
}

void QmMatrixKeyboardPrivate::timer1Finished()
{
	keyPressedLong[1] = 1;
	longPressAction[1] = 1;
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
