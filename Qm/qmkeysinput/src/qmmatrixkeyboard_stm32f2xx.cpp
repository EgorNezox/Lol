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

static void qmmatrixkeyboardPollTimerFinished(xTimerHandle xTimer) {
	QmMatrixKeyboardPrivate* qmmatrixkeyboardPrivate = static_cast<QmMatrixKeyboardPrivate*>(pvTimerGetTimerID(xTimer));
	qmmatrixkeyboardPrivate->scan();
	qmmatrixkeyboardPrivate->pollTimerStart();
}

static void qmmatrixkeyboardPressTimer0Finished(xTimerHandle xTimer) {
	QmMatrixKeyboardPrivate* qmmatrixkeyboardPrivate = static_cast<QmMatrixKeyboardPrivate*>(pvTimerGetTimerID(xTimer));
	qmmatrixkeyboardPrivate->pressTimerFinished(0);
}

static void qmmatrixkeyboardPressTimer1Finished(xTimerHandle xTimer) {
	QmMatrixKeyboardPrivate* qmmatrixkeyboardPrivate = static_cast<QmMatrixKeyboardPrivate*>(pvTimerGetTimerID(xTimer));
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
	hw_resource(-1), column_pins(NULL), row_pins(NULL)
{
	poll_timer = new xTimerHandle;
	*poll_timer = xTimerCreate(static_cast<const char*>("QmMatrixKeyboardPollTimer"),
			KEYBOARD_POLLING_DELAY / portTICK_RATE_MS, pdFALSE, static_cast<void*>(this), qmmatrixkeyboardPollTimerFinished);

	press_timer = new xTimerHandle[KEYBOARD_MAX_PRESSES];
	press_timer[0] = xTimerCreate(static_cast<const char*>("QmMatrixKeyboardPressTimer0"),
				KEYBOARD_POLLING_DELAY / portTICK_RATE_MS, pdFALSE, static_cast<void*>(this), qmmatrixkeyboardPressTimer0Finished);
	press_timer[1] = xTimerCreate(static_cast<const char*>("QmMatrixKeyboardPressTimer1"),
				KEYBOARD_POLLING_DELAY / portTICK_RATE_MS, pdFALSE, static_cast<void*>(this), qmmatrixkeyboardPressTimer1Finished);

	keyPressedLong = new bool[KEYBOARD_MAX_PRESSES];
	curKeysPressedSequence = new uint8_t[KEYBOARD_MAX_PRESSES];
	keyboard_state = no_presses;
	curKeysPressed = new uint8_t[KEYBOARD_MAX_PRESSES];
	prevKeysPressed = new uint8_t[KEYBOARD_MAX_PRESSES];
	pressesCounter = 0;
}

QmMatrixKeyboardPrivate::~QmMatrixKeyboardPrivate() {
	delete poll_timer;
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
	xTimerStart(*poll_timer, 0);
}

void QmMatrixKeyboardPrivate::deinit() {
	xTimerStop(*poll_timer, 0);
	xTimerStop(press_timer[0], 0);
	xTimerStop(press_timer[0], 0);
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
}

/*!
 * @brief Сканирует матрицу клавиатуры и устанавливает коды нажатых клавиш и их количество
 * @param maxPresses максимальное количество одновременно нажатых клавиш
 * @retval true в случае успешного сканирования
 * 		   false в случае ошибки
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
 * @brief Сканирует столбец columnNumber и возвращает массив значений строк для этого столбца
 * @param columnNumber номер столбца
 * @param row массив значений строк для возврата результата
 **/
void QmMatrixKeyboardPrivate::scanKBColumn(int columnNumber, hal_gpio_level_t row[]) {
	QM_ASSERT(columnNumber < column_count);
	for (int i = 0; i < column_count; ++i) {
		hal_gpio_set_output(column_pins[i], hgpioHigh);
	}
	hal_gpio_set_output(column_pins[columnNumber], hgpioLow);
	hal_timer_delay(1); //ожидание установки уровней на пинах
	for (int i = 0; i < row_count; ++i) {
		row[i] = hal_gpio_get_input(row_pins[i]);
	}
}

void QmMatrixKeyboardPrivate::noPressesHandler(keyboard_state_t prev_keyboard_state) {
	switch (prev_keyboard_state) {
	case no_presses:
		return;
	case single_press:
		pressTimerStop(0);
		keyReleased(0, curKeysPressed[0]);
		break;
	case double_press:
		pressTimerStop(0);
		pressTimerStop(1);
		keyReleased(0, curKeysPressed[0]);
		keyReleased(1, curKeysPressed[1]);
		break;
	}
	prevKeysPressed[0] = -1;
	prevKeysPressed[1] = -1;
}

void QmMatrixKeyboardPrivate::singlePressHandler(keyboard_state_t prev_keyboard_state) {
	switch (prev_keyboard_state) {
	case no_presses:
		pressTimerStart(0);
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
		pressTimerStop(0);
		keyReleased(releasedKeyNumber, releasedKeyCode);
		break;
	}
	}
	prevKeysPressed[0] = curKeysPressed[0];
}

void QmMatrixKeyboardPrivate::doublePressHandler(keyboard_state_t prev_keyboard_state) {
	switch (prev_keyboard_state) {
	case no_presses:
		pressTimerStart(0);
		pressTimerStart(1);
		curKeysPressedSequence[0] = curKeysPressed[0];
		curKeysPressedSequence[1] = curKeysPressed[1];
		keyPressed(curKeysPressed[0]);
		keyPressed(curKeysPressed[1]);
		break;
	case single_press: {
		pressTimerStart(1);
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

void QmMatrixKeyboardPrivate::keyPressed(uint8_t key_code) {
	QM_Q(QmMatrixKeyboard);
	QmApplication::postEvent(q, new QmMatrixKeyboardKeyStateChangedEvent(QmEvent::KeyStateChanged, key_code, true));
	qmDebugMessage(QmDebug::Dump, "keyStateChanged %d %d", key_code, 1);
}

void QmMatrixKeyboardPrivate::keyReleased(int key_number, uint8_t key_code) {
	QM_Q(QmMatrixKeyboard);
	QM_ASSERT(key_number < KEYBOARD_MAX_PRESSES);
	QmApplication::postEvent(q, new QmMatrixKeyboardKeyStateChangedEvent(QmEvent::KeyStateChanged, key_code, false));
	qmDebugMessage(QmDebug::Dump, "keyStateChanged %d %d", key_code, 0);
	if (keyPressedLong[key_number]) {
		keyPressedLong[key_number] = 0;
	} else {
		QmApplication::postEvent(q, new QmMatrixKeyboardKeyActionEvent(QmEvent::KeyStateChanged, key_code, QmMatrixKeyboard::PressSingle));
		qmDebugMessage(QmDebug::Dump, "keyAction %d %d", key_code, QmMatrixKeyboard::PressSingle);
	}
}

void QmMatrixKeyboardPrivate::pollTimerStart() {
	xTimerStart(*poll_timer, 0);
}

void QmMatrixKeyboardPrivate::pressTimerStart(int number) {
	QM_ASSERT(number < KEYBOARD_MAX_PRESSES);
	xTimerStart(press_timer[number], 0);
}

void QmMatrixKeyboardPrivate::pressTimerStop(int number) {
	QM_ASSERT(number < KEYBOARD_MAX_PRESSES);
	xTimerStop(press_timer[number], 0);
}

void QmMatrixKeyboardPrivate::pressTimerFinished(int number) {
	QM_ASSERT(number < KEYBOARD_MAX_PRESSES);
	keyPressedLong[number] = 1;
	QmApplication::postEvent(q, new QmMatrixKeyboardKeyActionEvent(QmEvent::KeyStateChanged, key_code, QmMatrixKeyboard::PressLong));
	qmDebugMessage(QmDebug::Dump, "keyAction %d %d", curKeysPressedSequence[number], QmMatrixKeyboard::PressLong);
}

bool QmMatrixKeyboard::event(QmEvent* event) {
	switch (event->type()) {
	case QmEvent::KeyStateChanged: {
		QmMatrixKeyboardKeyStateChangedEvent* e = static_cast<QmMatrixKeyboardKeyStateChangedEvent*>(event);
		keyStateChanged(e->getKeyId(), e->getState());
		return true;
	}
	case QmEvent::KeyAction: {
		QmMatrixKeyboardKeyActionEvent* e = static_cast<QmMatrixKeyboardKeyActionEvent*>(event);
		keyAction(e->getKeyId(), e->getPressType());
		return true;
	}
	default:
		return QmObject::event(event);
	}
}

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(QmMatrixKeyboard, LevelVerbose)
#include "qmdebug_domains_end.h"
