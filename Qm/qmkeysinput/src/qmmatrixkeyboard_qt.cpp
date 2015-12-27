/**
  ******************************************************************************
  * @file    qmmatrixkeyboard_qt.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    28.10.2015
  *
  ******************************************************************************
  */

#include <QDebug>
#include "qmmatrixkeyboard.h"
#include "qmmatrixkeyboard_p.h"

#define KEYPRESS_LONG_TIME  1000 //ms

QmMatrixKeyboardPrivateAdapter::QmMatrixKeyboardPrivateAdapter(QmMatrixKeyboardPrivate *qmmatrixkeyboardprivate) :
    qmmatrixkeyboardprivate(qmmatrixkeyboardprivate)
{
    interface = MatrixKeyboardInterface::getInstance(qmmatrixkeyboardprivate->hw_resource);
    keys_number = interface->keysNumber();
    QObject::connect(interface, &MatrixKeyboardInterface::keyStateChanged, this, &QmMatrixKeyboardPrivateAdapter::processKeyStateChanged);
}

QmMatrixKeyboardPrivateAdapter::~QmMatrixKeyboardPrivateAdapter()
{
}

void QmMatrixKeyboardPrivateAdapter::processKeyStateChanged(int id, bool state) {
    Q_ASSERT(id < keys_number);
    qmmatrixkeyboardprivate->processKeyStateChanged(id, state);
}

KeyActionTimer::KeyActionTimer(QmMatrixKeyboardPrivate *qmmatrixkeyboardprivate, int id, int interval) :
    qmmatrixkeyboardprivate(qmmatrixkeyboardprivate),
    id(id)
{
    timer = new QTimer();
    timer->setSingleShot(true);
    timer->setInterval(interval);
    QObject::connect(timer, &QTimer::timeout, this, &KeyActionTimer::timeoutSlot);
}

KeyActionTimer::~KeyActionTimer() {
    delete timer;
}

void KeyActionTimer::start() {
    timer->start();
}

void KeyActionTimer::stop() {
    timer->stop();
}

bool KeyActionTimer::isActive() {
    return timer->isActive();
}

void KeyActionTimer::timeoutSlot() {
    qmmatrixkeyboardprivate->keyActionLong(id);
}

QmMatrixKeyboardPrivate::QmMatrixKeyboardPrivate(QmMatrixKeyboard *q) :
    QmObjectPrivate(q),
    hw_resource(-1), matrixkb_adapter(0), keys_count(0)
{
    timers = new QList<KeyActionTimer*>();
}

QmMatrixKeyboardPrivate::~QmMatrixKeyboardPrivate() {
    delete timers;
}

void QmMatrixKeyboardPrivate::init() {
    matrixkb_adapter = new QmMatrixKeyboardPrivateAdapter(this);
    keys_count = matrixkb_adapter->keys_number;
    for (int i = 0; i < keys_count; ++i)
        timers->append(new KeyActionTimer(this, i, KEYPRESS_LONG_TIME));
}

void QmMatrixKeyboardPrivate::deinit() {
    timers->clear();
    delete matrixkb_adapter;
}

bool QmMatrixKeyboardPrivate::isKeyPressed(int id) {
    Q_UNUSED(id);
    return false;
}

void QmMatrixKeyboardPrivate::processKeyStateChanged(int id, bool state) {
    QM_Q(QmMatrixKeyboard);
    if (state) {
        timers->at(id)->start();
    } else {
        if (timers->at(id)->isActive()) {
            timers->at(id)->stop();
            q->keyAction(id, QmMatrixKeyboard::PressSingle);
        }
    }
    q->keyStateChanged(id, state);
}

void QmMatrixKeyboardPrivate::keyActionLong(int id) {
    QM_Q(QmMatrixKeyboard);
    q->keyAction(id, QmMatrixKeyboard::PressLong);
}

bool QmMatrixKeyboard::event(QmEvent* event) {
	return QmObject::event(event);
}

int QmMatrixKeyboard::keysNumber(int hw_resource) {
	QM_UNUSED(hw_resource);
	return 0;
}
