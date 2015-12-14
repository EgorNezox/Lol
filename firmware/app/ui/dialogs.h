/**
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

#define LANG_COUNT 2

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


#endif /* DIALOGS_H_ */
