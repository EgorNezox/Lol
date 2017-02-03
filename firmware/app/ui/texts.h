/*
 * texts.h
 *
 *  Created on: 14 дек. 2015 г.
 *      Author: user
 */

#ifndef TEXTS_H_
#define TEXTS_H_


#define LANG_COUNT 1
#define MAIN_MENU_MAX_LIST_SIZE 8
#define _LIST_SIZE 5
#define SETTINGS_LIST_SIZE 2

extern const char *test_Pass;
extern const char *errorStr;
extern const char* exitStr;
extern char *mode_txt[];
extern char *disabled_ch_txt;
extern char ch_zero_letter;
extern char ch_open_letter;
extern char ch_closed_letter;
extern char ch_invalid_letter;

extern const char* ch_em_type_str[];

extern const char* freq_khz;
extern const char* freq_hz;

extern const char* exitStr;
extern const char* speed_bit;

extern const char proc;

extern const char* trans;
extern const char* callTitle[];
extern const char* yesNo[];

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
extern const char *dataAndTime[];
extern const char *files[];

extern const char *aruStr;
extern const char *armStr;
extern const char *ausStr;

extern const char *error_SWF;
extern const char *true_SWF;

extern const char *txSmsResultStatus[];
extern const char *rxSmsResultStatus[];
extern const char *smsText[];

extern const char *sms_quit_fail1;
extern const char *sms_quit_fail2;
extern const char *sms_crc_fail;

extern const char* receiveStr;
extern const char* receiveStatusStr[];
extern const char* continueStr;

extern const char ch_key0[2];
extern const char ch_key1[7];
extern const char ch_key2[5];
extern const char ch_key3[5];
extern const char ch_key4[5];
extern const char ch_key5[5];
extern const char ch_key6[5];
extern const char ch_key7[5];
extern const char ch_key8[5];
extern const char ch_key9[5];

extern const char *recvCondCommandStr;
extern const char* groupCondCommFreqStr;

extern const char* putOffVoiceStageOneStr[];
extern const char* putOffVoiceStageTwoStr[];

extern const char* startAleTxVoiceMailStr;
extern const char* smatrHSStateStr[];
extern const char* aleStateStr[];
extern const char* voiceRxStr[];
extern const char* voiceRxTxLabelStr[];
extern const char* condCommStr[];
extern const char* condCommSendStr;
extern const char* ticketStr[];
extern const char* voicePostTitleStr[];

extern const char* errorCrcGuc;
extern const char* gucQuitTextFail;
extern const char* gucQuitTextOk;

extern const char *coordinateStr;
extern const char *coordNotExistStr;

extern const char* pressEnter;
extern const char* recievedGUC_CMD;
extern const char* titleGuc;
extern const char* titleCoord;
extern const char* typeCondCmd;

extern const char* NoYesGucCoord[];
extern const char* StartGucTx;
extern const char* GucIndividGroup[];
extern const char* Sheldure_label;
extern const char* editSheldure_label;
extern const char* newSheldure_label;
extern const char* delSheldure_lable;
extern const char* addSheldure;
extern const char* editSheldure;
extern const char* delSheldure;
extern const char* askDelSheldure;
extern const char* tmpParsing[];

extern const char* startStr;

extern const char* atumalfunction_title_str;
extern const char* atumalfunction_text_str;

extern const char* dsphardwarefailure_7_5_title_str;
extern const char* dsphardwarefailure_7_5_text_str;
extern const char* dsphardwarefailure_unknown_title_str;
extern const char* dsphardwarefailure_unknown_text_str;

extern const char *smsDataInformDx[];

extern const char *sheldureSubMenu[];

extern const char *StartCmd;
extern const char *EndCmd;
extern const char *EndSms;
extern const char *EndSms2;

extern const char *STARTS;

extern const char *errorReadFile;

extern const char *displayBrightnessStr[];
extern const char *displayBrightnessTitleStr;

extern const char *schedulePromptStr;

extern const char *txrxFilesStr[];

extern const char *rxtxFiledSmsStr[];

extern const char *recPacket;
extern const char *cmdRec;
extern const char *notReiableRecPacket;

extern const char *rxCondErrorStr[];
extern const char *rnKey;

#endif /* TEXTS_H_ */
