/**
  ******************************************************************************
  * @file    gui_elements_common.h
  * @author  Egor Dudyak, PMR dept. software team, ONIIP, PJSC
  * @date    06 февр. 2015 г.
  * @brief   Типы используемые при отрисовке элементов GUI
  ******************************************************************************
  */
#ifndef GUI_ELEMENTS_COMMON_H_
#define GUI_ELEMENTS_COMMON_H_

#include "gdispcfg.h"

#define CONTENT_XE(content) (content.x+content.W-1)
#define CONTENT_YE(content) (content.y+content.H-1)
#define GEOM_W(area)	((area.xe-area.xs+1))
#define GEOM_H(area)	((area.ye-area.ys+1))

#define COMMON_ELEMENT_VP_MODE	(GNORMAL)

/*!Геометрическая область графического объекта*/
struct MoonsGeometry{
	GXT xs;
	GYT ys;
	GXT xe;
	GYT ye;
};

struct ContentSize{
	GXT x;
	GYT y;
	GXT W;
	GYT H;
};

// Цветовая схема
struct  ColorScheme{
	GCOLOR foreground;
	GCOLOR background;
};

// Поля
struct Margins{
	GXT Left;
	GXT Right;
	GYT Top;
	GYT Bottom;
};

// Выравнивание
enum  AlignmentH{
	alignLeft,
	alignRight,
	alignHCenter
};

enum AlignmentV{
	alignTop,
	alignBottom,
	alignVCenter
};

struct Alignment{
	AlignmentH align_h;
	AlignmentV align_v;
};

#endif /* GUI_ELEMENTS_COMMON_H_ */
