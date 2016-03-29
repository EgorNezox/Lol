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

const char* freq_khz = "���";
const char* freq_hz = "��";

char * ok_texts[LANG_COUNT]={(char *)"��"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"����������� ������� ������� �������"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"����������� �������� ������� ������"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"�������������� ������� ������� ������� ������ ������� ���������"};


const char *mainMenu[] = {"������� ����", "\t�����", "\t�����", "\t������", "\t���������"};

const char *callSubMenu[] = {"�������� �������", "SMS", "��������� �����", "������ ���.������"};
    const char *commandsSubMenu[] = {"������.�����", "������.�����"};
        const char *smplSubMenu[] = {"��������� �����", "��������������"};
        const char *twSubMenu[] = {"��� �������������", "� ��������������"};
    const char *smsSubMenu[] = {"��� �������������", "� ��������������"};
    const char *groupCommandsSubMenu[] = {"������.�����", "������.�����"};
        const char *groupCommandsSimplSubMenu[] = {"��������� �����", "��������������"};

const char *reciveSubMenu[] = {"����", "���������(���)", "������ ���.������"};

const char *dataSubMenu[] = {"��������", "������������", "����������"};
    const char *dataSubSubMenu[] = {"�������� �������", "���", "������� �����", "������ ���.������"};

const char *settingsSubMenu[] = {"����/�����", "��������� �����", "������������","���������"};
    const char *dateAndTimeSubMenu[] = {"GPS ����-���", "������ ���������"};
        const char *setDateOrTime[] = {"���������� ����", "���������� �����"};
    const char *setConnParam[] = {"�������", "��������"};
    const char *useScanMenu[] = {"���", "����"};
