/*
 * qmguiramtexgeometry.h
 *
 *  Created on: 29 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIRAMTEXGEOMETRY_H_
#define QMGUIRAMTEXGEOMETRY_H_


#include "gdisp.h"

struct QmGuiRamtexGeometry{
	GXT xs;
	GYT ys;
	GXT xe;
	GYT ye;
};

struct QmGuiRamtexMargins{
	GXT left;
	GXT right;
	GYT top;
	GYT bottom;
};



#endif /* QMGUIRAMTEXGEOMETRY_H_ */
