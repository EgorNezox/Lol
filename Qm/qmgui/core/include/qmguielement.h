/*
 * qmguielement.h
 *
 *  Created on: 28 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIELEMENT_H_
#define QMGUIELEMENT_H_

#include "qmobject.h"
#include "qmguigeometry.h"

struct RamtexGeometry;
struct RamtexMargins;
struct RamtexAlignment;
class QmGuiVisualObject;

struct QmGuiElementParams{
	QmGuiMargins margins;
	QmGuiAlignment align;
	QmGuiGeometry geom;
};

class QmGuiElement: public QmObject{
	public:
		QmGuiElement(QmGuiElementParams *el_params, QmGuiVisualObject *parent_obj=0);
		virtual ~QmGuiElement();
		virtual void Draw() =0;
		void PrepareContent(); //Должна вызываться в конце конструктора конкретного элемента!
		void PrepareViewport();	//Выбирает текущим вьпорт элемента и настраивает его размеры в соответствии с el_geom, рекомендуется к использованию в отрисовке.
		/*! Возвращает координаты контента относительно координат диалога
		 * Внимание! Вызывать только если content уже вычислен
		 */
		QmGuiGeometry GetContentGeomOnElem();
		QmGuiContentSize *content;	//Размер и относительные координаты содержимого относительно области элемента. Вычисляются.
		RamtexGeometry *el_geom;	//Область элемента. Абсолютные координаты
		RamtexGeometry *geom;	//Координаты элемента относительно диалога
		RamtexMargins *margins;
		QmGuiAlignment align;
	private:
		void AlignContent();
	protected:
		virtual void CalcContentGeom() =0;

};

#endif /* QMGUIELEMENT_H_ */
