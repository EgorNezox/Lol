/*
 * texts.h
 *
 *  Created on: 14 дек. 2015 г.
 *      Author: user
 */

#ifndef TEXTS_H_
#define TEXTS_H_


#define LANG_COUNT 1
#define MAIN_MENU_MAX_LIST_SIZE 5
#define _LIST_SIZE 5
#define SETTINGS_LIST_SIZE 2

extern char *mode_txt[];
extern char *disabled_ch_txt;
extern char ch_zero_letter;
extern char ch_open_letter;
extern char ch_closed_letter;
extern char ch_invalid_letter;

extern const char* freq_khz;
extern const char* freq_hz;

extern const char* trans;
extern const char* callTitle[];

extern char * ok_texts[LANG_COUNT];
extern char * missing_ch_table_txt[LANG_COUNT];
extern char * missing_open_ch_txt[LANG_COUNT];
extern char * ch_table_mismatch_txt[LANG_COUNT];

extern const char *mainMenu[];
extern const char *callSubMenu[];
extern const char *commandsSubMenu[];
extern const char *smplSubMenu[];
extern const char *twSubMenu[];
extern const char *smsSubMenu[];
extern const char *groupCommandsSubMenu[];
extern const char *groupCommandsSimplSubMenu[];
extern const char *reciveSubMenu[];
extern const char *dataSubMenu[];
extern const char *dataSubSubMenu[];
extern const char *settingsSubMenu[];
extern const char *dateAndTimeSubMenu[];
extern const char *setDateOrTime[];
extern const char *setConnParam[];
extern const char *useScanMenu[];

#endif /* TEXTS_H_ */
