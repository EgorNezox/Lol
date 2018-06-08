/**
 ******************************************************************************
 * @file    qmconsolescreen.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    20.05.2016
 *
 ******************************************************************************
 */

#ifndef QMCONSOLESCREEN_H_
#define QMCONSOLESCREEN_H_

class QmConsoleScreen
{
public:
	static void init(unsigned int top_margin, unsigned int bottom_margin, unsigned int left_margin, unsigned int right_margin);
	static void oputc(const char c);
	static void oputstr(const char* s);
	static void oprintf(const char * format, ...);
	static void clearScreen();
};

#endif /* QMCONSOLESCREEN_H_ */
