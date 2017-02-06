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
#include <stdio.h>

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
                       Multiradio::voice_channel_t channel_type,
                       bool valid_freq
                       );
    void setModeText(const char*);
    void setFocus(int newFocus)
    {
        focus = newFocus;
    }
    void setFreq(const char *fr)
    {
        std::string strIn, strOut;
        strIn.append(fr);
        uint8_t size = strIn.size();
        for (uint8_t i = 0; i < size; i++)
        {
            strOut.push_back(strIn[i]);
            if (i == (size - 4) || i == (size - 7))
            strOut.push_back('.');
        }
        strOut.append(freq_hz);
        strOut.push_back('\0');
        freq->SetText((char*)strOut.c_str());
    }
    void editingFreq(UI_Key);
    void keyPressed(UI_Key);
    bool isEditing(){ if(editing) return true; else return false; }
    bool editing;
    std::string nFreq, oFreq;



private:
    GUI_EL_Window *window;
    GUI_EL_Label  *ch_num_label;
    GUI_EL_Label  *mode_text;
    GUI_EL_Label  *freq;
    bool cur_ch_invalid;
    int focus = -2;

    void updateChannel(Multiradio::VoiceServiceInterface::ChannelStatus status,
                       int ch_num,
                       Multiradio::voice_channel_t channel_type
                       );
    void prepChString(char *str, int ch_num, Multiradio::voice_channel_t type );
public:
    // фокус элементов главного экрана
    int mwFocus;
    int mainWindowModeId;
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
        void UpdateGpsStatus(uint8_t gpsStatus);
        void Draw();
        virtual void Draw( Multiradio::MainServiceInterface::Status,
                           Headset::Controller::Status,
                           int,
                           uint8_t
                          );
        void setDateTime(std::string str)
        {
            date_time->SetText((char*)str.c_str());
        }
private:
        GUI_EL_Icon *ind_multiradio;
        GUI_EL_Icon *ind_headset;
        GUI_EL_Battery *ind_battery;
public:
        GUI_EL_Label *date_time;
        GUI_EL_Icon  *gpsLabel;
};


/*!Класс диалога вывода сообщения*/
class GUI_Dialog_MsgBox: public GUI_Obj
{
public:
    GUI_Dialog_MsgBox(MoonsGeometry* area, const char *title, Alignment align);
    GUI_Dialog_MsgBox(MoonsGeometry* area, const char *title, const char *text, Alignment align);
    GUI_Dialog_MsgBox(MoonsGeometry*, const char*, const int, Alignment);
    GUI_Dialog_MsgBox(MoonsGeometry*, const char*, const int, const int, const int, Alignment);
    virtual ~GUI_Dialog_MsgBox();
    virtual void Draw();
    void Draws();
    void keyPressed(UI_Key);
    void setCmd(int cmd);
    void Draw_Sms();
    static void showMessage(MoonsGeometry *area, bool isFrame, const char *p_title, const char *p_text);
protected:
    MoonsGeometry window_geom;
    TextAreaParams title_area_params;
    TextAreaParams text_area_params;
    std::string title;
//    std::string text;
    char text[255];

    // slider
public:
    uint32_t list_size;
    uint32_t position;

    uint8_t focus_rxline = 1;
    uint8_t max_line = 1;

    void DrawWithCoord(uint8_t *coord);
private:
    MoonsGeometry title_area_geom;
    MoonsGeometry text_area_geom;
    MoonsGeometry button_geom;
};

#endif /* DIALOGS_H_ */
