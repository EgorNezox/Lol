/*
 * qmguiscreen.cpp
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#include "qmdebug.h"
#include "qmguiscreen.h"
#include "qmguivisualobject.h"
#include "qmguidialog.h"
#include "qmguiscenario.h"

//----------DEFINES------------

//----------TYPES--------------

//----------GLOBAL_VARS--------

//----------PROTOTYPES---------

//----------CODE---------------


QmGuiScreen::QmGuiScreen(QmObject *parent):QmObject(parent){
	InitScreen();
}

//-----------------------------

QmGuiScreen::~QmGuiScreen(){

}

//-----------------------------

void QmGuiScreen::RenderScreen(){
	std::list<ScreenStackInst>::iterator  it, end;
	for(it=stack.begin(), end=stack.end();it!=end;it++){
		if(it==end--){
			DrawProcessor(it->gui_obj);
		}
		else{
			if(it->draw_if_non_active){
				DrawProcessor(it->gui_obj);
			}
		}
	}
}

void QmGuiScreen::DrawProcessor(QmGuiObject *gui_obj){
	switch(gui_obj->getType()){
		case qmguiVisual:
			((QmGuiVisualObject *)gui_obj)->Draw();
		case qmguiDialog:
			((QmGuiDialog *)gui_obj)->Draw();
			break;
		case qmguiScenario:
			((QmGuiScenario *)gui_obj)->RenderSubScreen();
			break;
	}
}

//-----------------------------

void QmGuiScreen::ResetToInitState(){	//Объекты удаляются напрямую, это возможно при переключении в другой режим.
	std::list<ScreenStackInst>::iterator  it, end;
	for(it=stack.begin(), end=stack.end();it!=end;it++){
		delete it->gui_obj;
	}
	stack.erase(stack.begin(),stack.end());
	InitScreen();
}

//-----------------------------

void QmGuiScreen::AppendObject(QmGuiObject *obj_copy, bool draw_if_non_active){
	QM_ASSERT(obj_copy!=NULL);
	stack.push_back((ScreenStackInst){obj_copy, draw_if_non_active});
}

//-----------------------------

bool QmGuiScreen::event(QmEvent *event){
	QM_UNUSED(event);
	return false;
}

//-----------------------------

void QmGuiScreen::InitScreen(){

}


