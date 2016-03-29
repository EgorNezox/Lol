﻿#include "menu.h"
#include <string.h>
#include "all_sym_indicators.h"

CGuiMenu::CGuiMenu(MoonsGeometry* area, const char *title, const char *text, Alignment align):CGuiDialog(area),
                                                                                              editing(false),
                                                                                              focus(0),
                                                                                              numItem(7)
{
    windowArea = {0,0,(GXT)(GEOM_W(this->area)-5),(GYT)(GEOM_H(this->area)-5)};
    textAreaParams = GUI_EL_TEMP_CommonTextAreaLT;
    textAreaParams.element.align = align;
    this->setTitle(title);

    titleParams = GUI_EL_TEMP_LabelTitle;
    titleParams.element.align = align;

    itemParams.label_params = GUI_EL_TEMP_LabelText;
    itemParams.label_params.element.align = {alignHCenter, alignTop};
    itemParams.icon_params.element  = GUI_EL_TEMP_CommonIcon;
    itemParams.icon_params.icon = sym_new_msg;

    for (int i = 0; i < numItem; i++)
        item[i] = nullptr;

    inputStr[0] = nullptr;
    inputStr[1] = nullptr;

    label[0] = nullptr;
    label[1] = nullptr;

    tx = nullptr;
    tx = new char[100];
}

void CGuiMenu::initDialog(CEndState state)
{
    titleParams = GUI_EL_TEMP_LabelTitle;
    titleParams.element.align = {alignHCenter, alignTop};

    LabelParams params;
    params = GUI_EL_TEMP_CommonTextAreaLT;
    params.element.align = {alignHCenter, alignTop};

    MoonsGeometry labelStrArea[5];
    bool f;

    auto size = state.listItem.size();
    int i = 0;
//    for (int i = 0; i < size; i++)
    for (auto &k: state.listItem)
    {
        labelStrArea[i] = {(GXT)(windowArea.xs + MARGIN),
                           (GYT)(windowArea.ys + 17 + 2*i*(MARGIN + BUTTON_HEIGHT)),
                           (GXT)(windowArea.xe - MARGIN),
                           (GYT)(windowArea.ys + 14 + (2*i+1)*(MARGIN + BUTTON_HEIGHT) )
                          };

        itemArea[i] = {(GXT)(windowArea.xs + 40),
                       (GYT)(windowArea.ys + 17 + (2*i+1)*(MARGIN + BUTTON_HEIGHT)),
                       (GXT)(windowArea.xe - 40),
                       (GYT)(windowArea.ys + 14 + (2*i+2)*(MARGIN + BUTTON_HEIGHT) )
                      };

        f = false;
        if (i == focus)
            f = true;

        label[i] = new GUI_EL_Label   (&params,     &labelStrArea[i], (char*)k->label, (GUI_Obj*)this);
        item [i] = new GUI_EL_MenuItem(&itemParams, &itemArea[i],     (char*)k->inputStr.c_str(), true, f, (GUI_Obj*)this);

    i++;
    }

    f = false;
    if (size == focus)
        f = true;

    // OK
    itemArea[size] = {(GXT)(windowArea.xs + 40),
                      (GYT)(windowArea.ys + 17 + 5 + 4*(MARGIN + BUTTON_HEIGHT)),
                      (GXT)(windowArea.xe - 40),
                      (GYT)(windowArea.ys + 14 + 5 + 5*(MARGIN + BUTTON_HEIGHT) )
                     };
    item [size] = new GUI_EL_MenuItem(&itemParams, &itemArea[size],  (char*)trans, true, f, (GUI_Obj*)this);
}

void CGuiMenu::initCallDialog()
{
    //
}

void CGuiMenu::initTwoStateDialog()
{
    int i = 2;
    itemArea[0] = {(GXT)(windowArea.xs + 7*MARGIN),
                   (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                   (GXT)(windowArea.xe - 7*MARGIN),
                   (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                  };

//    MoonsGeometry volume_geom  = {  35,  25,  112,  85 };
//    GUI_EL_Label *volume = new GUI_EL_Label (&GUI_EL_TEMP_LabelChannel, &volume_geom,  NULL, (GUI_Obj*)this);
//    volume->SetText("20");

    // title
    titleArea = {(GXT)(windowArea.xs + MARGIN),
                 (GYT)(windowArea.ys + MARGIN),
                 (GXT)(windowArea.xe - MARGIN),
                 (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )
                };

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();
}

void CGuiMenu::initVolumeDialog()
{
    int i = 2;
    itemArea[0] = {(GXT)(windowArea.xs + 7*MARGIN),
                   (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                   (GXT)(windowArea.xe - 7*MARGIN),
                   (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                  };

    MoonsGeometry volume_geom  = {  35,  25,  112,  85 };
    GUI_EL_Label *volume = new GUI_EL_Label (&GUI_EL_TEMP_LabelChannel, &volume_geom,  NULL, (GUI_Obj*)this);

    char s[3]; itoa( vol, s, 3);
    volume->SetText((char*) s);

    // title
    titleArea = {(GXT)(windowArea.xs + MARGIN),
                 (GYT)(windowArea.ys + MARGIN),
                 (GXT)(windowArea.xe - MARGIN),
                 (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )
                };

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    volume->Draw();

    delete volume;
}

void CGuiMenu::setTitle(const char* title)
{
    titleStr.clear();
    titleStr.append(title);
}

void CGuiMenu::initItems(std::list<std::string> text, const char* title, int focusItem)
{
    setTitle(title);

    int i = 0;
    for (auto &k: text)
    {

        itemArea[i] = {(GXT)(windowArea.xs + MARGIN),
                       (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                       (GXT)(windowArea.xe - MARGIN),
                       (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                      };
        bool f;
        if (i == focusItem)
            f = true;
        else
            f = false;

        item[i] = new GUI_EL_MenuItem(&itemParams, &itemArea[i], (char*)k.c_str(), true, f, (GUI_Obj*)this);
        i++;
    }
}

void CGuiMenu::Draw()
{
    // title
    titleArea = {(GXT)(windowArea.xs + MARGIN),
                 (GYT)(windowArea.ys + MARGIN),
                 (GXT)(windowArea.xe - MARGIN),
                 (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )
                };

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();

    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
        if (item[i] != nullptr)
            item[i]->Draw();

    for (int i = 0; i < 2; i++)
        if (inputStr[i] != nullptr)
            inputStr[i]->Draw();

    for (int i = 0; i < 2; i++)
        if (label[i] != nullptr)
            label[i]->Draw();

    for (int i = 0; i < 5; i++)
    {    if (item[i] != nullptr) delete item[i]; item[i] = nullptr; }

    for (int i = 0; i < 2; i++)
    {    if (inputStr[i] != nullptr){ delete inputStr[i]; inputStr[i] = nullptr;}
        if (label[i] != nullptr){ delete label[i]; label[i] = nullptr;}}
}

CGuiMenu::~CGuiMenu()
{
    for (int i = 0; i < numItem; i++)
        if (item[i] != nullptr)
            delete item[i];

    for (int i = 0; i < 2; i++)
        if (inputStr[i] != nullptr)
            delete inputStr[i];

    for (int i = 0; i < 2; i++)
        if (label[i] != nullptr)
            delete label[i];

    if (tx != nullptr)
        delete tx;
}

void CGuiMenu::keyPressed(UI_Key key)
{
    char value;
    switch (key)
    {
    case keyEnter: // подтверждение ввода
        if (focus == 0)
        {
            dstAddr.clear();
            dstAddr.append(&newDstAddr.back());
            newDstAddr.clear();
        }
        else if ( focus == 1 )
        {
            message.clear();
            message.append(&newMessage.back());
            newMessage.clear();
        }
        editing = false;
        break;
    case keyBack: // стирание или выход из режима редактирования
        if ( focus == 0 && dstAddr.size() > 0)
        {
            newDstAddr.pop_back();
        }
        else if( focus == 1 && message.size() > 0)
        {
            newMessage.pop_back();
        }
        else
        {
            //
        }
        break;
    case key0:
        value = '0';
        editing = true;
        break;
    case key1:
        value = '1';
        editing = true;
        break;
    case key2:
        value = '2';
        editing = true;
        break;
    case key3:
        value = '3';
        editing = true;
        break;
    case key4:
        value = '4';
        editing = true;
        break;
    case key5:
        value = '5';
        editing = true;
        break;
    case key6:
        value = '6';
        editing = true;
        break;
    case key7:
        value = '7';
        editing = true;
        break;
    case key8:
        value = '8';
        editing = true;
        break;
    case key9:
        value = '9';
        editing = true;
        break;
    default:
        break;
    }

    if ( focus == 0)
    {
        newDstAddr.push_back(value);
    }else if( focus == 1 ){
        newMessage.push_back(value);
    }
}
