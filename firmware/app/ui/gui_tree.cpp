#include "gui_tree.h"

void CGuiTree::init()
{

    CState main, call, recv, data, settings;
    CState tlf, condCmd, sms;
    main.prevState = nullptr;

    call.prevState = &main;

    main.statesList.push_back(&call);
    main.statesList.push_back(&recv);
    main.statesList.push_back(&data);
    main.statesList.push_back(&settings);






    std::string str[6];
    str[1].append("mainWindow");

/*

    CState initState;
    initState.setName("mainWindow");

    CState simpl;
    simpl.setName("Симплексная телефонная радиосвязь");

    CState pprtch;
    pprtch.setName("Условные команды (ПП� Ч)");

    CState sms;
    sms.setName("Передача/прием SMS сообщений (ПП� Ч)");

    CState post;
    post.setName("Голосовая почта");

    CState commands;
    commands.setName("Группа условных команд");

    statesList.push_back(initState);
    statesList.push_back(simpl);
    statesList.push_back(pprtch);
    statesList.push_back(sms);
    statesList.push_back(post);
    statesList.push_back(commands);
*/
}

void CGuiTree::append(GuiWindowTypes type, char *p)
{
    statesList.push_back(new CState(type, p));
}

int CGuiTree::getLastElement( CState& st)
{
    if (statesList.size() > 0)
    {
        st = *statesList.front();
        return 1;
    }
    else
        return -1;
}

void CGuiTree::delLastElement()
{
    if ( statesList.size() > 0 )
    {
        statesList.pop_back();
    }
}

CGuiTree::~CGuiTree()
{
}
