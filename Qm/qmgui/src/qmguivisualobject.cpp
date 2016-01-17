/*
 * qmguivisualobject.cpp
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#include "qmguivisualobject.h"

//----------DEFINES------------

//----------TYPES--------------

//----------GLOBAL_VARS--------

//----------PROTOTYPES---------

//----------CODE---------------

QmGuiVisualObject::QmGuiVisualObject(QmGuiGeometry *area, QmObject *parent, QmGuiObjectType type):QmGuiObject(type, parent){
	this->area=*area;
}

//-----------------------------

QmGuiVisualObject::~QmGuiVisualObject(){

}

//-----------------------------

bool QmGuiVisualObject::event(QmEvent *event){
	QM_UNUSED(event);
	return false;
}

