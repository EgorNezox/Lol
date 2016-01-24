/*
 * qmguiscreen.h
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUISCREEN_H_
#define QMGUISCREEN_H_

#include <list>
#include "qmobject.h"

class QmGuiObject;

/*!Класс описывающий экран(стек диалогов) и реализующий управление им*/
class QmGuiScreen: public QmObject{
	public:
		QmGuiScreen(QmObject *parent = 0);
		~QmGuiScreen();
		/*!Отрисовывает режим.
		 * Нельзя вызывать напрямую!
		 */
		void renderScreen();
		void appendObject(QmGuiObject *gui_obj, bool draw_if_non_active);
		void keyProcessor(int key_id);
		void resetToInitState();
	protected:
		virtual bool event(QmEvent *event);
	private:
		struct ScreenStackInst{
			QmGuiObject *gui_obj;
			bool draw_if_non_active;
		};
		std::list <ScreenStackInst> stack;
		int stack_size;
		void deleteGuiObkectFromStack(QmGuiObject *gui_obj);
		void drawProcessor(QmGuiObject *gui_obj);
		virtual void initScreen()=0;
		friend QmGuiObject;

		//todo макросы Qm
};


#endif /* QMGUISCREEN_H_ */
