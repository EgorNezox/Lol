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
#include "qmapplication.h"

static void qmmatrixkeyboardPollTimerFinished(hal_timer_handle_t handle) {
	QmMatrixKeyboardPrivate* qmmatrixkeyboardPrivate = static_cast<QmMatrixKeyboardPrivate*>(hal_timer_get_userid(handle));
	qmmatrixkeyboardPrivate->scan();
	hal_timer_start(handle, KEYBOARD_POLLING_DELAY, NULL);
}

static void qmmatrixkeyboardPressTimer0Finished(hal_timer_handle_t handle) {
	QmMatrixKeyboardPrivate* qmmatrixkeyboardPrivate = static_cast<QmMatrixKeyboardPrivate*>(hal_timer_get_userid(handle));
	qmmatrixkeyboardPrivate->pressTimerFinished(0);
}

static void qmmatrixkeyboardPressTimer1Finished(hal_timer_handle_t handle) {
	QmMatrixKeyboardPrivate* qmmatrixkeyboardPrivate = static_cast<QmMatrixKeyboardPrivate*>(hal_timer_get_userid(handle));
	qmmatrixkeyboardPrivate->pressTimerFinished(1);
}

QmMatrixKeyboardKeyStateChangedEvent::QmMatrixKeyboardKeyStateChangedEvent(QmEvent::Type type, int key_id, bool state) :
	QmEvent(type), key_id(key_id), state(state)
{
}

int QmMatrixKeyboardKeyStateChangedEvent::getKeyId() {
	return key_id;
}

bool QmMatrixKeyboardKeyStateChangedEvent::getState() {
	return state;
}

QmMatrixKeyboardKeyActionEvent::QmMatrixKeyboardKeyActionEvent(QmEvent::Type type, int key_id, QmMatrixKeyboard::PressType pressType) :
	QmEvent(type), key_id(key_id), pressType(pressType)
{
}

int QmMatrixKeyboardKeyActionEvent::getKeyId() {
	return key_id;
}

QmMatrixKeyboard::PressType QmMatrixKeyboardKeyActionEvent::getPressType() {
	return pressType;
}

QmMatrixKeyboardPrivate::QmMatrixKeyboardPrivate(QmMatrixKeyboard *q) :
	QmObjectPrivate(q),
	hw_resource(-1), column_pins(NULL), row_pins(NULL), column_count(0), row_count(0)
{
	hal_timer_params_t params;
	params.userid = static_cast<void*>(this);
	params.callbackTimeout = qmmatrixkeyboardPollTimerFinished;
	poll_timer = hal_timer_create(&params);

	press_timer = new hal_timer_handle_t[KEYBOARD_MAX_PRESSES];
	params.callbackTimeout = qmmatrixkeyboardPressTimer0Finished;
	press_timer[0] = hal_timer_create(&params);
	params.callbackTimeout = qmmatrixkeyboardPressTimer1Finished;
	press_timer[1] = hal_timer_create(&params);

	keyPressedLong = new bool[KEYBOARD_MAX_PRESSES];
	curKeysPressedSequence = new uint8_t[KEYBOARD_MAX_PRESSES];
	keyboard_state = no_presses;
	curKeysPressed = new uint8_t[KEYBOARD_MAX_PRESSES];
	prevKeysPressed = new uint8_t[KEYBOARD_MAX_PRESSES];
	pressesCounter = 0;
}

QmMatrixKeyboardPrivate::~QmMatrixKeyboardPrivate() {
	delete[] press_timer;
	delete[] keyPressedLong;
	delete[] curKeysPressedSequence;
	delete[] curKeysPressed;
	delete[] prevKeysPressed;
}

void QmMatrixKeyboardPrivate::init() {
	stm32f2_get_matrixkeyboard_pins(hw_resource, &column_pins, &column_count, &row_pins, &row_count);
	QM_ASSERT(column_count > 0 && column_pins != NULL);
	QM_ASSERT(row_count > 0 && row_pins != NULL);
	stm32f2_ext_pins_init(hw_resource);
	for (int i = 0; i < KEYBOARD_MAX_PRESSES; ++i) {
		keyPressedLong[i] = false;
		curKeysPressedSequence[i] = 0;
		curKeysPressed[i] = 0;
		prevKeysPressed[i] = 0;
	}
	hal_timer_start(poll_timer, KEYBOARD_POLLING_DELAY, NULL);
}

void QmMatrixKeyboardPrivate::deinit() {
	hal_timer_stop(poll_timer);
	hal_timer_stop(press_timer[0]);
	hal_timer_stop(press_timer[1]);
	stm32f2_ext_pins_deinit(hw_resource);
	delete[] column_pins;
	delete[] row_pins;
	column_count = 0;
	row_count = 0;
}

bool QmMatrixKeyboardPrivate::isKeyPressed(int id)
{
	if (!scanKBMatrix(KEYBOARD_MAX_PRESSES))
		return false;
	if (pressesCounter == 0)
		return false;
	for (int i = 0; i < pressesCounter; ++i) {
		if (curKeysPressed[i] == id)
			return true;
	}
	return false;
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
}

/*!
 * @brief ?????????????????? ?????????????? ???????????????????? ?? ?????????????????????????? ???????? ?????????????? ???????????? ?? ???? ????????????????????
 * @param maxPresses ???????????????????????? ???????????????????? ???????????????????????? ?????????????? ????????????
 * @retval true ?? ???????????? ?????????????????? ????????????????????????
 * 		   false ?? ???????????? ????????????
 **/
bool QmMatrixKeyboardPrivate::scanKBMatrix(uint8_t maxPresses) {
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
 * @brief ?????????????????? ?????????????? columnNumber ?? ???????????????????? ???????????? ???????????????? ?????????? ?????? ?????????? ??????????????
 * @param columnNumber ?????????? ??????????????
 * @param row ???????????? ???????????????? ?????????? ?????? ???????????????? ????????????????????
 **/
void QmMatrixKeyboardPrivate::scanKBColumn(int columnNumber, hal_gpio_level_t row[]) {
	QM_ASSERT(columnNumber < column_count);
	for (int i = 0; i < column_count; ++i) {
		hal_gpio_set_output(column_pins[i], hgpioHigh);
	}
	hal_gpio_set_output(column_pins[columnNumber], hgpioLow);
	hal_timer_delay(1); //???????????????? ?????????????????? ?????????????? ???? ??????????
	for (int i = 0; i < row_count; ++i) {
		row[i] = hal_gpio_get_input(row_pins[i]);
	}
}

void QmMatrixKeyboardPrivate::noPressesHandler(keyboard_state_t prev_keyboard_state) {
	switch (prev_keyboard_state) {
	case no_presses:
		return;
	case single_press:
		keyReleased(0, prevKeysPressed[0]);
		break;
	case double_press:
		keyReleased(0, prevKeysPressed[0]);
		keyReleased(1, prevKeysPressed[1]);
		break;
	}
}

void QmMatrixKeyboardPrivate::singlePressHandler(keyboard_state_t prev_keyboard_state) {
	switch (prev_keyboard_state) {
	case single_press:
		if (prevKeysPressed[0] == curKeysPressed[0]) {
			break;
		}
		keyReleased(0, prevKeysPressed[0]);
//		break;
	case no_presses:
		curKeysPressedSequence[0] = curKeysPressed[0];
		keyPressed(0, curKeysPressed[0]);
		break;
	case double_press: {
		int8_t releasedKeyCode = 0;
		if (prevKeysPressed[0] == curKeysPressed[0]) {
			releasedKeyCode = prevKeysPressed[1];
		} else if (prevKeysPressed[1] == curKeysPressed[0]) {
			releasedKeyCode = prevKeysPressed[0];
		} else {
			keyReleased(0, prevKeysPressed[0]);
			keyReleased(1, prevKeysPressed[1]);
			curKeysPressedSequence[0] = curKeysPressed[0];
			keyPressed(0, curKeysPressed[0]);
			break;
		}
		int8_t releasedKeyNumber = 0;
		if (releasedKeyCode == curKeysPressedSequence[0]) {
			curKeysPressedSequence[0] = curKeysPressed[0];
		} else {
			releasedKeyNumber = 1;
		}
		keyReleased(releasedKeyNumber, releasedKeyCode);
		if (releasedKeyNumber == 0 && keyPressedLong[1]) {
			keyPressedLong[0] = true;
			keyPressedLong[1] = false;
		}
		break;
	}
	}
	prevKeysPressed[0] = curKeysPressed[0];
}

void QmMatrixKeyboardPrivate::doublePressHandler(keyboard_state_t prev_keyboard_state) {
	switch (prev_keyboard_state) {
	case no_presses:
		doublePress();
		break;
	case single_press:
		if (prevKeysPressed[0] == curKeysPressed[0]) {
			curKeysPressedSequence[1] = curKeysPressed[1];
			keyPressed(1, curKeysPressed[1]);
		} else if (prevKeysPressed[0] == curKeysPressed[1]) {
			curKeysPressedSequence[1] = curKeysPressed[0];
			keyPressed(1, curKeysPressed[0]);
		} else {
			keyReleased(0, prevKeysPressed[0]);
			doublePress();
			break;
		}
		break;
	case double_press:
		if (prevKeysPressed[0] == curKeysPressed[0] && prevKeysPressed[1] == curKeysPressed[1]) {
			break;
		} else if (prevKeysPressed[0] != curKeysPressed[0] && prevKeysPressed[1] != curKeysPressed[1]) {
			keyReleased(0, prevKeysPressed[0]);
			keyReleased(1, prevKeysPressed[1]);
			doublePress();
		} else if (prevKeysPressed[0] != curKeysPressed[0] && prevKeysPressed[1] == curKeysPressed[1]) {
			keyReleased(0, prevKeysPressed[0]);
			curKeysPressedSequence[0] = curKeysPressedSequence[1];
			curKeysPressedSequence[1] = curKeysPressed[0];
			keyPressed(0, curKeysPressed[0]);
		} else if (prevKeysPressed[0] == curKeysPressed[0] && prevKeysPressed[1] != curKeysPressed[1]) {
			keyReleased(1, prevKeysPressed[1]);
			curKeysPressedSequence[1] = curKeysPressed[1];
			keyPressed(1, curKeysPressed[1]);
		}
		break;
	}
	prevKeysPressed[0] = curKeysPressed[0];
	prevKeysPressed[1] = curKeysPressed[1];
}

void QmMatrixKeyboardPrivate::doublePress()
{
	curKeysPressedSequence[0] = curKeysPressed[0];
	curKeysPressedSequence[1] = curKeysPressed[1];
	keyPressed(0, curKeysPressed[0]);
	keyPressed(1, curKeysPressed[1]);
}

void QmMatrixKeyboardPrivate::keyPressed(int number, uint8_t code) {
	QM_ASSERT(number >= 0 && number < KEYBOARD_MAX_PRESSES);
	pressTimerStart(number);
	QM_Q(QmMatrixKeyboard);
	QmApplication::postEvent(q, new QmMatrixKeyboardKeyStateChangedEvent(QmEvent::KeyStateChanged, code, true));
}

void QmMatrixKeyboardPrivate::keyReleased(int number, uint8_t code) {
	QM_Q(QmMatrixKeyboard);
	QM_ASSERT(number >= 0 && number < KEYBOARD_MAX_PRESSES);
	pressTimerStop(number);
	QmApplication::postEvent(q, new QmMatrixKeyboardKeyStateChangedEvent(QmEvent::KeyStateChanged, code, false));
	if (keyPressedLong[number]) {
		keyPressedLong[number] = 0;
	} else {
		QmApplication::postEvent(q, new QmMatrixKeyboardKeyActionEvent(QmEvent::KeyAction, code, QmMatrixKeyboard::PressSingle));
	}
}

void QmMatrixKeyboardPrivate::pressTimerStart(int number) {
	QM_ASSERT(number >= 0 && number < KEYBOARD_MAX_PRESSES);
	hal_timer_start(press_timer[number], KEYPRESS_LONG_TIME, NULL);
}

void QmMatrixKeyboardPrivate::pressTimerStop(int number) {
	QM_ASSERT(number >= 0 && number < KEYBOARD_MAX_PRESSES);
	hal_timer_stop(press_timer[number]);
}

void QmMatrixKeyboardPrivate::pressTimerFinished(int number) {
	QM_Q(QmMatrixKeyboard);
	QM_ASSERT(number >= 0 && number < KEYBOARD_MAX_PRESSES);
	keyPressedLong[number] = 1;
	QmApplication::postEvent(q, new QmMatrixKeyboardKeyActionEvent(
			QmEvent::KeyAction, curKeysPressedSequence[number], QmMatrixKeyboard::PressLong));
}

bool QmMatrixKeyboard::event(QmEvent* event) {
	switch (event->type()) {
	case QmEvent::KeyStateChanged: {
		QmMatrixKeyboardKeyStateChangedEvent* e = static_cast<QmMatrixKeyboardKeyStateChangedEvent*>(event);
		keyStateChanged(e->getKeyId(), e->getState());
		qmDebugMessage(QmDebug::Dump, "keyStateChanged %d %d", e->getKeyId(), e->getState());
		return true;
	}
	case QmEvent::KeyAction: {
		QmMatrixKeyboardKeyActionEvent* e = static_cast<QmMatrixKeyboardKeyActionEvent*>(event);
		keyAction(e->getKeyId(), e->getPressType());
		qmDebugMessage(QmDebug::Dump, "keyAction %d %d", e->getKeyId(), e->getPressType());
		return true;
	}
	default:
		return QmObject::event(event);
	}
}

int QmMatrixKeyboard::keysNumber(int hw_resource) {
	int column_count = 0;
	int row_count = 0;
	stm32f2_get_matrixkeyboard_pins(hw_resource, NULL, &column_count, NULL, &row_count);
	return column_count * row_count;
}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(QmMatrixKeyboard, LevelDefault)
#include "qmdebug_domains_end.h"
