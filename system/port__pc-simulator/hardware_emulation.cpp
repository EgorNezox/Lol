/**
  ******************************************************************************
  * @file    hardware_emulation.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    17.02.2016
  *
  ******************************************************************************
  */

#include "qmessagebox.h"
#include "qfile.h"

#include "hardware_emulation.h"
#include "../platform_hw_map.h"
#include "../init.h"
#include "mainwidget.h"
#include "dsp/dspdevice.h"
#include "atu/atudevice.h"
#include "port_hardwareio/iopininterface.h"
#include "port_hardwareio/uartinterface.h"
#include "port_hardwareio/i2cbus.h"
#include "port_hardwareio/spibus.h"
#include "port_keysinput/pushbuttonkeyinterface.h"
#include "port_keysinput/matrixkeyboardinterface.h"
#include "port_rtc/rtc.h"
#include "devices/flashm25pdevice.h"

namespace QtHwEmu {

static MainWidget *main_widget = 0;
static DspDevice *dsp_device = 0;
static AtuDevice *atu_device = 0;
static I2CBus *battery_smbus = 0;
static SPIBus *data_flash_spibus = 0;
static FlashM25PDevice *data_flash_device = 0;
static const QString DATA_FLASH_DEVICE_IMAGE_PATH = "data-flash.bin";
static RTC *rtc;

void init() {
	main_widget = new MainWidget(platformhwMatrixKeyboard);
	main_widget->show();
	dsp_device = new DspDevice(platformhwDspUart, platformhwDspResetIopin);
	dsp_device->move(main_widget->frameGeometry().topRight() + QPoint(10,0));
	dsp_device->show();
	atu_device = new AtuDevice(platformhwAtuUart);
	atu_device->move(dsp_device->frameGeometry().topRight() + QPoint(10,0));
	atu_device->show();
	main_widget->activateWindow();
	main_widget->raise();
	battery_smbus = I2CBus::openInstance(platformhwBatterySmbusI2c);
	data_flash_spibus = SPIBus::openInstance(platformhwDataFlashSpi);
	FlashM25PDevice::Config data_flash_config;
	data_flash_config.memory_capacity_id = 0x15;
	data_flash_config.sector_size = 64*1024;
	data_flash_config.sectors_count = 32;
	QFile(DATA_FLASH_DEVICE_IMAGE_PATH).open(QFile::WriteOnly);
	data_flash_device = new FlashM25PDevice(DATA_FLASH_DEVICE_IMAGE_PATH, data_flash_config, platformhwDataFlashSpi, platformhwDataFlashCsPin);
	PushbuttonkeyInterface::createInstance(platformhwHeadsetPttIopin); // TODO: emulate HeadsetPttIopin
	IopinInterface::createInstance(platformhwKeyboardsLightIopin); // TODO: emulate KeyboardsLightIopin
	IopinInterface::createInstance(platformhwEnRxRs232Iopin); // TODO: emulate EnRxRs232Iopin
	IopinInterface::createInstance(platformhwEnTxRs232Iopin); // TODO: emulate EnTxRs232Iopin
	UartInterface::createInstance(platformhwHeadsetUart); //TODO: emulate HeadsetUart
	IopinInterface::createInstance(platformhwAtuIopin); // TODO: emulate AtuIopin
    //rtc = new RTC();
}

void deinit() {
	delete main_widget;
	delete dsp_device;
	delete atu_device;
	I2CBus::closeInstance(battery_smbus);
	delete data_flash_device;
	SPIBus::closeInstance(data_flash_spibus);
}

void show_message(const char* text) {
	QMessageBox::warning(0, "", QString(text));
}

} /* namespace QtHwEmu */

void target_device_multiradio_init(uint16_t freq) {}
