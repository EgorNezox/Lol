/*
 * qmguivisualobject.h
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIVISUALOBJECT_H_
#define QMGUIVISUALOBJECT_H_


#include "qmguiobject.h"
#include "gdisp.h"	//todo убрать

struct QmGuiGeometry{	//todo подумать как избавить от GXT, GYT
	GXT xs;
	GYT ys;
	GXT xe;
	GYT ye;
};

/*!Базовый класс видимого графического объекта.
 * 	area - Область занимаемая визуальным объекстом GUI. Абсолютные координаты пикселей дисплея.*/
class QmGuiVisualObject: public QmGuiObject{
	public:
		QmGuiVisualObject(QmGuiGeometry *area, QmObject *parent = 0, QmGuiObjectType type=qmguiVisual);
		virtual ~QmGuiVisualObject();
		virtual void Draw()=0;
	protected:
		virtual bool event(QmEvent *event);
	private:
		QmGuiGeometry area;
		//todo Qm Макросы
};


#endif /* QMGUIVISUALOBJECT_H_ */
