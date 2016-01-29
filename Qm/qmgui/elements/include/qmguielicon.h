/*
 * qmguielicon.h
 *
 *  Created on: 29 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIELICON_H_
#define QMGUIELICON_H_

#include "gdisp.h"


class QmGuiVisualObject;

class QmGuiElIcon: public QmGuiElement{
	public:
		QmGuiElIcon(QmGuiElementParams *params, PGSYMBOL icon, QmGuiVisualObject *parent_obj);
		void Draw();
		PGSYMBOL icon;
	protected:
		void CalcContentGeom();
};


#endif /* QMGUIELICON_H_ */
