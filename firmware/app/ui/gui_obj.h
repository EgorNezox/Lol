/*
 * gui_obj.h
 *
 *  Created on: 10 дек. 2015 г.
 *      Author: user
 */

#ifndef GUI_OBJ_H_
#define GUI_OBJ_H_


class GUI_Obj{
	public:
		GUI_Obj(MoonsGeometry *area){
			this->area=*area;
		}
		MoonsGeometry area;
		virtual ~GUI_Obj(){}
	private:

};




#endif /* GUI_OBJ_H_ */
