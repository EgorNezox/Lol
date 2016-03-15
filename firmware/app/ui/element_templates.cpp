/**
  ******************************************************************************
  * @file     element_templates.cpp
  * @author  Egor Dudyak, PMR dept. software team, ONIIP, PJSC
  * @date    08 окт. 2013 г.
  * @brief   Описание шаблонов параметров элементов
  ******************************************************************************
  */


//----------INCLUDES-----------

#include "elements.h"
#include "all_sym_indicators.h"

//----------DEFINES------------

//----------TYPES--------------

//----------GLOBAL_VARS--------

LabelParams GUI_EL_TEMP_LabelChannel{
	.element={{0,0,0,0},{alignHCenter, alignVCenter}},
    .font=&Consolas25x58,
	.color_sch={GENERAL_TEXT_COLOR,GENERAL_BACK_COLOR},
	.transparent=0
};

LabelParams GUI_EL_TEMP_LabelMode{
	.element={{2,2,1,1},{alignRight, alignTop}},
	.font=&Tahoma26,
	.color_sch={GENERAL_TEXT_COLOR,GENERAL_BACK_COLOR},
	.transparent=0
};

LabelParams GUI_EL_TEMP_LabelTitle{
	.element={{0,0,0,0},{alignHCenter, alignVCenter}},
	.font=&Tahoma_15x13,
	.color_sch={GENERAL_TEXT_COLOR,GENERAL_BACK_COLOR},
	.transparent=1
};

LabelParams GUI_EL_TEMP_LabelText{
	.element={{0,0,0,0},{alignLeft, alignTop}},
	.font=&Tahoma_15x13,
	.color_sch={GENERAL_TEXT_COLOR,GENERAL_BACK_COLOR},
	.transparent=1
};

LabelParams GUI_EL_TEMP_LabelInput{
	.element={{0,0,0,0},{alignLeft, alignTop}},
	.font=&Lucida_Console_8x12,
	.color_sch={GENERAL_TEXT_COLOR,GENERAL_BACK_COLOR},
	.transparent=1
};

LabelParams GUI_EL_TEMP_LabelCoords{
	.element={{0,0,0,0},{alignRight, alignTop}},
	.font=&Lucida_Console_8x12,
	.color_sch={GENERAL_TEXT_COLOR,GENERAL_BACK_COLOR},
	.transparent=1
};

LabelParams GUI_EL_TEMP_LabelSpBoxActive{
	.element={{0,0,2,0},{alignHCenter, alignVCenter}},
	.font=&Lucida_Console_8x12,
	.color_sch={SPBOX_ACTIVE_TEXT_COLOR,SPBOX_ACTIVE_BACK_COLOR},
	.transparent=0
};

LabelParams GUI_EL_TEMP_LabelSpBoxInactive{
	.element={{0,0,2,0},{alignHCenter, alignVCenter}},
	.font=&Lucida_Console_8x12,
	.color_sch={SPBOX_INACTIVE_TEXT_COLOR,SPBOX_INACTIVE_BACK_COLOR},
	.transparent=1
};

LabelParams GUI_EL_TEMP_LabelButton{
	.element={{3,3,3,3},{alignHCenter, alignVCenter}},
	.font=&Tahoma_15x13,
	.color_sch={GENERAL_TEXT_COLOR,GENERAL_BACK_COLOR},
	.transparent=0
};


TextAreaParams GUI_EL_TEMP_CommonTextAreaLT{
	.element={{0,0,0,0},{alignLeft, alignTop}},
	.font=&Tahoma_15x13,
	.color_sch={GENERAL_TEXT_COLOR,GENERAL_BACK_COLOR},
	.transparent=1
};

TextAreaParams GUI_EL_TEMP_CommonTextAreaCT{
	.element={{0,0,0,0},{alignHCenter, alignTop}},
	.font=&Tahoma_15x13,
	.color_sch={GENERAL_TEXT_COLOR,GENERAL_BACK_COLOR},
	.transparent=1
};

ElementParams GUI_EL_TEMP_CommonIcon{
	.margins={0,0,0,0},
	.align={alignHCenter, alignVCenter}
};

ElementParams GUI_EL_TEMP_IconIndicator{
	.margins={0,0,0,0},
	.align={alignLeft, alignTop}
};

ElementParams GUI_EL_TEMP_IconSpBoxUp{
	.margins={2,2,2,2},
	.align={alignHCenter, alignBottom}
};

ElementParams GUI_EL_TEMP_IconSpBoxDown{
	.margins={2,2,2,2},
	.align={alignHCenter, alignTop}
};


ElementParams GUI_EL_TEMP_BatteryIndicator{
	.margins={1,1,1,1},
	.align={alignLeft, alignTop}
};

VolumeTunerParams GUI_EL_TEMP_VolumeTuner{
	.el_params={{0,0,0,0},{alignLeft, alignBottom}},
	.bar_count=10,
	.bar_interval=4
};

WindowParams GUI_EL_TEMP_WindowGeneral{
	.color_sch={GENERAL_FORE_COLOR,GENERAL_BACK_COLOR},
	.frame_thick=2,
	.round_corners=1
};

WindowParams GUI_EL_TEMP_WindowGeneralBack{
	.color_sch={GENERAL_FORE_COLOR,GENERAL_BACK_COLOR},
	.frame_thick=0,
	.round_corners=0
};

MenuItemParams GUI_EL_TEMP_DefaultMenuItem{
	.label_params={.element={{2,2,2,2},{alignLeft, alignVCenter}},
		.font=&Tahoma_15x13,
		.color_sch={MENU_ITEM_INACTIVE_TEXT_COLOR,MENU_ITEM_INACTIVE_BACK_COLOR},
		.transparent=0},
	.icon_params={{.margins={2,4,2,2}, .align={alignRight, alignVCenter}}, .icon=sym_arrow_12}
};

MenuItemParams GUI_EL_TEMP_ActiveMenuItem{
	.label_params={.element={{2,2,2,2},{alignLeft, alignVCenter}},
			.font=&Tahoma_15x13,
			.color_sch={MENU_ITEM_ACTIVE_TEXT_COLOR,MENU_ITEM_ACTIVE_BACK_COLOR},
			.transparent=0},
		.icon_params={{.margins={2,4,2,2}, .align={alignRight, alignVCenter}}, .icon=sym_arrow_12}
};

MenuItemParams GUI_EL_TEMP_DefaultCheckBoxItem{
	.label_params={.element={{2,2,2,2},{alignLeft, alignVCenter}},
		.font=&Tahoma_15x13,
		.color_sch={MENU_ITEM_INACTIVE_TEXT_COLOR,MENU_ITEM_INACTIVE_BACK_COLOR},
		.transparent=0},
	.icon_params={{.margins={2,4,2,2}, .align={alignRight, alignVCenter}}, .icon=sym_check_mark_12}
};

MenuItemParams GUI_EL_TEMP_ActiveCheckBoxItem{
	.label_params={.element={{2,2,2,2},{alignLeft, alignVCenter}},
		.font=&Tahoma_15x13,
		.color_sch={MENU_ITEM_ACTIVE_TEXT_COLOR,MENU_ITEM_ACTIVE_BACK_COLOR},
		.transparent=0},
	.icon_params={{.margins={2,4,2,2}, .align={alignRight, alignVCenter}}, .icon=sym_check_mark_12}
};

MenuItemParams GUI_EL_TEMP_DefaultBtListItem{
	.label_params={.element={{2,2,2,2},{alignLeft, alignVCenter}},
		.font=&Tahoma_15x13,
		.color_sch={MENU_ITEM_INACTIVE_TEXT_COLOR,MENU_ITEM_INACTIVE_BACK_COLOR},
		.transparent=0},
	.icon_params={{.margins={2,4,2,2}, .align={alignRight, alignVCenter}}, .icon=sym_paired_mark_12}
};

MenuItemParams GUI_EL_TEMP_ActiveBtListItem{
	.label_params={.element={{2,2,2,2},{alignLeft, alignVCenter}},
		.font=&Tahoma_15x13,
		.color_sch={MENU_ITEM_ACTIVE_TEXT_COLOR,MENU_ITEM_ACTIVE_BACK_COLOR},
		.transparent=0},
	.icon_params={{.margins={2,4,2,2}, .align={alignRight, alignVCenter}}, .icon=sym_paired_mark_12}
};

MenuItemParams GUI_EL_TEMP_ActiveBtListItemConnected{
	.label_params={.element={{2,2,2,2},{alignLeft, alignVCenter}},
		.font=&Tahoma_15x13,
		.color_sch={MENU_ITEM_ACTIVE_TEXT_COLOR,MENU_ITEM_ACTIVE_BACK_COLOR},
		.transparent=0},
	.icon_params={{.margins={2,4,2,2}, .align={alignRight, alignVCenter}}, .icon=sym_headset_12}
};

MenuItemParams GUI_EL_TEMP_DefaultBtListItemConnected{
	.label_params={.element={{2,2,2,2},{alignLeft, alignVCenter}},
		.font=&Tahoma_15x13,
		.color_sch={MENU_ITEM_INACTIVE_TEXT_COLOR,MENU_ITEM_INACTIVE_BACK_COLOR},
		.transparent=0},
	.icon_params={{.margins={2,4,2,2}, .align={alignRight, alignVCenter}}, .icon=sym_headset_12}
};

ElementParams GUI_EL_TEMP_SliderUpArrow{
	.margins={0,0,0,0},
	.align={alignHCenter, alignTop}
};

ElementParams GUI_EL_TEMP_SliderDownArrow{
	.margins={0,0,0,0},
	.align={alignHCenter, alignBottom}
};

SpBoxParams GUI_EL_TEMP_CommonSpBox{
	sym_arrow_up,
	sym_arrow_down
};

//----------PROTOTYPES---------

//----------CODE---------------

//-----------------------------

