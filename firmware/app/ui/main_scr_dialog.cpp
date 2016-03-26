/**
  ******************************************************************************
  * @file     main_scr_dialog.cpp
  * @author  Egor Dudyak, PMR dept. software team, ONIIP, PJSC
  * @date    04 окт. 2013 г.
  * @brief   Диалог главного экрана. Предполагается что он будет создан в единственном экземпляре и не будет удаляться.
  * Поэтому для оптимизации(сокращение размера объекта, изегание лишних отрисовок) объекты Label
  * объявлены глобально и выделяются динамически.
  *
  ******************************************************************************
  */


//----------INCLUDES-----------

#include <string.h>
#include <stdio.h>

#include "dialogs.h"
#include "texts.h"
#include "qmdebug.h"

//----------DEFINES------------

//----------TYPES--------------

//----------GLOBAL_VARS--------

static MoonsGeometry ch_label_geom  = {  1,  5,  78,  65 };
static MoonsGeometry mode_text_geom = { 80,  5, 158,  32 };
static MoonsGeometry freq_geom      = {  10, 75, 149, 125 };

bool GUI_main_scr_init_flag=0;

//----------PROTOTYPES---------

//----------CODE---------------


GUI_Dialog_MainScr::GUI_Dialog_MainScr(MoonsGeometry *area):GUI_Obj(area){
	//mr_channel_t active_channel;

	MoonsGeometry window_geom={0,0,(GXT)(GEOM_W(this->area)-1),(GYT)(GEOM_H(this->area)-1)};

    GUI_EL_TEMP_LabelChannel.transparent = true;
    GUI_EL_TEMP_LabelMode.transparent  = true;

    window       = new GUI_EL_Window(&GUI_EL_TEMP_WindowGeneralBack, &window_geom,          (GUI_Obj*)this);
    ch_num_label = new GUI_EL_Label (&GUI_EL_TEMP_LabelChannel,      &ch_label_geom,  NULL, (GUI_Obj*)this);
    mode_text    = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode,         &mode_text_geom, NULL, (GUI_Obj*)this);
    freq         = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode,         &freq_geom,      NULL, (GUI_Obj*)this);


    cur_ch_invalid = false;
}

//-----------------------------
void GUI_Dialog_MainScr::setModeText(const char* newMode)
{
    mode.clear();
    mode.append(newMode);
}

void GUI_Dialog_MainScr::Draw( Multiradio::VoiceServiceInterface::ChannelStatus status,
                               int                                              ch_num,
                               Multiradio::voice_channel_t                      channel_type
                              )
{
    updateChannel(status, ch_num, channel_type);
    mode_text->SetText((char *)mode.c_str());

//    std::string str;
//    str.append("25000.00");
//    str.append(freq_khz);
//    freq->SetText((char*)str.c_str());
//    str.clear();

	window->Draw();
	ch_num_label->Draw();
    if(cur_ch_invalid)
    {
		groundrect(2,30,52,31,0,GFRAME);
	}

    mode_text->transparent = true;
    if (focus == 0)
        mode_text->transparent = false;

    mode_text->Draw();

    freq->transparent = true;
    if (focus == 1)
        freq->transparent = false;

    freq->Draw();

}

//-----------------------------

void GUI_Dialog_MainScr::updateChannel( Multiradio::VoiceServiceInterface::ChannelStatus status,
                                        int                                              ch_num,
                                        Multiradio::voice_channel_t                      channel_type
                                       )
{
    char ch_str[4];
    cur_ch_invalid = false;
    switch( status )
    {
        case Multiradio::VoiceServiceInterface::ChannelInvalid:
            cur_ch_invalid = true;
            /* no break */
        case Multiradio::VoiceServiceInterface::ChannelActive:
            prepChString(ch_str, ch_num, channel_type);
            ch_num_label->SetText(ch_str);
            break;
        case Multiradio::VoiceServiceInterface::ChannelDisabled:
            ch_num_label->SetText(disabled_ch_txt);
            break;
        default:
            QM_ASSERT(0);
            break;
    }
}


void GUI_Dialog_MainScr::setFreq(char* mas_freq)
{

  //  if ((Key >= 6) && (Key<=15))
 //   s2.append((char *)(48+Key - 6));
    freq->SetText( (char*) s2.append(mas_freq).append(freq_hz).c_str());
    s2 =  "";

}

void GUI_Dialog_MainScr::clearFreq()
{
      freq->SetText("");
}

//-----------------------------

void GUI_Dialog_MainScr::prepChString(char *str, int ch_num, Multiradio::voice_channel_t type )
{
    itoa(ch_num, str, 10);
    if(ch_num < 10)
    {
        str[1] = str[0];
        str[0] = ch_zero_letter;
	}
	switch(type){
		case Multiradio::channelOpen:
            str[2] = ch_open_letter;
			break;
		case Multiradio::channelClose:
            str[2] = ch_closed_letter;
			break;
		case Multiradio::channelInvalid:
            str[2] = ch_invalid_letter;
			break;
		default:
			QM_ASSERT(0);
			break;
	}
    str[3] = 0;	//null terminated;
}

//-----------------------------

GUI_Dialog_MainScr::~GUI_Dialog_MainScr()
{
    delete window;
    delete ch_num_label;
    delete mode_text;
    delete freq;
}

//-----------------------------


void GUI_Dialog_MainScr::keyPressed(UI_Key key)
{
    int value = -1;
    switch( key )
    {
    case keyBack:
        if (mwFocus != 0)
            mwFocus = -2;
        break;
    case keyLeft:
        if (mwFocus == 1 && mainWindowModeId > 0)
            mainWindowModeId--;
        break;
    case keyRight:
        if (mwFocus == 1 && mainWindowModeId < 2)
            mainWindowModeId++;
        break;
    case keyUp:
        if (mwFocus < 1)
            mwFocus++;
        break;
    case keyDown:
        if (mwFocus > 0)
            mwFocus--;
        break;
    case key0:
            value = 0;
        break;
    case key1:
        value = 1;
        break;
    case key2:
        value = 2;
        break;
    case key3:
        value = 3;
        break;
    case key4:
        value = 4;
        break;
    case key5:
        value = 5;
        break;
    case key6:
        value = 6;
        break;
    case key7:
        value = 7;
        break;
    case key8:
        value = 8;
        break;
    case key9:
        value = 9;
        break;
    default:
        break;
    }

    setFocus(1-mwFocus);
}

