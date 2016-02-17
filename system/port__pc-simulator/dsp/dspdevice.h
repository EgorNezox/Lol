/**
  ******************************************************************************
  * @file    dspdevice.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.12.2015
  * @brief   Интерфейс класса Qt, симулирующего устройство DSP
  *
  ******************************************************************************
  */

#ifndef DSP_DSPDEVICE_H
#define DSP_DSPDEVICE_H

#include <qmetaobject.h>
#include <qframe.h>
#include <qtimer.h>
#include "port_hardwareio/iopininterface.h"

class QLabel;

namespace Ui {
class DspDevice;
}

namespace QtHwEmu {

class DspTransport;

class DspDevice : public QFrame
{
	Q_OBJECT

public:
	enum RadioMode {
		RadioModeOff = 0,
		RadioModeCarrierTx = 1,
		RadioModeUSB = 7,
		RadioModeFM = 9,
		RadioModeSazhenData = 11
	};

	enum Module {
		RxRadiopath,
		TxRadiopath
	};

	enum RxParameterCode {
		RxFrequency = 1,
		RxRadioMode = 2
	};

	enum TxParameterCode {
		TxFrequency = 1,
		TxRadioMode = 2
	};

	union ParameterValue {
		uint32_t frequency;
		RadioMode radio_mode;
	};

	DspDevice(int uart_resource, int reset_iopin_resource, QWidget *parent = 0);
	~DspDevice();

private Q_SLOTS:
	void processResetLine(IopinInterface::Level level);
	void startup();
	void sendDspInfo(uint16_t id, uint16_t major_version, uint16_t minor_version);
	void sendCommandResponse(bool success, QtHwEmu::DspDevice::Module module, int code, QtHwEmu::DspDevice::ParameterValue value);

private:
	static void __attribute__((constructor)) init();
	void processCommandSetParameter(Module module, int code, ParameterValue value);
	void processTxFrame(uint8_t address, uint8_t *data, int data_size);
	void updateStateIndicator();
	void updateRadiopathMode(QLabel *label, RadioMode mode);
	void updateRadiopathFrequency(QLabel *label, uint32_t frequency);

	Ui::DspDevice *ui;
	DspTransport *transport;
	IopinInterface *reset_iopin;
	QTimer reset_timer;
	enum {
		stateUnderReset,
		stateStarting,
		stateOperating
	} state;
};

} /* namespace QtHwEmu */

Q_DECLARE_METATYPE(QtHwEmu::DspDevice::Module)
Q_DECLARE_METATYPE(QtHwEmu::DspDevice::ParameterValue)

#endif // DSP_DSPDEVICE_H
