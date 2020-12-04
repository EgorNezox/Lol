/**
  ******************************************************************************
  * @file    firmware_main.cpp
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

#include "app/multiradio.h"
#include "app/datastorage/fs.h"
#include "app/headset/controller.h"
#include "app/mrd/dispatcher.h"
#include "app/navigation/navigator.h"
#include "app/power/powercontroller.h"
#include "app/power/battery.h"
#include "app/ui/service.h"
#include "../system/init.h"
#include "app/usb/usbloader.h"

#define MIN_GENERATOR_FREQ		620
#define DEFAULT_GENERATOR_FREQ 	1900
#define MAX_GENERATOR_FREQ     	3100
#define FORMAT_FLASH            false

void qmMain() {
	QmApplication app;

#if defined (PORT__TARGET_DEVICE_REV1)

    QmSPIBus::enable(platformhwDataFlashSpi);
    QmM25PDevice::Config data_flash_config;

    data_flash_config.sector_size    = 64*1024;
    data_flash_config.sectors_count  = 32;
    data_flash_config.speed          = 75000000;
    data_flash_config.idle_clock_low = false;

    QmM25PDevice data_flash_device(data_flash_config, platformhwDataFlashSpi, platformhwDataFlashCsPin);
    QmSpiffs::Config data_fs_config;

    data_fs_config.device 			  = &data_flash_device;
    data_fs_config.physical_address   = 0;
    data_fs_config.physical_size      = 32*64*1024;
    data_fs_config.logical_block_size = 64*1024;
    data_fs_config.logical_page_size  = data_flash_device.getPageSize();
    data_fs_config.max_opened_files   = 10;


#if FORMAT_FLASH
    {
    	volatile bool do_format = true;
    	QM_DEBUG_BREAK;
    	if (do_format)
    		QmSpiffs::format(data_fs_config);
    }
#endif
    bool isMount = QmSpiffs::mount("data", data_fs_config);
#endif

    if (QmSpiffs::getErrorCode() == QmSpiffs::ERROR_NOT_FS)
    {
    	QmSpiffs::format(data_fs_config);
        isMount = QmSpiffs::mount("data", data_fs_config);
    }

    DataStorage::FS data_storage_fs("data");

    if (isMount != true)
    {
    	data_storage_fs.setBugDetect();
    }
    else
    {
        data_storage_fs.findFilesToFiletree();
    }


    //bool res = data_storage_fs.getDiagnsticInfo();

    // if mount not valid, flash is not work

#if defined (PORT__TARGET_DEVICE_REV1)
    target_device_multiradio_init(0);
#endif

    QmIopin reset_dsp(platformhwDspResetIopin);
    reset_dsp.writeOutput(QmIopin::Level_Low);
    QmThread::msleep(10);
    reset_dsp.writeOutput(QmIopin::Level_High);

#if defined(PORT__TARGET_DEVICE_REV1)
    Power::Controller power_controller(platformhwPowerHSControlIopin, platformhwPowerControllerIopin,platformhwPowerOffIntIopin, platformhwPowerSourceIopin);
#endif /* PORT__TARGET_DEVICE_REV1 */

    Power::Battery power_battery(platformhwBatterySmbusI2c);

    QmIopin enrxrs232_iopin(platformhwEnRxRs232Iopin);
    QmIopin entxrs232_iopin(platformhwEnTxRs232Iopin);
    Headset::Controller headset_controller(platformhwHeadsetUart, platformhwHeadsetPttIopin);

#if defined(PORT__TARGET_DEVICE_REV1)
    Navigation::Navigator navigator(platformhwNavigatorUart, platformhwNavigatorResetIopin,platformhwNavigatorAntFlagIopin, platformhwNavigator1PPSIopin);
    Multiradio::Dispatcher mr_dispatcher(platformhwDspUart, platformhwDspResetIopin, platformhwAtuUart, platformhwAtuIopin,&headset_controller, &navigator, &data_storage_fs, &power_battery);

    mr_dispatcher.setFlash(&data_flash_device);
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

    ui_matrixkb_desc.resource                            = platformhwMatrixKeyboard;
    ui_matrixkb_desc.key_id[platformhwKeyEnter]          = Ui::matrixkbkeyEnter;
    ui_matrixkb_desc.key_id[platformhwKeyBack]           = Ui::matrixkbkeyBack;
    ui_matrixkb_desc.key_id[platformhwKeyUp]             = Ui::matrixkbkeyUp;
    ui_matrixkb_desc.key_id[platformhwKeyDown]           = Ui::matrixkbkeyDown;
    ui_matrixkb_desc.key_id[platformhwKeyLeft]           = Ui::matrixkbkeyLeft;
    ui_matrixkb_desc.key_id[platformhwKeyRight]          = Ui::matrixkbkeyRight;
    ui_matrixkb_desc.key_id[platformhwKey0]              = Ui::matrixkbkey0;
    ui_matrixkb_desc.key_id[platformhwKey1]              = Ui::matrixkbkey1;
    ui_matrixkb_desc.key_id[platformhwKey2]              = Ui::matrixkbkey2;
    ui_matrixkb_desc.key_id[platformhwKey3]              = Ui::matrixkbkey3;
    ui_matrixkb_desc.key_id[platformhwKey4]              = Ui::matrixkbkey4;
    ui_matrixkb_desc.key_id[platformhwKey5]              = Ui::matrixkbkey5;
    ui_matrixkb_desc.key_id[platformhwKey6]              = Ui::matrixkbkey6;
    ui_matrixkb_desc.key_id[platformhwKey7]              = Ui::matrixkbkey7;
    ui_matrixkb_desc.key_id[platformhwKey8]              = Ui::matrixkbkey8;
    ui_matrixkb_desc.key_id[platformhwKey9]              = Ui::matrixkbkey9;
    ui_auxkb_desc.key_iopin_resource[Ui::auxkbkeyChNext] = platformhwKeyboardButt1Iopin;
    ui_auxkb_desc.key_iopin_resource[Ui::auxkbkeyChPrev] = platformhwKeyboardButt2Iopin;

#if defined(PORT__TARGET_DEVICE_REV1)
    Multiradio::usb_loader usb_class;
#endif

#ifdef PORT__TARGET_DEVICE_REV1
    Ui::Service ui_service( ui_matrixkb_desc, ui_auxkb_desc,&headset_controller,mr_dispatcher.getVoiceServiceInterface(),&power_battery,&navigator,&data_storage_fs, &usb_class);
#else
    Ui::Service ui_service(ui_matrixkb_desc,ui_auxkb_desc,&headset_controller,mr_dispatcher.getVoiceServiceInterface(),&power_battery,0,0,0);
    ui_service.setErrorCodeFs(QmSpiffs::getErrorCode());
#endif

    kb_light_iopin.writeOutput(QmIopin::Level_Low);

    Multiradio::voice_channels_table_t mr_channels_table;
    uint8_t count_channel;
	data_storage_fs.getVoiceChannelsTable(mr_channels_table, count_channel);

	if(QmSpiffs::getErrorCode() != 0)
	{
		ui_service.setNotification(Ui::NotificationErrorFlashMemmory);
	}

    if (mr_channels_table.empty() && QmSpiffs::getErrorCode() == 0)
		ui_service.setNotification(Ui::NotificationMissingVoiceChannelsTable);

    enrxrs232_iopin.writeOutput(QmIopin::Level_Low);
    entxrs232_iopin.writeOutput(QmIopin::Level_High);

	headset_controller.startServicing (mr_channels_table);
	mr_dispatcher.startServicing      (mr_channels_table);

#ifdef PORT__TARGET_DEVICE_REV1
    timer2_init();
#endif

#if defined(PORT__TARGET_DEVICE_REV1)
    navigator.setFlash(&data_storage_fs);
#endif

    //tune_frequency_generator(500, 1);
#if defined(PORT__TARGET_DEVICE_REV1)

    uint8_t is_usb_on = 0;
    data_storage_fs.getUsbOnOff(is_usb_on);

    if (is_usb_on)
    {
    	usb_class.startUsb();
    	usb_class.setfs(&data_storage_fs);
    }

#endif

    mr_dispatcher.startAtu();

	app.exec();
}
