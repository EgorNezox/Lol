/*
 * qmguidialog.cpp
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#include "qmguidialog.h"
#include "qmguikey.h"

//----------DEFINES------------

//----------TYPES--------------

//----------GLOBAL_VARS--------

//----------PROTOTYPES---------

//----------CODE---------------




QmGuiDialog::QmGuiDialog(QmGuiGeometry *area, QmObject *parent): QmGuiVisualObject(area, parent, qmguiDialog){

};

//-----------------------------

QmGuiDialog::~QmGuiDialog(){

}

//-----------------------------

bool QmGuiDialog::event(QmEvent *event){
	QM_UNUSED(event);
	return false;
}

//-----------------------------

void QmGuiDialog::keyHandler(QmGuiKey key){
	actionHandler(keymap[key]);	//todo assert если не найден
}

//-----------------------------

void QmGuiDialog::assignKeyMapping(std::map<QmGuiKey,int> &map){
	keymap=map;
}
