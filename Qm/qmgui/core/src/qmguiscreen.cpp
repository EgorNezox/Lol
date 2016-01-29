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
#include "qmguikey.h"



QmGuiScreen::QmGuiScreen(QmObject *parent):QmObject(parent){
	initScreen();
}


QmGuiScreen::~QmGuiScreen(){

}


void QmGuiScreen::renderScreen(){
	std::list<ScreenStackInst>::iterator  it, end;
	for(it=stack.begin(), end=stack.end();it!=end;it++){
		if(it==end--){
			drawProcessor(it->gui_obj);
		}
		else{
			if(it->draw_if_non_active){
				drawProcessor(it->gui_obj);
			}
		}
	}
}


void QmGuiScreen::deleteGuiObkectFromStack(QmGuiObject *gui_obj){
	std::list<ScreenStackInst>::iterator  it, end;
	for(it=stack.begin(), end=stack.end();it!=end;it++){
		if(it->gui_obj==gui_obj){
			stack.erase(it);
			break;
		}
	}
}


void QmGuiScreen::drawProcessor(QmGuiObject *gui_obj){
	switch(gui_obj->getType()){
		case qmguiVisual:
			((QmGuiVisualObject *)gui_obj)->Draw();
			break;
		case qmguiDialog:
			((QmGuiDialog *)gui_obj)->Draw();
			break;
		case qmguiScenario:
			((QmGuiScenario *)gui_obj)->RenderSubScreen();
			break;
		default:
			QM_ASSERT(0);
			break;
	}
}



void QmGuiScreen::resetToInitState(){	//Объекты удаляются напрямую, это возможно при переключении в другой режим.
	std::list<ScreenStackInst>::iterator  it, end;
	for(it=stack.begin(), end=stack.end();it!=end;it++){
		it->gui_obj->deleteLater();	//todo порядок выполнения операций!
	}
	stack.erase(stack.begin(),stack.end());
	initScreen();
}



void QmGuiScreen::appendObject(QmGuiObject *gui_obj, bool draw_if_non_active){
	QM_ASSERT(gui_obj!=NULL);
	stack.push_back((ScreenStackInst){gui_obj, draw_if_non_active});
	gui_obj->setParentScreen(this);
}


void QmGuiScreen::keyProcessor(QmGuiKey key){
	std::list<ScreenStackInst>::iterator  it, begin;
	if(stack.size()>0){
		it=stack.end();
		begin=stack.begin();
		do{
			it--;
			switch(it->gui_obj->type){
				case qmguiDialog:
					((QmGuiDialog *)(it->gui_obj))->keyHandler(key);
					return;
					break;
				case qmguiScenario:
					((QmGuiScenario *)(it->gui_obj))->sub_screen->keyProcessor(key);
					return;
					break;
				case qmguiVisual:
					break;
				default:
					QM_ASSERT(0);
					break;
			}
		}while(it!=begin);

	}
}


bool QmGuiScreen::event(QmEvent *event){
	QM_UNUSED(event);
	return false;
}



