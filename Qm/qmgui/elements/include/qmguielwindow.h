/*
 * qmguielwindow.h
 *
 *  Created on: 29 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIELWINDOW_H_
#define QMGUIELWINDOW_H_

#include "qmguicolorscheme.h"
#include "qmguielement.h"


struct QmGuiWindowParams{
		QmGuiElementParams elparams;
		QmGuiColorScheme color_sch;
		int frame_thick;
		bool round_corners;		// если 1, то углы будут скруглены с радиусов 5. Иначе будут квадратными.
};

class QmGuiVisualObject;

class QmGuiElWindow: public QmGuiElement{
	public:
		QmGuiElWindow(QmGuiWindowParams *params, QmGuiVisualObject *parrent_obj);
		void Draw();
	protected:
		void CalcContentGeom();
	private:
		QmGuiColorScheme color_sch;
		int frame_thick;
		bool round_corners;
};


#endif /* QMGUIELWINDOW_H_ */
