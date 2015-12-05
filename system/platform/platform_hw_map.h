/**
  ******************************************************************************
  * @file    platform_hw_map.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    23.10.2015
  * @brief   Определения назначения аппаратных ресурсов платформы
  *
  ******************************************************************************
  */

#ifndef PLATFORM_HW_MAP_H_
#define PLATFORM_HW_MAP_H_

enum platformhw_resource_t {
	platformhwInvalidResource = 0,
	platformhwKeyboardButt1Iopin,		// I/O-пин кнопки BUTT_1 двухкнопочной клавиатуры
	platformhwKeyboardButt2Iopin		// I/O-пин кнопки BUTT_2 двухкнопочной клавиатуры
};

#endif /* PLATFORM_HW_MAP_H_ */
