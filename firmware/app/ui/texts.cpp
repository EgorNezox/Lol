/*
 * texts.cpp
 *
 *  Created on: 14 ���. 2015 �.
 *      Author: user
 */


#include "texts.h"

char * mode_txt[] = {(char *)"����", (char*)"���", (char*)"������"};
char *disabled_ch_txt = (char *)"--";
char ch_open_letter='�';
char ch_zero_letter='0';
char ch_closed_letter='�';
char ch_invalid_letter='!';

const char* freq_khz = "���\0";
const char* freq_hz = "��\0";

const char* trans = "���������";
const char* callTitle[2] = {"�������\0", "�����\0"};

char * ok_texts[LANG_COUNT]={(char *)"��"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"����������� ������� ������� �������"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"����������� �������� ������� ������"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"�������������� ������� ������� ������� ������ ������� ���������"};


const char *mainMenu[] = {"������� ����\0", "\t�����\0", "\t�����\0", "\t������\0", "\t���������\0"};

const char *callSubMenu[] = {"�������� �������\0", "SMS\0", "��������� �����\0", "������ ���.������\0"};
    const char *commandsSubMenu[] = {"������.�����\0", "������.�����\0"};
        const char *smplSubMenu[] = {"��������� �����\0", "��������������\0"};
        const char *twSubMenu[] = {"��� �������������\0", "� ��������������\0"};
    const char *smsSubMenu[] = {"��� �������������\0", "� ��������������\0"};
    const char *groupCommandsSubMenu[] = {"������.�����\0", "������.�����\0"};
        const char *groupCommandsSimplSubMenu[] = {"��������� �����\0", "��������������\0"};

const char *reciveSubMenu[] = {"����", "���������(���)", "������ ���.������"};

const char *dataSubMenu[] = {"��������\0", "������������\0", "����������\0"};
    const char *dataSubSubMenu[] = {"�������� �������\0", "���\0", "������� �����\0", "������ ���.������\0"};

const char *settingsSubMenu[] = {"����/�����\0", "��������� �����\0", "������������\0", "���/���\0", "��������������\0"};
    const char *dateAndTimeSubMenu[] = {"GPS ����-���\0", "������ ���������\0"};
        const char *setDateOrTime[] = {"���������� ����\0", "���������� �����\0"};
    const char *setConnParam[] = {"�������\0", "��������\0"};
    const char *useScanMenu[] = {"���\0", "����\0"};
