//----------INCLUDES-----------
#include <string.h>
#include <stdio.h>

#include "dialogs.h"
#include "texts.h"
#include "qmdebug.h"

//----------DEFINES------------
//----------TYPES--------------

//----------GLOBAL_VARS--------

static MoonsGeometry ch_num_label_geom    = {  2, 40,  77,  74 };
static MoonsGeometry ch_type_label_geom    = {  64, 45,  75,  73 };
static MoonsGeometry mode_label_geom  = { 85, 48, 120,  73 };
static MoonsGeometry freq_geom        = {   2, 75, 126, 102 };


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
//    ch_num_label_params.element.align.align_v = alignVCenter;

  LabelParams ch_type_label_params = GUI_EL_TEMP_LabelMode;
  ch_num_label_params.transparent = true;
 // ch_num_label_params.element.align.align_v = alignBottom;

  LabelParams freq_label_params = GUI_EL_TEMP_LabelMode;
  freq_label_params.element.align.align_v = alignVCenter;

  ch_num_label = new GUI_EL_Label (&ch_num_label_params,   &ch_num_label_geom,   NULL, (GUI_Obj*)this);
  ch_type_label= new GUI_EL_Label (&ch_type_label_params,   &ch_type_label_geom,   NULL, (GUI_Obj*)this);
  mode_text    = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &mode_label_geom, NULL, (GUI_Obj*)this);
  freq         = new GUI_EL_Label (&freq_label_params, &freq_geom,       NULL, (GUI_Obj*)this);

  freq->setSkipTextBackgronundFilling(true);

  mode_text->SetText((char*)"--\0");

  ch_num_label->SetText((char*)"--\0");
  ch_type_label->SetText((char*)"\0");
  cur_ch_invalid = false;

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

#if EMUL
    status = Multiradio::VoiceServiceInterface::ChannelStatus::ChannelActive;
    ch_num = 39;
    channel_type = Multiradio::voice_channel_t::channelClose;
    valid_freq = true;
    setFreq("34500000");
#endif

  updateChannel(status, ch_num, channel_type);

  window->Draw();

  //mode_text->transparent = false;
  if (valid_freq)
    mode_text->Draw();

  freq->transparent = true; // todo : поменял значение
  if (focus == 1)
  {
      freq->transparent = false;
  }
  if (valid_freq)
    freq->Draw();

  ch_num_label->transparent = true;
  ch_type_label->transparent = true;
  if (focus == 2)
  {
	  ch_num_label->transparent = false;
  }
  ch_num_label->Draw();
 ch_type_label->Draw();

  if (cur_ch_invalid)
  {
      groundrect(2,15,52,16,0,GFRAME); // ???
  }



//  if (focus == 2)
//  {
//    groundrect(freq_geom.xs,freq_geom.ys,freq_geom.xe,freq_geom.ye,0,GFILL);
//  }
}

//-----------------------------

void GUI_Dialog_MainScr::updateChannel( Multiradio::VoiceServiceInterface::ChannelStatus status,
                                        int                                              ch_num,
                                        Multiradio::voice_channel_t                      channel_type
                                      )
{
  char ch_num_str[3];
  char ch_type_str[2];
  cur_ch_invalid = false;
  switch( status )
  {
      case Multiradio::VoiceServiceInterface::ChannelInvalid:
          cur_ch_invalid = true;
          ch_num_label->SetText((char*)"--\0");
          /* no break */
      case Multiradio::VoiceServiceInterface::ChannelActive:
    	  prepChNumString(ch_num_str, ch_num);
    	  prepChTypeString(ch_type_str, channel_type);
          ch_num_label->SetText(ch_num_str);
          ch_type_label->SetText(ch_type_str);
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

void GUI_Dialog_MainScr::prepChTypeString(char *str, Multiradio::voice_channel_t type )
{
  switch(type)
  {
      case Multiradio::channelOpen:
          str[0] = ch_open_letter;
          break;
      case Multiradio::channelClose:
          str[0] = ch_closed_letter;
          break;
      case Multiradio::channelInvalid:
          str[0] = ch_invalid_letter;
          break;
      default:
          QM_ASSERT(0);
          break;
  }
  str[1] = 0; //null terminated;

}

void GUI_Dialog_MainScr::prepChNumString(char *str, int ch_num)
{
	itoa(ch_num, str, 10);
	if(ch_num < 10)
	{
		str[1] = str[0];
		str[0] = ch_zero_letter;;
	}
	str[2] = 0;
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

void GUI_Dialog_MainScr::setFocus(int newFocus)
{
    focus = newFocus;
}
void GUI_Dialog_MainScr::setFreq(const char *fr)
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

    //strOut.append(freq_hz);

    strOut.push_back('\0');
    freq->SetText((char*)strOut.c_str());
}

bool GUI_Dialog_MainScr::isEditing()
{
    return editing;
}
