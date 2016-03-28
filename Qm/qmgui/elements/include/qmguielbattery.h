/*
 * qmguielbattery.h
 *
 *  Created on: 29 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIELBATTERY_H_
#define QMGUIELBATTERY_H_

#include "gdisp.h"
#include "qmguielement.h"

class QmGuiVisualObject;

class QmGuiElBattery: public QmGuiElement{
	public:
		QmGuiElBattery(QmGuiElementParams *params, uint8_t charge, QmGuiVisualObject *parent_obj);
		void Draw();
		uint8_t charge;	//заряд в процентах
	protected:
		void CalcContentGeom();
};


#endif /* QMGUIELBATTERY_H_ */
