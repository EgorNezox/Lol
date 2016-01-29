/*
 * qmguivisualobject.cpp
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#include "qmdebug.h"
#include "qmguivisualobject.h"
#include "ramtexgeometry.h"
#include "gdisp.h"



QmGuiVisualObject::QmGuiVisualObject(QmGuiGeometry *area, QmObject *parent, QmGuiObjectType type):QmGuiObject(type, parent){
	QM_ASSERT(area->xs>GDISPW-1);
	QM_ASSERT(area->xe>GDISPW-1);
	QM_ASSERT(area->ys>GDISPH-1);
	QM_ASSERT(area->ye>GDISPH-1);

	this->area=new RamtexGeometry;
	this->area->xs=(GXT)(area->xs);
	this->area->ys=(GYT)(area->ys);
	this->area->xe=(GXT)(area->xe);
	this->area->ye=(GYT)(area->ye);
}


QmGuiVisualObject::~QmGuiVisualObject(){
	delete area;
}


RamtexGeometry * QmGuiVisualObject::getObjectArea(){
	return area;
}


bool QmGuiVisualObject::event(QmEvent *event){
	QM_UNUSED(event);
	return false;
}

