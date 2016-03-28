/*
 * qmguiellabel.h
 *
 *  Created on: 29 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIELLABEL_H_
#define QMGUIELLABEL_H_

#include "qmguicolorscheme.h"
#include "qmguielement.h"


struct QmGuiLabelParams{
	QmGuiElementParams elparams;
	PGFONT font;
	QmGuiColorScheme color_sch;
	bool transparent;
};

class QmGuiVisualObject;

class QmGuiElLabel: public QmGuiElement{
	public:
		QmGuiElLabel(QmGuiLabelParams *params, char *text, QmGuiVisualObject *parent_obj);
		void Draw();
		void SetText(char *text);
	private:
		PGFONT font;
		QmGuiColorScheme color_sch;
		bool transparent;
	protected:
		void CalcContentGeom();
		char *text;
};




#endif /* QMGUIELLABEL_H_ */
