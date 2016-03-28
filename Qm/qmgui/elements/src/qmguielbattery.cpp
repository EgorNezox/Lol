/*
 * qmguielbattery.cpp
 *
 *  Created on: 29 янв. 2016 г.
 *      Author: Egor Dudyak
 */


#include "qmguielbattery.h"
#include "qmguielement_template.h"

#define BATTERY_HIGH_THRESHOLD	60
#define BATTERY_MID_THRESHOLD	30
#define BATTERY_W	14
#define BATTERY_H	22


QmGuiElBattery::QmGuiElBattery(QmGuiElementParams *params, uint8_t charge, QmGuiVisualObject *parent_obj):QmGuiElement(params, parent_obj){
	this->charge=charge;
	PrepareContent();
}


void QmGuiElBattery::Draw(){
	PrepareViewport();
	gsetcolorb(GENERAL_BACK_COLOR);
	gsetcolorf(GENERAL_FORE_COLOR);
	/*
	int16_t shpenek_w=content.W/2;		//альтернативный код для отрисовки шпенька батарейки, нужен для отрисовки батареек произвольного размера.
										//я написал его на всякий случай, подозреваю что он не пригодится
	if((shpenek_w%2)^(content.W%2)) --shpenek_w;	//хакерская штука для определния длины контакта
	GXT shpenek_x=content.x+(content.W-shpenek_w)/2;
	groundrect(shpenek_x, content.y, shpenek_x+shpenek_w-1,content.y,0,0);	//рисуем шпенек(контакт) батарейки
	*/
	groundrect(content->x+4, content->y, CONTENT_XE(content)-4,content->y,0,0);	//рисуем шпенек(контакт) батарейки
	groundrect(content->x, content->y+1, CONTENT_XE(content), CONTENT_YE(content), 0, GFRAME);	//рисуем корпус батарейки

	if(charge>BATTERY_HIGH_THRESHOLD){	//определяем цвет заряда батарейки
		gsetcolorb(BATTERY_HIGH_COLOR);
	}
	else{
		if(charge>BATTERY_MID_THRESHOLD){
			gsetcolorb(BATTERY_MID_COLOR);
		}
		else{
			gsetcolorb(BATTERY_LOW_COLOR);
		}
	}
	GYT battery_column_h=(GYT)(((int32_t)(content->H-3)*(int32_t)charge)/(int32_t)100);
	if(battery_column_h>0){
		groundrect(content->x+1, CONTENT_YE(content)-battery_column_h, CONTENT_XE(content)-1, CONTENT_YE(content)-1, 0, GFILL);//рисуем заряд батарейки
	}
}


void QmGuiElBattery::CalcContentGeom(){
	content->H=BATTERY_H;
	content->W=BATTERY_W;
}
