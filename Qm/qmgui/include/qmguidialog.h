/*
 * qmguidialog.h
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIDIALOG_H_
#define QMGUIDIALOG_H_

#include "qmguivisualobject.h"

#include <map>

class QmGuiKey;

/*!Базовый класс диалога GUI - графический объект реагирующий на нажатия кнопок*/
class QmGuiDialog: public QmGuiVisualObject{
	public:
		QmGuiDialog(QmGuiGeometry *area, QmObject *parent = 0);
		virtual ~QmGuiDialog();
		void keyHandler(QmGuiKey key);
		void assignKeyMapping(std::map<QmGuiKey,int> &map);
		virtual void actionHandler(int action)=0;
		virtual void Draw()=0;

		//todo Qm Макросы
	protected:
		virtual bool event(QmEvent *event);
	private:
		static std::map<QmGuiKey,int> keymap;
};


#endif /* QMGUIDIALOG_H_ */
