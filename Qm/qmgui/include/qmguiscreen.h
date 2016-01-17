/*
 * qmguiscreen.h
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUISCREEN_H_
#define QMGUISCREEN_H_

#include <list>
#include "qmguiobject.h"

/*!Класс описывающий экран(стек диалогов) и реализующий управление им*/
class QmGuiScreen: public QmObject{
	public:
		QmGuiScreen(QmObject *parent = 0);
		~QmGuiScreen();
		/*!Отрисовывает режим.
		 * Нельзя вызывать напрямую!
		 */
		void RenderScreen();
		void AppendObject(QmGuiObject *obj_copy, bool draw_if_non_active);
		//int32_t GetObjectNum(Moons_Obj *obj_copy);	//todo delete???
		//Moons_Obj * GetTopObj();		//todo delete???
		void KeyProcessor(int key_id);
		void ResetToInitState();
	protected:
		virtual bool event(QmEvent *event);
	private:
		struct ScreenStackInst{
			QmGuiObject *gui_obj;
			bool draw_if_non_active;
		};
		std::list <ScreenStackInst> stack;
		int stack_size;
		void DrawProcessor(QmGuiObject *gui_obj);
		virtual void InitScreen();	//функция инициализации дисплея //todo =0 ?
};


#endif /* QMGUISCREEN_H_ */
