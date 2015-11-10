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
	platformhwDataFlashSpi,				// SPI доступа Data-Flash
	platformhwMatrixKeyboard,			// матричная клавиатура
	platformhwKeyboardButt1Iopin,		// I/O-пин кнопки BUTT_1 двухкнопочной клавиатуры
	platformhwKeyboardButt2Iopin,		// I/O-пин кнопки BUTT_2 двухкнопочной клавиатуры
	platformhwKeyboardsLightIopin,		// I/O-пин управления подсветкой клавиатур
	platformhwEnRxRs232Iopin,			// I/O-пин входа EN_RX_232 трансивера последовательного порта
	platformhwEnTxRs232Iopin,			// I/O-пин входа EN_TX_232 трансивера последовательного порта
	platformhwDspUart,					// UART DSP
	platformhwDspResetIopin,			// I/O-пин сигнала RESET DSP
	platformhwAtuUart,					// UART АСУ
	platformhwBatterySmbusI2c			// I2C шины SMBus аккумуляторной батареи
};

enum platformhw_matrixkb_key_t {
#if defined(PORT__TARGET_DEVICE_REV1) //TODO: assign values for target-device-rev1 keys
	platformhwKeyEnter,
	platformhwKeyBack,
	platformhwKeyUp,
	platformhwKeyDown,
	platformhwKeyLeft,
	platformhwKeyRight,
	platformhwKey0,
	platformhwKey1,
	platformhwKey2,
	platformhwKey3,
	platformhwKey4,
	platformhwKey5,
	platformhwKey6,
	platformhwKey7,
	platformhwKey8,
	platformhwKey9,
#elif defined(PORT__STM3220G_EVAL) //TODO: assign values for stm3220g-eval keys
	platformhwKeyEnter,
	platformhwKeyBack,
	platformhwKeyUp,
	platformhwKeyDown,
	platformhwKeyLeft,
	platformhwKeyRight,
	platformhwKey0,
	platformhwKey1,
	platformhwKey2,
	platformhwKey3,
	platformhwKey4,
	platformhwKey5,
	platformhwKey6,
	platformhwKey7,
	platformhwKey8,
	platformhwKey9,
#elif defined(PORT__PCSIMULATOR) //TODO: assign values for pc-simulator keys
	platformhwKeyEnter,
	platformhwKeyBack,
	platformhwKeyUp,
	platformhwKeyDown,
	platformhwKeyLeft,
	platformhwKeyRight,
	platformhwKey0,
	platformhwKey1,
	platformhwKey2,
	platformhwKey3,
	platformhwKey4,
	platformhwKey5,
	platformhwKey6,
	platformhwKey7,
	platformhwKey8,
	platformhwKey9,
#endif /* PORT__* */
};

#endif /* PLATFORM_HW_MAP_H_ */