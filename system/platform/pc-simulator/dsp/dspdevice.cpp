/**
  ******************************************************************************
  * @file    dspdevice.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.12.2015
  * @brief   Реализация класса Qt, симулирующего устройство DSP
  *
  ******************************************************************************
  */

#include <QMetaObject>
#include <QtEndian>
#include "dspdevice.h"
#include "ui_dspdevice.h"
#include "dsptransport.h"

#define DEFAULT_PACKET_HEADER_LEN	3 // адрес + индикатор кадра + код параметра

namespace QtHwEmu {

void DspDevice::init() {
	qRegisterMetaType<Module>();
	qRegisterMetaType<ParameterValue>();
}

DspDevice::DspDevice(int uart_resource, int reset_iopin_resource, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::DspDevice)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Tool | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    transport = new DspTransport(uart_resource, this);
    QObject::connect(transport, &DspTransport::transferedTxFrame, this, &DspDevice::processTxFrame);
    reset_iopin = IopinInterface::createInstance(reset_iopin_resource);
    QObject::connect(reset_iopin, &IopinInterface::outputLevelChanged, this, &DspDevice::processResetLine);
    reset_timer.setSingleShot(true);
    QObject::connect(&reset_timer, &QTimer::timeout, this, &DspDevice::startup);
    state = (reset_iopin->getOutputLevel() == IopinInterface::Level_Low)?stateUnderReset:stateStarting;
    updateStateIndicator();
}

DspDevice::~DspDevice()
{
	IopinInterface::destroyInstance(reset_iopin);
    delete ui;
}

void DspDevice::processResetLine(IopinInterface::Level level) {
	switch (level) {
	case IopinInterface::Level_Low:
		state = stateUnderReset;
		reset_timer.stop();
		transport->reset();
		ui->labelRxRadiopathMode->setText("<unknown>");
		ui->labelRxRadiopathFrequency->setText("<unknown>");
		ui->labelTxRadiopathMode->setText("<unknown>");
		ui->labelTxRadiopathFrequency->setText("<unknown>");
		break;
	case IopinInterface::Level_High:
		if (state == stateUnderReset) {
			state = stateStarting;
			reset_timer.start(1000);
		}
		break;
	default: break;
	}
	updateStateIndicator();
}

void DspDevice::startup() {
	Q_ASSERT(state == stateStarting);
	state = stateOperating;
	updateRadiopathMode(ui->labelRxRadiopathMode, RadioModeOff);
	updateRadiopathFrequency(ui->labelRxRadiopathFrequency, 0);
	updateRadiopathMode(ui->labelTxRadiopathMode, RadioModeOff);
	updateRadiopathFrequency(ui->labelTxRadiopathFrequency, 0);
	sendDspInfo(0x5002, 0, 0);
	updateStateIndicator();
}

void DspDevice::sendDspInfo(uint16_t id, uint16_t major_version, uint16_t minor_version) {
	uint8_t data[DspTransport::MAX_FRAME_DATA_LEN];
	qToBigEndian((quint8)0, data+0); // адрес: 0
	qToBigEndian((quint8)5, data+1); // индикатор: "инициативное сообщение"
	qToBigEndian((quint8)2, data+2); // код параметра: "Цифровая информация о DSP прошивке"
	qToBigEndian(id, data+DEFAULT_PACKET_HEADER_LEN+0);
	qToBigEndian(major_version, data+DEFAULT_PACKET_HEADER_LEN+2);
	qToBigEndian(minor_version, data+DEFAULT_PACKET_HEADER_LEN+4);
	transport->transferRxFrame(0x10, data, (DEFAULT_PACKET_HEADER_LEN + 6));
}

void DspDevice::sendCommandResponse(bool success, Module module, int code, ParameterValue value) {
	uint8_t address = 0;
	uint8_t data[DspTransport::MAX_FRAME_DATA_LEN];
	int value_len = 0;
	switch (module) {
	case RxRadiopath: address = 0x51; break;
	case TxRadiopath: address = 0x81; break;
	default: Q_ASSERT(0);
	}
	qToBigEndian((quint8)0, data+0); // адрес: 0
	if (success)
		qToBigEndian((quint8)3, data+1); // индикатор: "команда выполнена"
	else
		qToBigEndian((quint8)4, data+1); // индикатор: "команда не выполнена"
	qToBigEndian((quint8)code, data+2); // код параметра
	switch (code) {
	case 1:
		qToBigEndian(value.frequency, data+DEFAULT_PACKET_HEADER_LEN+value_len);
		value_len += 4;
		break;
	case 2:
		qToBigEndian((quint8)value.radio_mode, data+DEFAULT_PACKET_HEADER_LEN+value_len);
		value_len += 1;
		break;
	default: Q_ASSERT(0);
	}
	transport->transferRxFrame(address, data, (DEFAULT_PACKET_HEADER_LEN + value_len));
}

void DspDevice::processCommandSetParameter(Module module, int code, ParameterValue value) {
	switch (module) {
	case RxRadiopath:
		switch (code) {
		case RxRadioMode:
			updateRadiopathMode(ui->labelRxRadiopathMode, value.radio_mode);
			break;
		case RxFrequency:
			updateRadiopathFrequency(ui->labelRxRadiopathFrequency, value.frequency);
			break;
		default: break;
		}
		break;
	case TxRadiopath:
		switch (code) {
		case TxRadioMode:
			updateRadiopathMode(ui->labelTxRadiopathMode, value.radio_mode);
			break;
		case TxFrequency:
			updateRadiopathFrequency(ui->labelTxRadiopathFrequency, value.frequency);
			break;
		default: break;
		}
		break;
	}
	// delayed response
	QMetaObject::invokeMethod(this, "sendCommandResponse", Qt::QueuedConnection,
			Q_ARG(bool, true),
			Q_ARG(QtHwEmu::DspDevice::Module, module),
			Q_ARG(int, code),
			Q_ARG(QtHwEmu::DspDevice::ParameterValue, value));
}

void DspDevice::processTxFrame(uint8_t address, uint8_t* data, int data_len) {
	if (state != stateOperating)
		return;
	if (data_len < 3)
		return;
	uint8_t indicator = qFromBigEndian<quint8>(data+1);;
	uint8_t code = qFromBigEndian<quint8>(data+2);;
	uint8_t *value_ptr = data + 3;
	int value_len = data_len - 3;
	switch (address) {
	case 0x50:
	case 0x80: {
		if (indicator == 2) { // "команда (установка)" ?
			ParameterValue value;
			if ((code == 1) && (value_len == 4)) {
				value.frequency = qFromBigEndian<quint32>(value_ptr+0);
			} else if ((code == 2) && (value_len == 1)) {
				value.radio_mode = (RadioMode)qFromBigEndian<quint8>(value_ptr+0);
			} else {
				break;
			}
			Module module;
			if (address == 0x50)
				module = RxRadiopath;
			else
				module = TxRadiopath;
			processCommandSetParameter(module, code, value);
		}
		break;
	}
	default: break;
	}
}

void DspDevice::updateStateIndicator() {
	switch (state) {
	case stateUnderReset: ui->labelState->setText("Under reset"); break;
	case stateStarting: ui->labelState->setText("Starting..."); break;
	case stateOperating: ui->labelState->setText("Operating"); break;
	}
}

void DspDevice::updateRadiopathMode(QLabel* label, RadioMode mode) {
	switch (mode) {
	case RadioModeOff: label->setText("Off"); break;
	case RadioModeCarrierTx: label->setText("Carrier Tx"); break;
	case RadioModeUSB: label->setText("USB"); break;
	case RadioModeFM: label->setText("FM"); break;
	case RadioModeSazhenData: label->setText("Sazhen Data"); break;
	}
}

void DspDevice::updateRadiopathFrequency(QLabel* label, uint32_t frequency) {
	label->setText(QString::number(frequency));
}

} /* namespace QtHwEmu */
