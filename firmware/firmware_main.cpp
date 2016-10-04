/**
  ******************************************************************************
  * @file    firmware_main.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @author  неизвестные
  * @date    18.08.2015
  * @brief   Загрузочный модуль приложения прошивки.
  *
  ******************************************************************************
 */

#include "../system/platform_hw_map.h"
#include "../system/init.h"

#include "qmdebug.h"
#include "qmapplication.h"
#include "qmiopin.h"
#include "qmspibus.h"
#include "qmspiffs.h"
#include "qmm25pdevice.h"

#include "multiradio.h"
#include "datastorage/fs.h"
#include "headset/controller.h"
#include "mrd/dispatcher.h"
#include "navigation/navigator.h"
#include "power/powercontroller.h"
#include "power/battery.h"
#include "ui/service.h"

void qmMain() {
	QmApplication app;

	target_device_multiradio_init();

#if defined(PORT__TARGET_DEVICE_REV1)
	Power::Controller power_controller(platformhwPowerHSControlIopin, platformhwPowerControllerIopin,
			platformhwPowerOffIntIopin, platformhwPowerSourceIopin);
#endif /* PORT__TARGET_DEVICE_REV1 */

	Power::Battery power_battery(platformhwBatterySmbusI2c);

	QmSPIBus::enable(platformhwDataFlashSpi);
	QmM25PDevice::Config data_flash_config;
	data_flash_config.sector_size = 64*1024;
	data_flash_config.sectors_count = 32;
	data_flash_config.speed = 75000000;
	data_flash_config.idle_clock_low = false;
	QmM25PDevice data_flash_device(data_flash_config, platformhwDataFlashSpi, platformhwDataFlashCsPin);
	QmSpiffs::Config data_fs_config;
	data_fs_config.device = &data_flash_device;
	data_fs_config.physical_address = 0;
	data_fs_config.physical_size = 4*64*1024;
	data_fs_config.logical_block_size = 64*1024;
	data_fs_config.logical_page_size = data_flash_device.getPageSize();
	data_fs_config.max_opened_files = 10;
#if 0
	{
		volatile bool do_format = false;
		QM_DEBUG_BREAK;
		if (do_format)
			QmSpiffs::format(data_fs_config);
	}
#endif
	QmSpiffs::mount("data", data_fs_config);

	Multiradio::voice_channels_table_t mr_channels_table;
	DataStorage::FS data_storage_fs("data");
	QmIopin enrxrs232_iopin(platformhwEnRxRs232Iopin);
	QmIopin entxrs232_iopin(platformhwEnTxRs232Iopin);
	Headset::Controller headset_controller(platformhwHeadsetUart, platformhwHeadsetPttIopin);
#if defined(PORT__TARGET_DEVICE_REV1)
    Navigation::Navigator navigator(platformhwNavigatorUart, platformhwNavigatorResetIopin,
    		platformhwNavigatorAntFlagIopin, platformhwNavigator1PPSIopin);
	Multiradio::Dispatcher mr_dispatcher(platformhwDspUart, platformhwDspResetIopin, platformhwAtuUart, platformhwAtuIopin,
            &headset_controller, &navigator, &data_storage_fs, &power_battery);
#else
    Multiradio::Dispatcher mr_dispatcher(platformhwDspUart, platformhwDspResetIopin, platformhwAtuUart, platformhwAtuIopin,
            &headset_controller, 0, &data_storage_fs, &power_battery);
#endif

	Ui::matrix_keyboard_t ui_matrixkb_desc;
	Ui::aux_keyboard_t ui_auxkb_desc;
	QmIopin kb_light_iopin(platformhwKeyboardsLightIopin);
#if !defined(PORT__PCSIMULATOR)
    QM_ASSERT(Ui::matrixkbKeysCount == QmMatrixKeyboard::keysNumber(platformhwMatrixKeyboard));
#endif

	ui_matrixkb_desc.resource = platformhwMatrixKeyboard;
	ui_matrixkb_desc.key_id[platformhwKeyEnter] = Ui::matrixkbkeyEnter;
	ui_matrixkb_desc.key_id[platformhwKeyBack] = Ui::matrixkbkeyBack;
	ui_matrixkb_desc.key_id[platformhwKeyUp] = Ui::matrixkbkeyUp;
	ui_matrixkb_desc.key_id[platformhwKeyDown] = Ui::matrixkbkeyDown;
	ui_matrixkb_desc.key_id[platformhwKeyLeft] = Ui::matrixkbkeyLeft;
	ui_matrixkb_desc.key_id[platformhwKeyRight] = Ui::matrixkbkeyRight;
	ui_matrixkb_desc.key_id[platformhwKey0] = Ui::matrixkbkey0;
	ui_matrixkb_desc.key_id[platformhwKey1] = Ui::matrixkbkey1;
	ui_matrixkb_desc.key_id[platformhwKey2] = Ui::matrixkbkey2;
	ui_matrixkb_desc.key_id[platformhwKey3] = Ui::matrixkbkey3;
	ui_matrixkb_desc.key_id[platformhwKey4] = Ui::matrixkbkey4;
	ui_matrixkb_desc.key_id[platformhwKey5] = Ui::matrixkbkey5;
	ui_matrixkb_desc.key_id[platformhwKey6] = Ui::matrixkbkey6;
	ui_matrixkb_desc.key_id[platformhwKey7] = Ui::matrixkbkey7;
	ui_matrixkb_desc.key_id[platformhwKey8] = Ui::matrixkbkey8;
	ui_matrixkb_desc.key_id[platformhwKey9] = Ui::matrixkbkey9;
	ui_auxkb_desc.key_iopin_resource[Ui::auxkbkeyChNext] = platformhwKeyboardButt1Iopin;
	ui_auxkb_desc.key_iopin_resource[Ui::auxkbkeyChPrev] = platformhwKeyboardButt2Iopin;


#ifdef PORT__TARGET_DEVICE_REV1
    Ui::Service ui_service( ui_matrixkb_desc, ui_auxkb_desc,
                           &headset_controller,
                            mr_dispatcher.getMainServiceInterface(),
                            mr_dispatcher.getVoiceServiceInterface(),
                           &power_battery,
                           &navigator,
                           &data_storage_fs
                          );
#else
    Ui::Service ui_service(
                             ui_matrixkb_desc,
                             ui_auxkb_desc,
                            &headset_controller,
                             mr_dispatcher.getMainServiceInterface(),
                             mr_dispatcher.getVoiceServiceInterface(),
                            &power_battery,
                             0,
                            &data_storage_fs
                          );
#endif


	kb_light_iopin.writeOutput(QmIopin::Level_Low);
	data_storage_fs.getVoiceChannelsTable(mr_channels_table);

    if (mr_channels_table.empty())
		ui_service.setNotification(Ui::NotificationMissingVoiceChannelsTable);

    enrxrs232_iopin.writeOutput(QmIopin::Level_Low);
	entxrs232_iopin.writeOutput(QmIopin::Level_High);
	headset_controller.startServicing(mr_channels_table);
	mr_dispatcher.startServicing(mr_channels_table);



	app.exec();
}
