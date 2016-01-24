/*
 * qmguiscenario.cpp
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#include "qmguiscenario.h"


//----------DEFINES------------

//----------TYPES--------------

//----------GLOBAL_VARS--------

//----------PROTOTYPES---------

//----------CODE---------------

QmGuiScenario::QmGuiScenario(QmGuiScreen *sub_screen, QmObject *parent):QmGuiObject(qmguiScenario,parent){
	this->sub_screen=sub_screen;
}

//-----------------------------

QmGuiScenario::~QmGuiScenario(){

}

//-----------------------------

void QmGuiScenario::RenderSubScreen(){
	sub_screen->renderScreen();
}

//-----------------------------

bool QmGuiScenario::event(QmEvent *event){
	QM_UNUSED(event);
	return false;
}
