//----------INCLUDES-----------
#include <string.h>
#include <stdio.h>

#include "dialogs.h"
#include "texts.h"
#include "qmdebug.h"

//----------DEFINES------------
//----------TYPES--------------

//----------GLOBAL_VARS--------

static MoonsGeometry ch_label_geom    = {  10, 40,  95,  75 };
static MoonsGeometry mode_label_geom  = { 100, 40, 150,  75 };
static MoonsGeometry freq_geom        = {   9, 76, 150, 120 };

bool GUI_main_scr_init_flag=0;

//----------PROTOTYPES---------

//----------CODE---------------


GUI_Dialog_MainScr::GUI_Dialog_MainScr(MoonsGeometry *area):GUI_Obj(area),
                                                          mainWindowModeId(0),
                                                          mwFocus(-2),
                                                          editing(false)
{
  MoonsGeometry window_geom = {0,0,(GXT)(GEOM_W(this->area)-1),(GYT)(GEOM_H(this->area)-1)};

    GUI_EL_TEMP_LabelMode.transparent = true;
  window       = new GUI_EL_Window(&GUI_EL_TEMP_WindowGeneralBack, &window_geom,          (GUI_Obj*)this);

  LabelParams ch_num_label_params = GUI_EL_TEMP_LabelChannel;
  ch_num_label_params.transparent = true;
  ch_num_label = new GUI_EL_Label (&ch_num_label_params,   &ch_label_geom,   NULL, (GUI_Obj*)this);
  mode_text    = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &mode_label_geom, NULL, (GUI_Obj*)this);
  freq         = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &freq_geom,       NULL, (GUI_Obj*)this);

  mode_text->SetText((char*)"--\0");

  ch_num_label->SetText((char*)"--\0");
  cur_ch_invalid = false;

  oFreq.append("2500000");
  setFreq(oFreq.c_str());
}

void GUI_Dialog_MainScr::setModeText(const char* newMode)
{
    mode_text->SetText((char*)newMode);
}

void GUI_Dialog_MainScr::Draw( Multiradio::VoiceServiceInterface::ChannelStatus status,
                               int                                              ch_num,
                               Multiradio::voice_channel_t                      channel_type,
                               bool valid_freq
                              )
{
  updateChannel(status, ch_num, channel_type);

  window->Draw();

  //ch_num_label->transparent = false;

  ch_num_label->Draw();

  if (cur_ch_invalid)
  {
      groundrect(2,30,52,31,0,GFRAME);
  }

  //mode_text->transparent = false;
  if (valid_freq)
    mode_text->Draw();

  freq->transparent = true; // todo : поменял значение
  if (focus == 1){ freq->transparent = false; }

  if (valid_freq)
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
          ch_num_label->SetText((char*)"--\0");
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

//-----------------------------

void GUI_Dialog_MainScr::prepChString(char *str, int ch_num, Multiradio::voice_channel_t type )
{
  itoa(ch_num, str, 10);
  if(ch_num < 10)
  {
      str[1] = str[0];
      str[0] = ch_zero_letter;
  }
  switch(type)
  {
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
  delete freq;
}

//-----------------------------
void GUI_Dialog_MainScr::editingFreq(UI_Key key)
{

  if (nFreq.size() < 8)
  {
      if ( key > 5 && key < 17)
          nFreq.push_back((char)(42 + key));
  }
  setFreq(nFreq.c_str());
}

void GUI_Dialog_MainScr::keyPressed(UI_Key key)
{
  int value = -1;
  switch( key )
  {
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

  if ( value != -1)
  {
      if ( mwFocus == 0 ) // setFrequence
      {}
  }
}
