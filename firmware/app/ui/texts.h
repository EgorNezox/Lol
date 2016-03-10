/*
 * texts.h
 *
 *  Created on: 14 дек. 2015 г.
 *      Author: user
 */

#ifndef TEXTS_H_
#define TEXTS_H_


#define LANG_COUNT 1
#define MAIN_MENU_LIST_SIZE 4
#define _LIST_SIZE 5
#define SETTINGS_LIST_SIZE 2

extern char *mode_txt;
extern char *disabled_ch_txt;
extern char ch_zero_letter;
extern char ch_open_letter;
extern char ch_closed_letter;
extern char ch_invalid_letter;

extern char * ok_texts[LANG_COUNT];
extern char * missing_ch_table_txt[LANG_COUNT];
extern char * missing_open_ch_txt[LANG_COUNT];
extern char * ch_table_mismatch_txt[LANG_COUNT];

extern char *mainMenu[];
extern char *callSubMenu[];
extern char *commandsSubMenu[];
extern char *smplSubMenu[];
extern char *twSubMenu[];
extern char *smsSubMenu[];
extern char *groupCommandsSubMenu[];
extern char *reciveSubMenu[];
extern char *dataSubMenu[];
extern char *recvSubMenu[];
extern char *sendSubMenu[];
extern char *saveSubMenu[];
extern char *settingsSubMenu[];
extern char *dateAndTimeSubMenu[];
extern char *setTimeSubMenu[];
extern char *modeSubMenu[];

#endif /* TEXTS_H_ */
