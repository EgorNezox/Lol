/**
  ******************************************************************************
  * @file    firmware_main.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    18.08.2015
  * @brief   Загрузочный модуль приложения прошивки.
  *
  ******************************************************************************
 */

#include "../system/platform/platform_hw_map.h"

#include "qmapplication.h"
#include "qmiopin.h"

#include "multiradio.h"
#include "datastorage/fs.h"
#include "headset/controller.h"
#include "mrd/dispatcher.h"
#include "power/battery.h"
#include "ui/service.h"

void qmMain() {
	QmApplication app;

	Multiradio::voice_channels_table_t mr_channels_table;
	DataStorage::FS data_storage_fs(platformhwDataFlashSpi);
	QmIopin enrxrs232_iopin(platformhwEnRxRs232Iopin);
	QmIopin entxrs232_iopin(platformhwEnTxRs232Iopin);
	Headset::Controller headset_controller(platformhwHeadsetUart, platformhwHeadsetPttIopin);
	Multiradio::Dispatcher mr_dispatcher(platformhwDspUart, platformhwDspResetIopin, platformhwAtuUart,
			&headset_controller);
	Power::Battery power_battery(platformhwBatterySmbusI2c);
	Ui::matrix_keyboard_t ui_matrixkb_desc;
	Ui::aux_keyboard_t ui_auxkb_desc;
	QmIopin kb_light_iopin(platformhwKeyboardsLightIopin);
	ui_matrixkb_desc.resource = platformhwMatrixKeyboard;
	ui_matrixkb_desc.key_id[Ui::matrixkbkeyEnter] = platformhwKeyEnter;
	ui_matrixkb_desc.key_id[Ui::matrixkbkeyBack] = platformhwKeyBack;
	ui_matrixkb_desc.key_id[Ui::matrixkbkeyUp] = platformhwKeyUp;
	ui_matrixkb_desc.key_id[Ui::matrixkbkeyDown] = platformhwKeyDown;
	ui_matrixkb_desc.key_id[Ui::matrixkbkeyLeft] = platformhwKeyLeft;
	ui_matrixkb_desc.key_id[Ui::matrixkbkeyRight] = platformhwKeyRight;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey0] = platformhwKey0;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey1] = platformhwKey1;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey2] = platformhwKey2;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey3] = platformhwKey3;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey4] = platformhwKey4;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey5] = platformhwKey5;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey6] = platformhwKey6;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey7] = platformhwKey7;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey8] = platformhwKey8;
	ui_matrixkb_desc.key_id[Ui::matrixkbkey9] = platformhwKey9;
	ui_auxkb_desc.key_iopin_resource[Ui::auxkbkeyChNext] = platformhwKeyboardButt1Iopin;
	ui_auxkb_desc.key_iopin_resource[Ui::auxkbkeyChPrev] = platformhwKeyboardButt2Iopin;
	Ui::Service ui_service(ui_matrixkb_desc, ui_auxkb_desc,
			&headset_controller,
			mr_dispatcher.getMainServiceInterface(), mr_dispatcher.getVoiceServiceInterface(),
			&power_battery);

	kb_light_iopin.writeOutput(QmIopin::Level_Low);
	data_storage_fs.init();
	data_storage_fs.getVoiceChannelsTable(mr_channels_table);
	if (mr_channels_table.empty())
		ui_service.setNotification(Ui::NotificationMissingVoiceChannelsTable);
	enrxrs232_iopin.writeOutput(QmIopin::Level_High);
	entxrs232_iopin.writeOutput(QmIopin::Level_High);
	headset_controller.startServicing(mr_channels_table);
	mr_dispatcher.startServicing(mr_channels_table);

	app.exec();
}
