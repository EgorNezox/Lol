/*
 * qmguidialog.cpp
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */

#include "qmguidialog.h"
#include "qmguikey.h"
#include "qmguielement.h"

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
	std::map<QmGuiKey,int>::iterator it=keymap.find(key);
	if(it!=keymap.end()){
		actionHandler(it->second);
	}
}

//-----------------------------

void QmGuiDialog::assignKeyMapping(std::map<QmGuiKey,int> &map){
	keymap=map;
}

void QmGuiDialog::draw(){
	updateInternalData();
	std::list<QmGuiElement*>::iterator  it, end;
	if(element_list.size()>0){
		for(it=element_list.begin(), end=element_list.end();it!=end;it++){
			(*it)->Draw();
		}
	}
}
