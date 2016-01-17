/*
 * qmguiscenario.h
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUISCENARIO_H_
#define QMGUISCENARIO_H_

#include "qmguiscreen.h"

/*! Класс реализует механизм абстрактных сценариев(наборов диалогов связанных логикой)
 *  Для реализации логики предлагается используется автомат состояний state_machine, см. state_machine.h*/
class QmGuiScenario: public QmGuiObject{
	public:
		QmGuiScenario(QmGuiScreen *sub_screen, QmObject *parent=0);
		virtual ~QmGuiScenario();
		void RenderSubScreen();
	protected:
		QmGuiScreen *sub_screen;
		virtual bool event(QmEvent *event);
};

//-----------------------------


#endif /* QMGUISCENARIO_H_ */
