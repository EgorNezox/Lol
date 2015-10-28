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

typedef enum {
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
} platformhw_resource_t;

#endif /* PLATFORM_HW_MAP_H_ */
