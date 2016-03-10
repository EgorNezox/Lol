/**
  ******************************************************************************
  * @file     msgbox_dialog.cpp
  * @author  Egor Dudyak, PMR dept. software team, ONIIP, PJSC
  * @date    13 ���. 2013 �.
  * @brief   
  *
  ******************************************************************************
  */


//----------INCLUDES-----------

#include "dialogs.h"
#include "texts.h"
#include "qmdebug.h"

//----------DEFINES------------

#define MARGIN			4
#define BUTTON_HEIGHT	15
#define BUTTON_WIDTH	30

//----------TYPES--------------

//----------GLOBAL_VARS--------



//----------PROTOTYPES---------

//----------CODE---------------

GUI_Dialog_MsgBox::GUI_Dialog_MsgBox(MoonsGeometry* area, char *text, Alignment align):GUI_Obj(area){
	window_geom={0,0,(GXT)(GEOM_W(this->area)-1),(GYT)(GEOM_H(this->area)-1)};
	text_area_params=GUI_EL_TEMP_CommonTextAreaLT;
	text_area_params.element.align=align;
	this->text=text;
//	this->service=service;
}

//----------------------------

GUI_Dialog_MsgBox::~GUI_Dialog_MsgBox(){

}

//-----------------------------

void GUI_Dialog_MsgBox::Draw(){
	text_area_geom={(GXT)(window_geom.xs+MARGIN),(GYT)(window_geom.ys+MARGIN),(GXT)(window_geom.xe-MARGIN),(GYT)(window_geom.ye-(MARGIN+BUTTON_HEIGHT))};
	button_geom.xs=window_geom.xs+((GEOM_W(window_geom)-BUTTON_WIDTH)/2);
	button_geom.ys=window_geom.ye-MARGIN-BUTTON_HEIGHT+1;
	button_geom.xe=button_geom.xs+BUTTON_WIDTH-1;
	button_geom.ye=window_geom.ye-MARGIN;
	GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral,&window_geom,(GUI_Obj *)this);
	GUI_EL_TextArea text_area(&text_area_params,&text_area_geom,text,(GUI_Obj *)this);
    GUI_EL_Button ok_button(&GUI_EL_TEMP_LabelButton, &button_geom, ok_texts[/*service->getLanguage()*/0], bs_unselected,(GUI_Obj *)this);

	window.Draw();
	text_area.Draw();
	ok_button.Draw();
}

//-----------------------------
/*
void GUI_Dialog_MsgBox::keyHandler(UI_Key key){
	switch(key){
        case keyEnter:
            //delete this;
		default:
			break;
	}
}
*/
