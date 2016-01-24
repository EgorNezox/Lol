/*
 * qmgui.cpp
 *
 *  Created on: 23 янв. 2016 г.
 *      Author: Egor Dudyak
 */


#include "qmgui.h"
#include "qmdebug.h"

QmGui* QmGui::self=0;

QmGui::QmGui(){
	cur_screen=NULL;
	cur_scr_need_render=0;
	QM_ASSERT(self==0);
	self=this;
}



QmGui::switchToScren(QmGuiScreen *screen, bool reset_screen){
	cur_screen=screen;
	if(reset_screen){
		cur_screen->ResetToInitState();
	}
	markCurScreenToRender();
}


QmGui::markCurScreenToRender(){
	cur_scr_need_render=true;
}


QmGuiScreen * QmGui::getCurScreen(){
	return cur_screen;
}


QmGui::renderCurScreen(){
	if(cur_scr_need_render){
		cur_screen->RenderScreen();
		cur_scr_need_render=false;
	}
}
