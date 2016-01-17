/*
 * qmguidialog.h
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIDIALOG_H_
#define QMGUIDIALOG_H_

#include "qmguivisualobject.h"

/*!Базовый класс диалога GUI - графический объект реагирующий на нажатия кнопок*/
class QmGuiDialog: public QmGuiVisualObject{
	public:
		QmGuiDialog(QmGuiGeometry *area, QmObject *parent = 0);
		virtual ~QmGuiDialog();
		virtual void Draw()=0;
			//todo Qm Макросы
	protected:
		virtual bool event(QmEvent *event);
};


#endif /* QMGUIDIALOG_H_ */
