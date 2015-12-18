/**
  ******************************************************************************
  * @file     element_templates.h
  * @author  Egor Dudyak, PMR dept. software team, ONIIP, PJSC
  * @date    08 окт. 2013 г.
  * @brief   ќпредел¤ет цветовые схемы и объ¤вл¤ет шаблоны параметров элементов
  ******************************************************************************
  */

#ifndef ELEMENT_TEMPLATES_H_
#define ELEMENT_TEMPLATES_H_

#define GCS_DARK

/*”правление цветовой схемой осуществл¤етс¤ через опцию компил¤ции GUI_COLOR_SCEME=GCS_BLACK/GCS_WHITE*/
#ifdef GCS_LIGHT
	#define GENERAL_TEXT_COLOR				G_BLACK
	#define GENERAL_FORE_COLOR				G_BLACK
	#define GENERAL_BACK_COLOR				G_WHITE
	#define MENU_ITEM_INACTIVE_BACK_COLOR	G_WHITE
	#define MENU_ITEM_INACTIVE_TEXT_COLOR	G_BLACK
	#define MENU_ITEM_ACTIVE_BACK_COLOR		G_BLUE_LIGHT
	#define MENU_ITEM_ACTIVE_TEXT_COLOR		G_BLACK
	#define SPBOX_ACTIVE_BACK_COLOR			G_GREY
	#define SPBOX_ACTIVE_TEXT_COLOR			G_BLACK
	#define SPBOX_INACTIVE_BACK_COLOR		G_WHITE
	#define SPBOX_INACTIVE_TEXT_COLOR		G_BLACK
	#define BATTERY_HIGH_COLOR				G_GREEN
	#define BATTERY_MID_COLOR				G_YELLOW
	#define BATTERY_LOW_COLOR				G_RED
#endif

#ifdef GCS_DARK
	#define GENERAL_TEXT_COLOR				G_YELLOW
	#define GENERAL_FORE_COLOR				G_YELLOW
	#define GENERAL_BACK_COLOR				G_BLACK
	#define MENU_ITEM_INACTIVE_BACK_COLOR	G_BLACK
	#define MENU_ITEM_INACTIVE_TEXT_COLOR	G_YELLOW
	#define MENU_ITEM_ACTIVE_BACK_COLOR		G_YELLOW
	#define MENU_ITEM_ACTIVE_TEXT_COLOR		G_BLACK
	#define SPBOX_ACTIVE_BACK_COLOR			G_YELLOW
	#define SPBOX_ACTIVE_TEXT_COLOR			G_BLACK
	#define SPBOX_INACTIVE_BACK_COLOR		G_BLACK
	#define SPBOX_INACTIVE_TEXT_COLOR		G_YELLOW
	#define BATTERY_HIGH_COLOR				G_YELLOW
	#define BATTERY_MID_COLOR				G_YELLOW
	#define BATTERY_LOW_COLOR				G_YELLOW
#endif

extern LabelParams GUI_EL_TEMP_LabelChannel;
extern LabelParams GUI_EL_TEMP_LabelTitle;
extern LabelParams GUI_EL_TEMP_LabelText;
extern LabelParams GUI_EL_TEMP_LabelInput;
extern LabelParams GUI_EL_TEMP_LabelCoords;
extern LabelParams GUI_EL_TEMP_LabelSpBoxActive;
extern LabelParams GUI_EL_TEMP_LabelSpBoxInactive;
extern LabelParams GUI_EL_TEMP_LabelButton;
extern LabelParams GUI_EL_TEMP_LabelMode;;
extern TextAreaParams GUI_EL_TEMP_CommonTextAreaLT;
extern TextAreaParams GUI_EL_TEMP_CommonTextAreaCT;
extern ElementParams GUI_EL_TEMP_IconSpBoxUp;
extern ElementParams GUI_EL_TEMP_IconSpBoxDown;
extern ElementParams GUI_EL_TEMP_CommonIcon;
extern ElementParams GUI_EL_TEMP_IconIndicator;
extern ElementParams GUI_EL_TEMP_BatteryIndicator;
extern ElementParams GUI_EL_TEMP_SliderUpArrow;
extern ElementParams GUI_EL_TEMP_SliderDownArrow;
extern VolumeTunerParams GUI_EL_TEMP_VolumeTuner;
extern WindowParams GUI_EL_TEMP_WindowGeneral;
extern WindowParams GUI_EL_TEMP_WindowGeneralBack;
extern MenuItemParams GUI_EL_TEMP_DefaultMenuItem;
extern MenuItemParams GUI_EL_TEMP_ActiveMenuItem;
extern MenuItemParams GUI_EL_TEMP_DefaultCheckBoxItem;
extern MenuItemParams GUI_EL_TEMP_ActiveCheckBoxItem;
extern MenuItemParams GUI_EL_TEMP_DefaultBtListItem;
extern MenuItemParams GUI_EL_TEMP_ActiveBtListItem;
extern MenuItemParams GUI_EL_TEMP_DefaultBtListItemConnected;
extern MenuItemParams GUI_EL_TEMP_ActiveBtListItemConnected;
extern SpBoxParams GUI_EL_TEMP_CommonSpBox;

#endif /* ELEMENT_TEMPLATES_H_ */
