//----------INCLUDES-----------

#include "dialogs.h"
#include "texts.h"
#include "qmdebug.h"
#include <stdio.h>

//----------DEFINES------------

#define MARGIN			4
#define BUTTON_HEIGHT	13
#define BUTTON_WIDTH	30

//----------TYPES--------------

//----------GLOBAL_VARS--------



//----------PROTOTYPES---------

//----------CODE---------------

GUI_Dialog_MsgBox::GUI_Dialog_MsgBox( MoonsGeometry* area,
                                      const char *p_title,
                                      Alignment align
                                      )
    :GUI_Obj(area)
{
  this->window_geom = { 1, 1, 166, 126 };
  title_area_params = GUI_EL_TEMP_CommonTextAreaLT;
  title_area_params.element.align = align;
  text_area_params = GUI_EL_TEMP_CommonTextAreaLT;
  text_area_params.element.align = align;
  this->title.append( p_title );
}

GUI_Dialog_MsgBox::GUI_Dialog_MsgBox( MoonsGeometry* area,
                                      const char *p_title,
                                      const char *p_text,
                                      Alignment align
                                      )
    :GUI_Obj(area)
{
    this->window_geom = { 1, 1, 166, 126 };
    title_area_params = GUI_EL_TEMP_CommonTextAreaLT;
    title_area_params.element.align = align;
    text_area_params = GUI_EL_TEMP_CommonTextAreaLT;
    text_area_params.element.align = align;
    this->title = p_title;
    strcpy(this->text, p_text);
}

GUI_Dialog_MsgBox::GUI_Dialog_MsgBox( MoonsGeometry* area,
                                      const char *p_title,
                                      const int p_text,
                                      Alignment align
                                      )
    :GUI_Obj(area)
{
    this->window_geom = { 1, 1, 166, 126 };
    title_area_params = GUI_EL_TEMP_CommonTextAreaLT;
    title_area_params.element.align = align;
    text_area_params = GUI_EL_TEMP_LabelMode;
    text_area_params.element.align = align;
    this->title = p_title;

    char sym[3];
    sprintf(sym,"%d",p_text);
    sym[2] = '\0';
    memcpy(&this->text, &sym, 3);
}

GUI_Dialog_MsgBox::GUI_Dialog_MsgBox( MoonsGeometry* area,
                                      const char *p_title,
                                      const int   p_text,
                                      const int   list_size,
                                      const int   position,
                                      Alignment align
                                      )
    :GUI_Obj(area)
{
    this->window_geom = { 1, 1, 166, 126 };
    title_area_params = GUI_EL_TEMP_CommonTextAreaLT;
    title_area_params.element.align = align;
    text_area_params = GUI_EL_TEMP_LabelMode;
    text_area_params.element.align = align;
    this->title = p_title;
    this->list_size = list_size;
    this->position = position;

    setCmd(p_text);
}
//----------------------------

void GUI_Dialog_MsgBox::setCmd(int cmd)
{
    char sym[3];
    sprintf(sym,"%d", cmd);
    sym[2] = '\0';
    memcpy(&this->text, &sym, 3);
}

GUI_Dialog_MsgBox::~GUI_Dialog_MsgBox()
{

}

//-----------------------------

void GUI_Dialog_MsgBox::Draw(){
  title_area_geom = { (GXT)(window_geom.xs + MARGIN), (GYT)(window_geom.ys + MARGIN), (GXT)(window_geom.xe - MARGIN), (GYT)( window_geom.ys + 5*MARGIN) };
  text_area_geom  = { (GXT)(window_geom.xs + MARGIN), (GYT)(window_geom.ys + 10*MARGIN + 1), (GXT)(window_geom.xe - MARGIN), (GYT)( window_geom.ye - (MARGIN + BUTTON_HEIGHT) ) };

  button_geom.xs = window_geom.xs + ( (GEOM_W(window_geom) - BUTTON_WIDTH) / 2 );
  button_geom.ys = window_geom.ye - MARGIN-BUTTON_HEIGHT + 1;
  button_geom.xe = button_geom.xs + BUTTON_WIDTH - 1;
  button_geom.ye = window_geom.ye - MARGIN;

  GUI_EL_Window   window( &GUI_EL_TEMP_WindowGeneral, &window_geom, (GUI_Obj *)this );
  GUI_EL_TextArea title_area( &title_area_params, &title_area_geom, (char*)title.c_str(), (GUI_Obj *)this );
  GUI_EL_TextArea text_area ( &text_area_params,  &text_area_geom,  (char*)text,  (GUI_Obj *)this );
  GUI_EL_Button   ok_button( &GUI_EL_TEMP_LabelButton, &button_geom, ok_texts[/*service->getLanguage()*/0], bs_unselected, (GUI_Obj *)this);

  window.Draw();
  title_area.Draw();
  text_area.Draw();
  ok_button.Draw();
}

void GUI_Dialog_MsgBox::Draws(){
  title_area_geom = { (GXT)(window_geom.xs + MARGIN), (GYT)(window_geom.ys + MARGIN), (GXT)(window_geom.xe - MARGIN), (GYT)( window_geom.ys + 5*MARGIN) };
  text_area_geom  = { (GXT)(window_geom.xs + MARGIN), (GYT)(window_geom.ys + 10*MARGIN + 1), (GXT)(window_geom.xe - MARGIN), (GYT)( window_geom.ye - (MARGIN + BUTTON_HEIGHT) ) };

  button_geom.xs = window_geom.xs + ( (GEOM_W(window_geom) - BUTTON_WIDTH) / 2 );
  button_geom.ys = window_geom.ye - MARGIN-BUTTON_HEIGHT + 1;
  button_geom.xe = button_geom.xs + BUTTON_WIDTH - 1;
  button_geom.ye = window_geom.ye - MARGIN;

  text_area_params.transparent = true;

  GUI_EL_Window   window( &GUI_EL_TEMP_WindowGeneral, &window_geom, (GUI_Obj *)this );
  GUI_EL_TextArea title_area( &title_area_params, &title_area_geom, (char*)title.c_str(), (GUI_Obj *)this );
  GUI_EL_TextArea text_area ( &text_area_params,  &text_area_geom,  (char*)text,  (GUI_Obj *)this );
  GUI_EL_Button   ok_button( &GUI_EL_TEMP_LabelButton, &button_geom, ok_texts[/*service->getLanguage()*/0], bs_unselected, (GUI_Obj *)this);

  MoonsGeometry sliderArea  = { window_geom.xe - 4*MARGIN, title_area_geom.ye, window_geom.xe - 2*MARGIN, button_geom.ys-5};
  SliderParams sliderParams = {(int32_t)list_size, (int32_t)list_size-1, (int32_t)position};
  GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this);

  window.Draw();
  title_area.Draw();
  text_area.Draw();
  slider.Draw();
  ok_button.Draw();
}

void GUI_Dialog_MsgBox::keyPressed(UI_Key key)
{
//    if (1)
//    {
//        if (key == keyUp && position > 0)
//        { position--; }
//        if (key == keyDown && position < list_size)
//        { position++;}
//    }
}
