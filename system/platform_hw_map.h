/**
  ******************************************************************************
  * @file    platform_hw_map.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  * @brief   Определения назначения аппаратных ресурсов платформы
  *
  ******************************************************************************
  */

#ifndef PLATFORM_HW_MAP_H_
#define PLATFORM_HW_MAP_H_

enum platformhw_resource_t {
	platformhwInvalidResource = 0,
	platformhwHeadsetUart,				// UART RS-232 гарнитуры
	platformhwHeadsetPttIopin,			// I/O-пин тангенты гарнитуры
	platformhwDataFlashSpi,				// SPI-шина Data-Flash
	platformhwDataFlashCsPin,			// CS-пин Data-Flash
	platformhwMatrixKeyboard,			// матричная клавиатура
	platformhwKeyboardButt1Iopin,		// I/O-пин кнопки BUTT_1 двухкнопочной клавиатуры
	platformhwKeyboardButt2Iopin,		// I/O-пин кнопки BUTT_2 двухкнопочной клавиатуры
	platformhwKeyboardsLightIopin,		// I/O-пин управления подсветкой клавиатур
	platformhwEnRxRs232Iopin,			// I/O-пин входа EN_RX_232 трансивера последовательного порта
	platformhwEnTxRs232Iopin,			// I/O-пин входа EN_TX_232 трансивера последовательного порта
	platformhwDspUart,					// UART DSP
	platformhwDspResetIopin,			// I/O-пин сигнала RESET DSP
	platformhwAtuUart,					// UART АСУ
	platformhwBatterySmbusI2c,			// I2C шины SMBus аккумуляторной батареи
	platformhwNavigatorUart,			// UART ГНСС приемника
	platformhwNavigatorResetIopin,		// I/O-пин сигнала RESET ГНСС приемника
	platformhwNavigatorAntFlagIopin,	// I/O-пин сигнала ANT_FLAG ГНСС приемника
	platformhwPowerHSControlIopin,		// I/O-пин цепи вкл./выкл. радиостанции гартинуры
	platformhwPowerControllerIopin,		// I/O-пин управления контроллером кнопки вкл./выкл. питания
	platformhwPowerOffIntIopin,			// I/O-пин прерывания предупреждения о выключении питания
	platformhwPowerSourceIopin			// I/O-пин детектирования источника текущего питания (предварительный или основной)
};

enum platformhw_matrixkb_key_t {
#if defined(PORT__TARGET_DEVICE_REV1)
	platformhwKeyEnter		= 15,
	platformhwKeyBack		= 0,
	platformhwKeyUp			= 1,
	platformhwKeyDown		= 2,
	platformhwKeyLeft		= 3,
	platformhwKeyRight		= 7,
	platformhwKey0			= 11,
	platformhwKey1			= 4,
	platformhwKey2			= 8,
	platformhwKey3			= 12,
	platformhwKey4			= 5,
	platformhwKey5			= 9,
	platformhwKey6			= 13,
	platformhwKey7			= 6,
	platformhwKey8			= 10,
	platformhwKey9			= 14
#elif defined(PORT__STM3220G_EVAL)
	platformhwKeyEnter	= 3,
	platformhwKeyBack 	= 12,
	platformhwKeyUp 	= 13,
	platformhwKeyDown 	= 14,
	platformhwKeyLeft 	= 15,
	platformhwKeyRight 	= 11,
	platformhwKey0 		= 7,
	platformhwKey1 		= 8,
	platformhwKey2 		= 4,
	platformhwKey3 		= 0,
	platformhwKey4 		= 9,
	platformhwKey5 		= 5,
	platformhwKey6 		= 1,
	platformhwKey7 		= 10,
	platformhwKey8 		= 6,
	platformhwKey9 		= 2
#elif defined(PORT__PCSIMULATOR)
        platformhwKeyEnter      = 15,
        platformhwKeyBack       = 0,
        platformhwKeyUp         = 4,
        platformhwKeyDown       = 8,
        platformhwKeyLeft       = 12,
        platformhwKeyRight      = 13,
        platformhwKey0          = 14,
        platformhwKey1          = 1,
        platformhwKey2          = 2,
        platformhwKey3          = 3,
        platformhwKey4          = 5,
        platformhwKey5          = 6,
        platformhwKey6          = 7,
        platformhwKey7          = 9,
        platformhwKey8          = 10,
        platformhwKey9          = 11
#endif /* PORT__* */
};

#endif /* PLATFORM_HW_MAP_H_ */
