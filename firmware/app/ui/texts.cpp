/*
 * texts.cpp
 *
 *  Created on: 14 ���. 2015 �.
 *      Author: user
 */


#include "texts.h"

char * mode_txt = (char *)"����";
char *disabled_ch_txt = (char *)"--";
char ch_open_letter='�';
char ch_zero_letter='0';
char ch_closed_letter='�';
char ch_invalid_letter='!';


char * ok_texts[LANG_COUNT]={(char *)"��"};
char * missing_ch_table_txt[LANG_COUNT]={(char *)"����������� ������� ������� �������"};
char * missing_open_ch_txt[LANG_COUNT]={(char *)"����������� �������� ������� ������"};
char * ch_table_mismatch_txt[LANG_COUNT]={(char *)"�������������� ������� ������� ������� ������ ������� ���������"};


char *mainMenu[] = {(char*)"\t�����", (char*)"\t�����", (char*)"\t������", (char*)"\t���������"};

char *callSubMenu[] = {(char*)"���", (char*)"�������� �������", (char*)"SMS", (char*)"��������� �����", (char*)"������ ���.������"};
    char *commandsSubMenu[] = {(char*)"������.�����", (char*)"������.�����"};
        char *smplSubMenu[] = {(char*)"��������������", (char*)"��������� �����"};
        char *twSubMenu[] = {(char*)"��� �������������", (char*)"� ��������������"};
    char *smsSubMenu[] = {(char*)"��� �������������", (char*)"� ��������������"};
    char *groupCommandsSubMenu[] = {(char*)"������.�����", (char*)"������.�����"};

char *reciveSubMenu[] = {(char*)"�������� �����", (char*)"������������", (char*)"���", (char*)"�������� �������", (char*)"SMS", (char*)"��������� �����", (char*)"������ ���.������"};

char *dataSubMenu[] = {(char*)"��������", (char*)"������������", (char*)"����������"};
    char *recvSubMenu[] = {(char*)"�������� �������", (char*)"���", (char*)"������� �����", (char*)"������ ���.������"};
    char *sendSubMenu[] = {(char*)"�������� �������", (char*)"���", (char*)"������� �����", (char*)"������ ���.������"};
    char *saveSubMenu[] = {(char*)"�������� �������", (char*)"���", (char*)"������� �����", (char*)"������ ���.������"};

char *settingsSubMenu[] = {(char*)"����/�����", (char*)"����� �����"};
    char *dateAndTimeSubMenu[] = {(char*)"���������� ����", (char*)"���������� �����"};
    char *setTimeSubMenu[] = {(char*)"GPS ����-���", (char*)"������ ���������"};
        char * modeSubMenu[] = {(char*)"������", (char*)"��������������"};
