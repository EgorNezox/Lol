/*
 * texts.h
 *
 *  Created on: 14 дек. 2015 г.
 *      Author: user
 */

#ifndef TEXTS_H_
#define TEXTS_H_


#define LANG_COUNT 1
#define MAIN_MENU_MAX_LIST_SIZE 6
#define _LIST_SIZE 5
#define SETTINGS_LIST_SIZE 2

extern const char *test_Pass;

extern char *mode_txt[];
extern char *disabled_ch_txt;
extern char ch_zero_letter;
extern char ch_open_letter;
extern char ch_closed_letter;
extern char ch_invalid_letter;

extern const char* freq_khz;
extern const char* freq_hz;

extern const char* speed_bit;

extern const char proc;

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

extern const char *aruStr;
extern const char *armStr;
extern const char *ausStr;

extern const char *error_SWF;
extern const char *true_SWF;

extern const char *smsText[];

extern const char *sms_quit_fail1;
extern const char *sms_quit_fail2;
extern const char *sms_crc_fail;

extern const char ch_key0[2];
extern const char ch_key1[6];
extern const char ch_key2[4];
extern const char ch_key3[4];
extern const char ch_key4[4];
extern const char ch_key5[4];
extern const char ch_key6[4];
extern const char ch_key7[4];
extern const char ch_key8[4];
extern const char ch_key9[4];

extern const char *recvCondCommandStr;
#endif /* TEXTS_H_ */
