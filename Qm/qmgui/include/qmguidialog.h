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
#include <list>

class QmGuiKey;
class QmGuiElement;

/*!Базовый класс диалога GUI - графический объект реагирующий на нажатия кнопок*/
class QmGuiDialog: public QmGuiVisualObject{
	public:
		QmGuiDialog(QmGuiGeometry *area, QmObject *parent = 0);
		virtual ~QmGuiDialog();
		void keyHandler(QmGuiKey key);
		void assignKeyMapping(std::map<QmGuiKey,int> &map);
		void draw();
		virtual void actionHandler(int action)=0;

		//todo Qm Макросы
	protected:
		virtual bool event(QmEvent *event);
	private:
		virtual void updateInternalData()=0;
		static std::map<QmGuiKey,int> keymap;
		std::list<QmGuiElement*> element_list;
};


#endif /* QMGUIDIALOG_H_ */
