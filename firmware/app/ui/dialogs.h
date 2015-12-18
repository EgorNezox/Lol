﻿/**
  ******************************************************************************
  * @file     dialogs.h
  * @author  Egor Dudyak, PMR dept. software team, ONIIP, PJSC
  * @date    03 окт. 2013 г.
  * @brief   ќписание классов диалогов наследуемых оn Moons_Dialog
  ******************************************************************************
  */

#ifndef DIALOGS_H_
#define DIALOGS_H_

#include <stdint.h>
#include <stddef.h>

#include "service.h"
#include "elements.h"
#include "keyboard.h"

extern MoonsGeometry ui_common_dialog_area;
extern MoonsGeometry ui_indicator_area;

//--------------------------

/*! ласс диалога главного рабочего экрана*/
class GUI_Dialog_MainScr: public GUI_Obj{
	public:
		GUI_Dialog_MainScr(MoonsGeometry *area, Ui::Service *service);
		~GUI_Dialog_MainScr();
		virtual void Draw();
		void keyHandler(UI_Key key);
	private:
		void updateChannel();
		void prepChString(char *str, int ch_num, Multiradio::voice_channel_t type );
		Ui::Service *service;
		GUI_EL_Window *window;
		GUI_EL_Label *ch_num_label;
		GUI_EL_Label *mode_text;
		bool cur_ch_invalid;
};

//--------------------------


/*!Класса панели индикаторов: отображает статусную информацию в верхней части экрана.
 * Объект этого класса создается глобально в единственном экземпляре. Работа с ним осуществляется напрямую*/
class GUI_Indicator: public GUI_Obj{
	public:
		GUI_Indicator(MoonsGeometry *area, Ui::Service *service);
		void UpdateMultiradio(Multiradio::MainServiceInterface::Status status);
		void UpdateHeadset(Headset::Controller::Status status);
		void UpdateBattery(int new_val);
		virtual void Draw();
	private:
		Ui::Service *service;
		GUI_EL_Icon *ind_multiradio;
		GUI_EL_Icon *ind_headset;
		GUI_EL_Battery *ind_battery;
};


/*!Класс диалога вывода сообщения*/
class GUI_Dialog_MsgBox: public GUI_Obj{
	public:
		GUI_Dialog_MsgBox(MoonsGeometry* area, char *text, Alignment align, Ui::Service *service);
		~GUI_Dialog_MsgBox();
		virtual void Draw();
		void keyHandler(UI_Key key);
	protected:
		MoonsGeometry window_geom;
		TextAreaParams text_area_params;
		char *text;
	private:
		Ui::Service *service;
		MoonsGeometry text_area_geom;
		MoonsGeometry button_geom;
};


#endif /* DIALOGS_H_ */
