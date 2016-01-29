/*
 * qmguielicon.cpp
 *
 *  Created on: 29 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#include "qmguielement.h"
#include "qmguielicon.h"


QmGuiElIcon::QmGuiElIcon(QmGuiElementParams *params, PGSYMBOL icon, QmGuiVisualObject *parent_obj):QmGuiElement(params, parent_obj){
	this->icon=icon;
	PrepareContent();
}


void QmGuiElIcon::CalcContentGeom(){
	content->H=gsymh(icon);
	content->W=gsymw(icon);
}


void QmGuiElIcon::Draw(){
	PrepareViewport();
	gputsym(content->x,content->y, icon);
}
