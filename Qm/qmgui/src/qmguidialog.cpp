/*
 * qmguidialog.cpp
 *
 *  Created on: 21 дек. 2015 г.
 *      Author: Egor Dudyak
 */


#include "qmguidialog.h"

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
