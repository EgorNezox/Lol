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

#include "../../qmcore/src/qmobject_p.h"
#include "qmmatrixkeyboard.h"

#ifdef QM_PLATFORM_STM32F2XX
#include "hal_gpio.h"
#include "hal_timer.h"
#include "qmevent.h"
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
#include <qobject.h>
#include <qlist.h>
#include <qtimer.h>
#include "port_keysinput/matrixkeyboardinterface.h"
#endif /* QM_PLATFORM_QT */

#ifdef QM_PLATFORM_STM32F2XX
class QmMatrixKeyboardKeyStateChangedEvent : public QmEvent
{
public:
	QmMatrixKeyboardKeyStateChangedEvent(QmEvent::Type type, int key_id, bool state);
	int getKeyId();
	bool getState();
private:
	int key_id;
	bool state;
};

class QmMatrixKeyboardKeyActionEvent : public QmEvent
{
public:
	QmMatrixKeyboardKeyActionEvent(QmEvent::Type type, int key_id, QmMatrixKeyboard::PressType pressType);
	int getKeyId();
	QmMatrixKeyboard::PressType getPressType();
private:
	int key_id;
	QmMatrixKeyboard::PressType pressType;
};
#endif /* QM_PLATFORM_STM32F2XX */

#ifdef QM_PLATFORM_QT
class QmMatrixKeyboardPrivateAdapter : public QObject
{
    Q_OBJECT
public:
    QmMatrixKeyboardPrivateAdapter(QmMatrixKeyboardPrivate *qmmatrixkeyboardprivate);
    ~QmMatrixKeyboardPrivateAdapter();
    QmMatrixKeyboardPrivate *qmmatrixkeyboardprivate;
    MatrixKeyboardInterface *interface;
    int keys_number;
public Q_SLOTS:
    void processKeyStateChanged(int id, bool state);
};

class KeyActionTimer : public QObject {
    Q_OBJECT
public:
    KeyActionTimer(QmMatrixKeyboardPrivate *qmmatrixkeyboardprivate, int id, int interval);
    ~KeyActionTimer();
    void start();
    void stop();
    bool isActive();
private Q_SLOTS:
    void timeoutSlot();
private:
    QmMatrixKeyboardPrivate *qmmatrixkeyboardprivate;
    int id;
    QTimer* timer;
};
#endif /* QM_PLATFORM_QT */

class QmMatrixKeyboardPrivate : public QmObjectPrivate {
	QM_DECLARE_PUBLIC(QmMatrixKeyboard)
public:
	QmMatrixKeyboardPrivate(QmMatrixKeyboard *q);
	virtual ~QmMatrixKeyboardPrivate();
#ifdef QM_PLATFORM_STM32F2XX
	void scan();
	void pressTimerFinished(int number);
#endif /* QM_PLATFORM_STM32F2XX */
private:
	void init();
	void deinit();
	bool isKeyPressed(int id);
	int hw_resource;
#ifdef QM_PLATFORM_STM32F2XX
	typedef enum keyboard_state {
		no_presses,
		single_press,
		double_press
	} keyboard_state_t;

	bool scanKBMatrix(uint8_t maxPresses);
	void scanKBColumn(int columnNumber, hal_gpio_level_t row[]);
	void noPressesHandler(keyboard_state_t prev_keyboard_state);
	void singlePressHandler(keyboard_state_t prev_keyboard_state);
	void doublePressHandler(keyboard_state_t prev_keyboard_state);
	void doublePress();
	void keyPressed(int number, uint8_t code);
	void keyReleased(int number, uint8_t code);
	void pressTimerStart(int number);
	void pressTimerStop(int number);

	hal_gpio_pin_t* column_pins;
	hal_gpio_pin_t* row_pins;
	int column_count;
	int row_count;
	hal_timer_handle_t poll_timer;
	hal_timer_handle_t* press_timer;

	bool* keyPressedLong; /*! ???????? ?????????????????????? ?????????????? ?????????????? (?????????????????????????????? ???? ?????????????????? ?????????????? ?????????????????????? ??????????????) */
	uint8_t* curKeysPressedSequence; /*! ???????????????????????????????????? ?????????????? ???????????? */
	keyboard_state_t keyboard_state;
	uint8_t* curKeysPressed;
	uint8_t* prevKeysPressed;
	uint8_t pressesCounter;
#endif /* QM_PLATFORM_STM32F2XX */
#ifdef QM_PLATFORM_QT
    void processKeyStateChanged(int id, bool state);
    void keyActionLong(int id);

    friend class QmMatrixKeyboardPrivateAdapter;
    QmMatrixKeyboardPrivateAdapter *matrixkb_adapter;

    int keys_count;
    friend class KeyActionTimer;
    QList<KeyActionTimer*> *timers;
#endif /* QM_PLATFORM_QT */
};

#endif /* QMMATRIXKEYBOARD_P_H_ */
