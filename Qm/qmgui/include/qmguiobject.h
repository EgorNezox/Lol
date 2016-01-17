/*
 * qmguiobject.h
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIOBJECT_H_
#define QMGUIOBJECT_H_

#include "qmobject.h"

enum QmGuiObjectType {qmguiDialog,qmguiVisual,qmguiScenario};

/*!Базовый класс объекта(не экрана)  GUI
 * Первоначально содержал переменную храняющую тип наследника, однако принципы иерархии
 * были несколько изменены и класс остался пустой заглушкой в иерархии.
 * Тем не менее он оставлен на случай необходимости внесения общего функционала*/
class QmGuiObject: public QmObject{
	public:
		QmGuiObject(QmGuiObjectType type, QmObject *parent = 0);
		virtual ~QmGuiObject();
		QmGuiObjectType getType();
	protected:
		virtual bool event(QmEvent *event)=0;
	private:
		QmGuiObjectType type;
		//todo Qm Макросы
};

#endif /* QMGUIOBJECT_H_ */
