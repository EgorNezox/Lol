#include "texts.h"

const char *errorStr = "������\0";
char *mode_txt[] = {" ����\0", "����.\0"};
char *disabled_ch_txt = (char *)"--";
char ch_open_letter='�';
char ch_zero_letter='0';
char ch_closed_letter='�';
char ch_invalid_letter='x';

const char* ch_em_type_str[] = {(char *)"F3E\0", (char*)"J3E\0",(char*)"�_�\0",(char*)"�_�\0"};

const char* exitStr = "�����\0";

const char* freq_khz = "���\0";
const char* freq_hz = "��\0";

const char* speed_bit = "���/�\0";

const char proc = '%';

const char* trans = "���������\0";
const char* callTitle[2] = {"�������\0", "�����\0"};
const char* yesNo[] = {"\t\t���\0", "\t\t��\0"};

char * ok_texts[LANG_COUNT]={(char *)"��"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"����������� �������\n������� �������\0"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"����������� ��������\n������� ������\0"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"��������������\n������� �������\n������� ������\n������� ���������\0"};

const char* receiveStr = "�������\0";
const char* continueStr = "����������\0";
const char* receiveStatusStr[] = {"  ������\n  �����\0", "   ����\n  �����\0"};

const char *mainMenuFull[] = {"������� ����\0", " ��������\0", " �����\0", " ������\0", " ���������\0"};
const char *mainMenu[]     = {"������� ����\0", " �����.\0", " �����\0", " ������\0", " �����.\0"};

const char *callSubMenu[] = {" ��\0", " ���\0", " ��\0", " ���\0"};
const char *commandsSubMenu[] = {" ������.�����\0", " ������.�����\0"};
const char *smplSubMenu[] = {"���������\0", "�������.\0", "  �������.\n   � ����.\0"};
const char *twSubMenu[] = {" ��� �������������\0", " � ��������������\0"};
const char *smsSubMenu[] = {" ��� �������������\0", " � ��������������\0"};
const char *groupCommandsSubMenu[] = {" ������.�����\0", " ������.�����\0"};
const char *groupCommandsSimplSubMenu[] = {" ��������� �����\0", " ��������������\0"};

const char *reciveSubMenu[] = {" ����\0", " ���\0", " ��\0", " ��\0", " ���\0"};
//const char *tmpParsing[] = {" ����\0", " ��\0", " ���\0", " ��\0", " ���\0"};
const char *tmpParsing[] = {" ����\0", " ��\0", " ���\0", " ���\0", " ��\0"};
const char *fileRxTx[] = {" ���\0", " ���\0"};

const char *dataSubMenu[] = {" ��������\0", " ������������\0", " ����������\0", " GPS ����������\0"};
const char *dataSubSubMenu[] = {" �������� �������\0", " ���\0", " ������� �����\0", " ������ ���.������\0"};

const char *settingsSubMenu[] = {" ���.��.\0", " �����\0", " ����.\0", " �����\0", " ����\0",
                                 " ���\0"," ����� ���\0", " ��. ����"," ������."," �����",
                                 " �������\0", " ������.\0", " �����\0" }; //TODO:

const char *settingsSubMenuIn[] = {" ���� �����\0", " �����\0", " ����.\0", " �����\0", " ����\0",
                                 " ��������������\0"," ����� ���\0", " ���� ����"," ������������"," ����������",
                                 " �������\0", " ������.����\0", " ���������\n �����:\0"  };

const char *technoSubMenu[]   = {" �����.\0", " �����\0", " ������\0", " �������\0", "������.\0"};
const char *technoSubMenuIn[] = {" �����.����������\0", " ����� �������\0", " ������ ��\0", "������ ������ ���\0", "����\0"};

const char *dateAndTimeSubMenu[] = {" ������\0", " ������\0"};
const char *setDateOrTime[] = {" ����\0", " �����\0"};
//const char *setDateOrTime[] = {" ���� ����\0", " ���� �����\0"};
const char *setConnParam[] = {" �������\0", " ����.\0", " �����.\0", " ����\0"};
const char *files[] = {" �����\0"};

const char *coordinateStr = "����������\0";
const char *coordNotExistStr = "����������\n �����������\0";
//const char *coordNotExistStr = "���������\n ���\0";
const char *notExistStr = "��������.\0";
const char *useScanMenu[] = {"����\0", "���\0", "���\0","����\0","���\0"};
const char *aruStr = "���\0";
const char *armStr = "���\0";
const char *ausStr = "��C\0";
const char *error_SWF = "���� �������\0";
const char *true_SWF = "���� �������\0";

const char *smsText[] = {"�����: \0", "���������:\n\0"};

const char *sms_quit_fail1 = "��� ������\0";
const char *sms_quit_fail2 = "��������� ��������\0";
//const char *sms_crc_fail = "����� ��������� � ��������\0";
const char *sms_crc_fail = "����� ����. � ����.\0";

const char ch_key0[] = { ' ', '0' };
const char ch_key1[] = { '.', ',', '!', '?', '\"', ':', '1' };
const char ch_key2[] = { '�', '�', '�', '�', '2' };
const char ch_key3[] = { '�', '�', '�', '�', '3' };
const char ch_key4[] = { '�', '�', '�', '�', '4' };
const char ch_key5[] = { '�', '�', '�', '�', '5' };
const char ch_key6[] = { '�', '�', '�', '�', '6' };
const char ch_key7[] = { '�', '�', '�', '�', '7' };
const char ch_key8[] = { '�', '�', '�', '�', '8' };
const char ch_key9[] = { '�', '�', '�', '�', '9' };

//const char* recvCondCommand = "������� �������� �������\0";
const char* recvCondCommand = "������� ���. ���.\0";

const char* groupCondCommFreqStr = "������� \0";
const char* schedulerFreqStr = "������� ��\0";
const char* dataAndTime[] = {"����\0", "�����\0"};

//const char* putOffVoiceStageOneStr[] = {"������� ���� ��� ������\0", "�������� ������\0"};
//const char* putOffVoiceStageTwoStr[] = {"������� ���� ��� ����������\0", "������ ����\0"};
const char* putOffVoiceStageOneStr[] = {"��������\0", "����. ���.\0"};
const char* putOffVoiceStageTwoStr[] = {"���������\0", "������ ����\0"};

//const char* startAleTxVoiceMailStr = "�������\n���� ���\n�������\0";
//const char* startAleTxVoiceMailStr = "������� ���� ��� �������\n ��������.\0";
const char* startAleTxVoiceMailStr = "���������\n ��������\0";

//const char* smatrHSStateStr[] = {
//    "�������������\n����������.\n���������.\0",
//    "�������������\n����������.\n��� ���������\0",
//    " ������\n ���������.\0",
//    "������ ������\n ��������\n ������ �����.\0",
//    " ����������\n ���������.\n ��������.\0",
//    "������������\n���������.\n��������.\0",
//    " ��������\n ������ �����.\n ��������.\0",
//    "������������\n c��������.\n ��������.\0",
//    " ����������\n ������.\n ��������.\0",
//    " ����������\n ������.\n ��������.\0",
//    " �����������\n ��������\n ��� ������\0",
//    " ���������\n ������.\n ��������.\0",
//    " �������\n �����\n ��������.\0",
//    " ������.\0",
//    "14","15","16","17","18","19","20"
//};

const char* smatrHSStateStr[] = {
    "���������.\n������.\n���������.\0",
    "���������.\n������. ���\n ���������\0",
    " ������\n ������.\0",
    "����. ���.\n ��������\n ��. �����\0",
    " ������.\n �����.\n ��������.\0",
    "��������.\n���������.\n��������.\0",
    " ��������\n ���. �����.\n ��������\0",
    "��������.\n c����.\n ��������\0",
    " ��������.\n ������.\n ��������\0",
    " ��������.\n ������.\n ��������\0",
    " �������.\n ��������\n ��� ������\0",
    " ���������\n ������.\n ��������\0",
    " �������\n �����\n ��������\0",
    " ������.\0",
    "14","15","16","17","18","19","20"
};

//const char* openChannelPlayErrorStr = "���������\n����������.\n�������� �����.\0";
//const char* noHeadsetPlayErrorStr   = "���������\n����������.\n��� ���������.\0";
const char* openChannelPlayErrorStr = "������.\n��������.\n����. ���.\0";
const char* noHeadsetPlayErrorStr   = "������.\n��������.\n��� ����.\0";

const char* aleStateStr[] = {
/* 0 */			"\0",
/* 1 */		    "���\n���������.\0",
/* 2 */		    "����������\n ����\0",
/* 3 */		    "��� ������\n ���\n �� ���������\0",
/* 4 */		    "�� �����\n �����\n �������\0",
/* 5 */		    "������.\n �����.\n���������.\0",
/* 6 */		    "��������\n ������...\0",
/* 7 */		    "����. ���.\n ���������\0",
/* 8 */		    "����.\n ������\n �������.\n ���������.\0",
/* 9 */		    "����� ��\0",                            //"<AleVmProgress>%\0",
/* 10 */	    "���������\n �����\0",
/* 11 */	    "��� �����.\n ���������.\0",
/* 12 */	    "���. ����.\n��������.\n ������.\0",
/* 13 */	    "�������� ��.\0",                         //"<AleVmProgress>%\0",
/* 14 */	    "��� �����.\n ���������.\0",
/* 15 */	    "���������\n��������.\n���������.\0",
/* 16 */	    "��������\n �������\n ���������.\0",
/* 17 */	    "����� ����.\n �������.\n ���������.\0",
/* 18 */	    "��� �����.\n ���������.\0",
/* 19 */	    "�� �������\n� ��������.\0",
/* 20 */	    "������� ��.\n ���� ���\n �����������.\0",
/* 21 */		"��������\n ������.\0",
/* 22 */		"�����\n ������\n ������.\0",
/* 23 */		"�����\n ������.\0",
/* 24 */		"��������\n ������\n ������.\0",
/* 25 */		"����\n ������\n ��.\0",
/* 26 */		"��� ������\n ��� ��������\n ��.\0",
};

const char *smsDataInformTx[] = {
    "�����\n��������\0",
    "��������\n������\0",
    "�����\n������\0",
    "��������\n������\0",
    "������\n�������.\0",
    "�����\n�������\n���������\0",
    "������\n��������\n���������\n��������\0",
};

const char *smsDataInformRx[] = {
    "��������\n������\0",
    "�������\n��������\n�����\0",
    "�����\n�����������\0",
    "�����\n������\0",
    "�����\n�������\n���������\0",
    "������\n��������\n���������\0",
};

const char *txSmsResultStatus[] = {
    "����� �������\0",
    "�������� ������\0",
    "�������� ������\0",
    "���������\0"
};

const char *smsDataInformDx[] = {
    "����������� ���������\0",
    "����� �����\0",
    "����� �� �������\0",
    "�������� ������\0",
    "������ �������� (�������� ���������)\0",
};
const char *rxSmsResultStatus[] = {
    "�������� ������\0",
    "��������� ������\0",
    "����� ������\0",
    " ��������\n ���������\0"
};

const char* utcStr[] = {" ����\0" , "������� ����\0"};

const char* voiceRxStr[] = {"���������\n �����\0", "��:\0", "����. ����\0"};
//const char* voiceRxStr[] = {"������� ����\n ��� �������\n ������\0", "��:\0", "������� ����\0"};
const char* voiceRxTxLabelStr[] = {"����� ���.\0", "�����\n���.\0", "���. ����\0", "���. ����\0", "�������.\0", "�����. ��\0"};
//const char* voiceRxTxLabelStr[] = {"����� ������\0", "�����\n�����������.\0", "������ ����.\0", "�����. ����.\0", "����������\0", "�������� ��\0"};
const char* condCommStr[] = {"����� ����������\0", "       ����� \n  �������������\0", "�������\0", "������������\0", "���������\0"};
const char* condCommSendStr = "��������� \n��������\0";
//const char* condCommSendStr = "������� ���� ��� \n������� ��������\0";

const char* ticketStr[] = {"������������\0", "������������\0"};
//const char* voicePostTitleStr[] = {"�������� ��\0", "����� ��\0"};
const char* voicePostTitleStr[] = {"�����.\0", "�����\0"};

const char* gucQuitTextFail = "��������� �� �������\0";
const char* gucQuitTextOk   = "������� ���������\0";
const char* errorCrcGuc = "������ �� ����������\0";
//const char* pressEnter = "������� ����\n��� �����������\0";
const char* pressEnter = "����������\0";

const char* recievedGUC_CMD = "������� �������\0";
const char* titleGuc        = "���������\0";
const char* titleCoord      = "��������\n����������\0";

const char* startStr           = "�����\0";

const char* typeCondCmd         = "�������� ���\0";
const char* NoYesGucCoord[]     = {"���\0", "����\0"};
const char* StartGucTx          = "�����\0";
const char* GucIndividGroup[]   = {"������\0", "����\0"};
const char* Sheldure            = "12 000000 ��\n12:34 ���\0";
const char* Sheldure_label      = "  ����������";
const char* editSheldure_label  = "  ��������������";
const char* delSheldure_lable   = "  ��������";
const char* newSheldure_label   = "  ����� �����";
const char* addSheldure         = " ��������\n �����\0";
const char* editSheldure        = " ��������\n �����\0";
const char* delSheldure         = " �������\n �����\0";
const char* askDelSheldure      = "\t\t �������\n\t �����?\0";
const char* atumalfunction_title_str = "���� ���";
const char* atumalfunction_text_str = "�����. �� ���. ����. ����������";

const char* dsphardwarefailure_7_5_title_str = "������ ���";
const char* dsphardwarefailure_7_5_text_str = "0";
const char* dsphardwarefailure_unknown_title_str = "���������� ���� DSP";
const char* dsphardwarefailure_unknown_text_str = "����. %d, ��� %d";

const char *StartCmd = "��������...\0";
const char *EndCmd   = "�����\0";
const char *EndSms   = "��������� ����������\0";
const char *EndSms2  = "��������� �� ����������\0";

const char *coodGucStr  = "�������.\n\0";

const char *STARTS = "�����";

const char *recPacket = "������� �������\0";
//const char* cmdRec   = "������� ���������\0";
const char* cmdRec   = "���������\0";
const char *notReiableRecPacket = "������� ������������� �������\0";

const char *errorReadFile = "������ ������ �����\0";

//const char *displayBrightnessStr[] = {"�����������\0","�������\0","������������\0"};
const char *displayBrightnessStr[] = {"�������\0","�������\0","��������\0"};
const char *displayBrightnessTitleStr = "�������\0";

const char *schedulePromptStr = " �����\n �� ������\n ������ �����\n ������\0";

const char *rxtxFilesStr[] = {"��������\0", "����������\0"};

const char *rxtxFiledSmsStr[] = {"�������� �����\0", "������ ���\0"};

const char *rxCondErrorStr[] = {"�������� �����:\n\t������\t\0", "�������� �����:\n\t�����������\n\t������\t\0" };
const char *rnKey = "���� ���������\0";

const char *waitingStr = "��������.\0";
const char *flashProcessingStr = "���� ������\n � �������\0";

const char *radioStationStr = "������������\0";
//const char *sazhenNameStr = "������-�\0";

const char *fromStr = "��\0";

const char *syncWaitingStr = "������...\0";

const char *txQwit  = "����.\n ����.\0";

const char *err  = " ������\0";
const char *errCrc  = " ������ CRC\0";

const char *mainScrMode[] = {"��\0", "���\0", "���\0", "��\0"};
