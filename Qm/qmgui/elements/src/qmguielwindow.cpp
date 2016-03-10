/*
 * qmguielwindow.cpp
 *
 *  Created on: 29 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#include "../../core/src/qmguiramtexgeometry.h"
#include "qmguielwindow.h"


#define GLOBAL_WINDOW_ARC	5

QmGuiElWindow::QmGuiElWindow(QmGuiWindowParams *params, QmGuiVisualObject *parrent_obj):QmGuiElement(&(params->elparams),parrent_obj){
	frame_thick=params->frame_thick;
	color_sch=params->color_sch;
	round_corners=params->round_corners;

}

void QmGuiElWindow::Draw(){
	int32_t i=0;
	PrepareContent();
	PrepareViewport();
	gsetcolorf(color_sch.foreground);
	gsetcolorb(color_sch.background);

	GXT arc=0;
	if(round_corners){
		arc=GLOBAL_WINDOW_ARC;
	}

	for(i=0;i<frame_thick;++i){
		groundrect(content->x+i,content->y+i, CONTENT_XE(content)-i, CONTENT_YE(content)-i,arc,GLINE);
	}

	groundrect(content->x+i,content->y+i, CONTENT_XE(content)-i, CONTENT_YE(content)-i,arc,GFILL);
}

void QmGuiElWindow::CalcContentGeom(){
	content->H=GEOM_H(el_geom);
	content->W=GEOM_W(el_geom);
}
