/*
 * ramtexgeometry.h
 *
 *  Created on: 28 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef RAMTEXGEOMETRY_H_
#define RAMTEXGEOMETRY_H_

#include "gdisp.h"

struct RamtexGeometry{
	GXT xs;
	GYT ys;
	GXT xe;
	GYT ye;
};

struct RamtexMargins{
	GXT left;
	GXT right;
	GYT top;
	GYT bottom;
};


#endif /* RAMTEXGEOMETRY_H_ */
