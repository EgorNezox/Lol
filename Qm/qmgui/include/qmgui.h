/*
 * qmgui.h
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUI_H_
#define QMGUI_H_

#include "qmguiscreen.h"

class QmGui{
	public:
		QmGui();
		~QmGui();
		void markCurScreenToRender();
		void switchToScren(QmGuiScreen *screen, bool reset_screen);
		QmGuiScreen * getCurScreen();
		void renderCurScreen();
	private:
		QmGuiScreen *cur_screen;
		bool cur_scr_need_render;
		static QmGui* self;
};

#endif /* QMGUI_H_ */
