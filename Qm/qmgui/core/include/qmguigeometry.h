/*
 * qmguigeometry.h
 *
 *  Created on: 28 янв. 2016 г.
 *      Author: Egor Dudyak
 */

#ifndef QMGUIGEOMETRY_H_
#define QMGUIGEOMETRY_H_

#define CONTENT_XE(content) (content->x+content->W-1)
#define CONTENT_YE(content) (content->y+content->H-1)
#define GEOM_W(area)	((area->xe-area->xs+1))
#define GEOM_H(area)	((area->ye-area->ys+1))

struct QmGuiGeometry{
	unsigned int xs;
	unsigned int ys;
	unsigned int xe;
	unsigned int ye;
};

struct QmGuiContentSize{
	unsigned int x;
	unsigned int y;
	unsigned int W;
	unsigned int H;
};

struct QmGuiMargins{
	unsigned int left;
	unsigned int right;
	unsigned int top;
	unsigned int bottom;
};

struct QmGuiAlignment{
	enum  AlignmentH{
		alignLeft,
		alignRight,
		alignHCenter
	};

	enum AlignmentV{
		alignTop,
		alignBottom,
		alignVCenter
	};
	AlignmentH align_h;
	AlignmentV align_v;
};


#endif /* QMGUIGEOMETRY_H_ */
