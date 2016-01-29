/*
 * qmguielement.cpp
 *
 *  Created on: 28 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#include "qmdebug.h"
#include "qmguielement.h"
#include "qmguivisualobject.h"
#include "ramtexgeometry.h"
#include "gdisp.h"



#define VP_COMMON	0



QmGuiElement::QmGuiElement(QmGuiElementParams *el_params, QmGuiVisualObject *parent_obj):QmObject((QmObject*)parent_obj){
	QM_ASSERT(parent_obj);
	QM_ASSERT(el_params->geom.xs>GDISPW-1);
	QM_ASSERT(el_params->geom.xe>GDISPW-1);
	QM_ASSERT(el_params->geom.ys>GDISPH-1);
	QM_ASSERT(el_params->geom.ye>GDISPH-1);
	QM_ASSERT(el_params->margins.left>GDISPW-1);
	QM_ASSERT(el_params->margins.right>GDISPW-1);
	QM_ASSERT(el_params->margins.top>GDISPH-1);
	QM_ASSERT(el_params->margins.bottom>GDISPH-1);

	RamtexGeometry *parent_geom=parent_obj->getObjectArea();

	this->geom=new RamtexGeometry;
	this->el_geom=new RamtexGeometry;
	this->margins=new RamtexMargins;
	this->content=new QmGuiContentSize;

	this->geom->xs=(GXT)(el_params->geom.xs);
	this->geom->ys=(GYT)(el_params->geom.ys);
	this->geom->xe=(GXT)(el_params->geom.xe);
	this->geom->ye=(GYT)(el_params->geom.ye);

	this->el_geom->xs=this->geom->xs+parent_geom->xs;
	this->el_geom->ys=this->geom->ys+parent_geom->ys;
	this->el_geom->xe=this->geom->xe+parent_geom->xs;
	this->el_geom->ye=this->geom->ye+parent_geom->ys;

	this->margins->left=(GXT)(el_params->margins.left);
	this->margins->right=(GXT)(el_params->margins.right);
	this->margins->top=(GYT)(el_params->margins.top);
	this->margins->bottom=(GYT)(el_params->margins.bottom);

	this->align=el_params->align;

	content->x=0;
	content->y=0;
	content->H=0;
	content->W=0;
}



QmGuiElement::~QmGuiElement(){
	delete geom;
	delete el_geom;
	delete margins;
	delete content;
}


void QmGuiElement::PrepareContent(){
	CalcContentGeom();
	AlignContent();
}


void QmGuiElement::PrepareViewport(){
	gselvp(VP_COMMON);
	gsetvp(el_geom->xs,el_geom->ys,el_geom->xe, el_geom->ye);
}


QmGuiGeometry QmGuiElement::GetContentGeomOnElem(){
	QmGuiGeometry abs_content;
	abs_content.xs=content->x+geom->xs;
	abs_content.ys=content->y+geom->ys;
	abs_content.xe=abs_content.xs+content->W-1;
	abs_content.ye=abs_content.ys+content->H-1;
	return abs_content;
}



void QmGuiElement::AlignContent(){	//В зависимости от типа выравнивания, вычисляем дополнительное смещение онтента внутри элемента
	content->x=margins->left;
	content->y=margins->top;
	switch(align.align_h){
		case QmGuiAlignment::alignLeft:
			break;
		case QmGuiAlignment::alignHCenter:
			content->x+=(GEOM_W(el_geom)- margins->right-margins->left-content->W)/2;
			break;
		case QmGuiAlignment::alignRight:
			content->x+=(GEOM_W(el_geom)- margins->right-margins->left-content->W);
			break;
		default:
			break;
	}

	switch(align.align_v){
		case QmGuiAlignment::alignTop:
			break;
		case QmGuiAlignment::alignVCenter:
			content->y+=(GEOM_H(el_geom)- margins->top-margins->bottom-content->H)/2;
			break;
		case QmGuiAlignment::alignBottom:
			content->y+=(GEOM_H(el_geom)-margins->top-margins->bottom-content->H);
			break;
		default:
			break;
	}
}
