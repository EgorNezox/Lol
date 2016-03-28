/*
 * qmguiellabel.cpp
 *
 *  Created on: 29 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#include <stdio.h>
#include <string.h>
#include "../../core/src/qmguiramtexgeometry.h"
#include "qmguielement.h"
#include "qmguiellabel.h"


#define COMMON_ELEMENT_VP_MODE	(GNORMAL)

QmGuiElLabel::QmGuiElLabel(QmGuiLabelParams *params, char *text, QmGuiVisualObject *parent_obj):QmGuiElement(&(params->elparams), parent_obj){
	font=params->font;
	transparent=params->transparent;
	color_sch=params->color_sch;
	SetText(text);
	if(text!=0)PrepareContent();
}


void QmGuiElLabel::SetText(char *text){
	if(text!=0){
		this->text=text;
	}
}


void QmGuiElLabel::Draw(){
	if(text!=0){
		PrepareContent();
		PrepareViewport();
		gselfont(font);
		gsetcolorb(color_sch.background);
		gsetcolorf(color_sch.foreground);
		if(!transparent){
			gsetmode(COMMON_ELEMENT_VP_MODE);
			groundrect(0,0,GEOM_W(el_geom)-1,GEOM_H(el_geom)-1,0,GFILL);
		}
		else {
			gsetmode(COMMON_ELEMENT_VP_MODE | GTRANSPERANT);
		}
		gsetpos(content->x, CONTENT_YE(content));
		gputs(text);
		if(transparent){
			gsetmode(COMMON_ELEMENT_VP_MODE);
		}
	}
}


void QmGuiElLabel::CalcContentGeom(){
	int32_t i = 0, size=strlen(text);
	content->W=0;
	content->H=0;
	gselfont(font);
	for (i = 0; i < size; i++)		//Вычисляем ширину и высоту
	{
		content->W += ggetsymw(text[i]);
	}
	content->H=ggetfh();

}
