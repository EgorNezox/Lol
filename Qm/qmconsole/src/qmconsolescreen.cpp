/**
 ******************************************************************************
 * @file    qmconsolescreen.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    20.05.2016
 *
 ******************************************************************************
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include "qm.h"
#include "qmdebug.h"
#include "gdisp.h"

#include "qmconsolescreen.h"

#ifndef __MINGW32__
static ssize_t qmConsoleStreamWriteFunction(void *__cookie, const char *__buf, size_t __n);
#endif
static void qmConsoleOutputChar(const char c);
static void qmConsoleRedraw();
static void qmConsoleClearLine(int line);
static void qmConsoleNewLine();
static void qmConsoleProcessLineFeed();

static bool qm_cscreen_initialized = false;
#ifndef __MINGW32__
static FILE *qm_cscreen_stream;
#endif
static struct {
	int symbols_per_line, line_count;
	char *buffer;
	int top_line, cur_line, cur_s;
	bool pending_line_feed;
} qm_cscreen_cb;


void QmConsoleScreen::init(unsigned int top_margin, unsigned int bottom_margin, unsigned int left_margin, unsigned int right_margin) {
	QM_ASSERT(((top_margin + bottom_margin) < GDISPH) && (left_margin + right_margin) < GDISPW);

#ifndef __MINGW32__
	cookie_io_functions_t io_fuinctions;
	io_fuinctions.read = NULL;
	io_fuinctions.write = qmConsoleStreamWriteFunction;
	io_fuinctions.seek = NULL;
	io_fuinctions.close = NULL;
	qm_cscreen_stream = fopencookie(NULL, "w", io_fuinctions);
	QM_ASSERT(qm_cscreen_stream);
	setvbuf(qm_cscreen_stream, NULL, _IONBF, 0);
#endif

	ginit();

	gselvp(0);
	gsetcolorb(G_WHITE);
	gsetcolorf(G_BLACK);
	gsetvp(0, 0, GDISPW-1, GDISPH-1);
	groundrect(0, 0, GDISPW-1, GDISPH-1, 0, GFILL);

	gsetvp(left_margin, top_margin, GDISPW-1-right_margin, GDISPH-1-bottom_margin);
	gselfont(&SYSFONT);

	qm_cscreen_cb.symbols_per_line = (GDISPW - left_margin - right_margin)/ggetfw();
	qm_cscreen_cb.line_count = (GDISPH - top_margin - bottom_margin)/ggetfh();
	QM_ASSERT((qm_cscreen_cb.symbols_per_line > 0) && (qm_cscreen_cb.line_count > 0));
	qm_cscreen_cb.buffer = new char[(qm_cscreen_cb.symbols_per_line+1)*qm_cscreen_cb.line_count];
	for (int line = 0; line < qm_cscreen_cb.line_count; line++) {
		qm_cscreen_cb.buffer[qm_cscreen_cb.symbols_per_line + line*(qm_cscreen_cb.symbols_per_line+1)] = 0;
		qmConsoleClearLine(line);
	}
	qm_cscreen_cb.top_line = 0;
	qm_cscreen_cb.cur_line = 0;
	qm_cscreen_cb.cur_s = 0;
	qm_cscreen_cb.pending_line_feed = false;
	qm_cscreen_initialized = true;
}

void QmConsoleScreen::oputc(const char c) {
	QM_ASSERT(qm_cscreen_initialized);
	qmConsoleOutputChar(c);
}

void QmConsoleScreen::oprintf(const char * format, ...) {
	QM_ASSERT(qm_cscreen_initialized);
	va_list args;
	va_start(args, format);
#ifndef __MINGW32__
	vfprintf(qm_cscreen_stream, format, args);
#else
	char *buf;
	int size = vasprintf(&buf, format, args);
	QM_ASSERT(size >= 0);
	for (int i = 0; i < size; i++)
		qmConsoleOutputChar(buf[i]);
	free(buf);
#endif
	va_end(args);
}

#ifndef __MINGW32__
static ssize_t qmConsoleStreamWriteFunction(void *__cookie, const char *__buf, size_t __n) {
	QM_UNUSED(__cookie);
	for (size_t i = 0; i < __n; i++)
		qmConsoleOutputChar(__buf[i]);
	return __n;
}
#endif

static void qmConsoleOutputChar(const char c) {
#ifndef NDEBUG
	putchar(c);
#endif
	switch (c) {
	case '\r': {
		qm_cscreen_cb.cur_s = 0;
		break;
	}
	case '\n': {
		qmConsoleProcessLineFeed();
		qmConsoleNewLine();
		break;
	}
	case 0: {
		QM_ASSERT(0);
		break;
	}
	default: {
		if (qm_cscreen_cb.cur_s == qm_cscreen_cb.symbols_per_line) {
			qm_cscreen_cb.cur_s = 0;
			qmConsoleNewLine();
		}
		qmConsoleProcessLineFeed();
		int s = qm_cscreen_cb.cur_s;
		int line = qm_cscreen_cb.top_line + qm_cscreen_cb.cur_line;
		if (line < qm_cscreen_cb.line_count) {
			gsetcpos(s, line);
		} else {
			gsetcpos(s, qm_cscreen_cb.line_count-1);
			line -= qm_cscreen_cb.line_count;
		}
		qm_cscreen_cb.buffer[s + line*(qm_cscreen_cb.symbols_per_line+1)] = c;
		gputch(c);
		qm_cscreen_cb.cur_s++;
		break;
	}
	}
}

static void qmConsoleRedraw() {
	gclrvp();
	gsetcpos(0, 0);
	int line = qm_cscreen_cb.top_line;
	bool wrapped = false;
	while (!wrapped || (line < qm_cscreen_cb.top_line)) {
		gputs(qm_cscreen_cb.buffer + (qm_cscreen_cb.symbols_per_line+1)*line);
		line++;
		if (line == qm_cscreen_cb.line_count) {
			line = 0;
			wrapped = true;
		}
	}
}

static void qmConsoleClearLine(int line) {
	for (int s = 0; s < qm_cscreen_cb.symbols_per_line; s++)
		qm_cscreen_cb.buffer[s + line*(qm_cscreen_cb.symbols_per_line+1)] = '.';
}

static void qmConsoleNewLine() {
	if (qm_cscreen_cb.cur_line < (qm_cscreen_cb.line_count - 1)) {
		qm_cscreen_cb.cur_line++;
	} else {
		qmConsoleClearLine(qm_cscreen_cb.top_line);
		qm_cscreen_cb.top_line++;
		if (qm_cscreen_cb.top_line == qm_cscreen_cb.line_count)
			qm_cscreen_cb.top_line = 0;
		qm_cscreen_cb.pending_line_feed = true;
	}
}

static void qmConsoleProcessLineFeed() {
	if (qm_cscreen_cb.pending_line_feed) {
		qm_cscreen_cb.pending_line_feed = false;
		qmConsoleRedraw();
	}
}
