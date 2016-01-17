/*
 * qmguiobject.cpp
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#include "qmguiobject.h"


//----------DEFINES------------

//----------TYPES--------------

//----------GLOBAL_VARS--------

//----------PROTOTYPES---------

//----------CODE---------------


QmGuiObject::QmGuiObject(QmGuiObjectType type, QmObject *parent):QmObject(parent){
	this->type=type;
};

//-----------------------------

QmGuiObject::~QmGuiObject(){};

//-----------------------------

QmGuiObjectType QmGuiObject::getType(){
	return type;
}
