/*
 * qmguikey.h
 *
 *  Created on: 25 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIKEY_H_
#define QMGUIKEY_H_

struct QmGuiKey{
	enum {kmt_matrixkb,kmt_pushbutton}type;
	union{
		struct{
			int key_id;		//key id from paltform_hw_map.h
			int press_type;
		}matrix_key;
		struct{
			int pushbutton_id;	//resource_id
		}pusbutton;
	};

	bool operator < (const QmGuiKey & b) const{
		if(this->type<b.type) return true;
		if(this->type>b.type) return false;
		switch(this->type){
			case kmt_matrixkb:
				if(this->matrix_key.key_id<b.matrix_key.key_id) return true;
				if(this->matrix_key.key_id>b.matrix_key.key_id) return false;
				if(this->matrix_key.press_type<b.matrix_key.press_type) return true;
				if(this->matrix_key.press_type>b.matrix_key.press_type) return false;
				break;
			case kmt_pushbutton:
				if(this->pusbutton.pushbutton_id<b.pusbutton.pushbutton_id) return true;
				if(this->pusbutton.pushbutton_id>b.pusbutton.pushbutton_id) return false;
				break;
			default:
				break;
		}
		return false;
	}
};

#endif /* QMGUIKEY_H_ */
