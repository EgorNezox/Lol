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
#include <string>

#include "service.h"
#include "elements.h"
#include "keyboard.h"

extern MoonsGeometry ui_common_dialog_area;
extern MoonsGeometry ui_indicator_area;

//--------------------------

/*!Класс диалога главного рабочего экрана*/
class GUI_Dialog_MainScr: public GUI_Obj
{
public:
    GUI_Dialog_MainScr(MoonsGeometry *area);
    virtual ~GUI_Dialog_MainScr();
    virtual void Draw( Multiradio::VoiceServiceInterface::ChannelStatus status,
                       int ch_num,
                       Multiradio::voice_channel_t channel_type
                       );
    void setModeText(const char*);
    void setFocus(int newFocus)
    {
        focus = newFocus;
    }
    void setFreq(const char *fr)
    {
        std::string str;
        str.append(fr);
        str.append(freq_hz);
        freq->SetText((char*)str.c_str());
    }
    void editingFreq(UI_Key);
    void keyPressed(UI_Key);
    bool isEditing(){ if(editing) return true; else return false; }
    bool editing;
    std::string nFreq, oFreq;
private:
    GUI_EL_Window *window;
    GUI_EL_Label *ch_num_label;
    GUI_EL_Label *mode_text;
    GUI_EL_Label *freq;
    bool cur_ch_invalid;
    std::string mode;
    int focus = -2;

    void updateChannel(Multiradio::VoiceServiceInterface::ChannelStatus status,
                       int ch_num,
                       Multiradio::voice_channel_t channel_type
                       );
    void prepChString(char *str, int ch_num, Multiradio::voice_channel_t type );
public:
    // фокус элементов главного экрана
    int mwFocus = -1;
    int mainWindowModeId = 0;
};

//--------------------------

/*!Класса панели индикаторов: отображает статусную информацию в верхней части экрана.
 * Объект этого класса создается глобально в единственном экземпляре. Работа с ним осуществляется напрямую*/
class GUI_Indicator: public GUI_Obj{
        public:
                GUI_Indicator(MoonsGeometry *area);
        virtual ~GUI_Indicator();
        void UpdateMultiradio(Multiradio::MainServiceInterface::Status status);
        void UpdateHeadset(Headset::Controller::Status status);
        void UpdateBattery(int new_val);
        void Draw();
        virtual void Draw( Multiradio::MainServiceInterface::Status,
                                   Headset::Controller::Status,
                                   int
                                   );
private:
        GUI_EL_Icon *ind_multiradio;
        GUI_EL_Icon *ind_headset;
        GUI_EL_Battery *ind_battery;
};


/*!Класс диалога вывода сообщения*/
class GUI_Dialog_MsgBox: public GUI_Obj
{
public:
    GUI_Dialog_MsgBox(MoonsGeometry* area, char *text, Alignment align);
    virtual ~GUI_Dialog_MsgBox();
    virtual void Draw();
    void keyPressed(UI_Key);
protected:
    MoonsGeometry window_geom;
    TextAreaParams text_area_params;
    char *text;
private:
    MoonsGeometry text_area_geom;
    MoonsGeometry button_geom;
};

#endif /* DIALOGS_H_ */
