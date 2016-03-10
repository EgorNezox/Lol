/*
 * qmguiobject.cpp
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#include "qmguiobject.h"
#include "qmguiscreen.h"




QmGuiObject::QmGuiObject(QmGuiObjectType type, QmObject *parent):QmObject(parent){
	this->type=type;
	parent_screen=NULL;
};



QmGuiObject::~QmGuiObject(){};



QmGuiObjectType QmGuiObject::getType(){
	return type;
}



void QmGuiObject::deleteGuiObject(){
	if(parent_screen!=NULL){
		parent_screen->deleteGuiObkectFromStack(this);
	}
	this->deleteLater();
}

void QmGuiObject::setParentScreen(QmGuiScreen *screen){
	this->parent_screen=screen;
}
