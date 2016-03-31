#include <stdio.h>
#include "menu.h"
#include <string.h>
#include "all_sym_indicators.h"

CGuiMenu::CGuiMenu(MoonsGeometry* area, const char *title, const char *text, Alignment align):CGuiDialog(area),
    editing(false),
    focus(0),
    numItem(7)
{
    windowArea = {0,0,(GXT)(GEOM_W(this->area)-5),(GYT)(GEOM_H(this->area))};
    textAreaParams = GUI_EL_TEMP_CommonTextAreaLT;
    textAreaParams.element.align = align;
    this->setTitle(title);

    titleParams = GUI_EL_TEMP_LabelTitle;
    titleParams.element.align = align;

    itemParams.label_params = GUI_EL_TEMP_LabelText;
    itemParams.label_params.element.align = {alignHCenter, alignTop};
    itemParams.icon_params.element  = GUI_EL_TEMP_CommonIcon;
    itemParams.icon_params.icon = sym_new_msg;

    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
    {
        item[i] = nullptr;
        label[i] = nullptr;
    }

    inputStr[0] = nullptr;
    inputStr[1] = nullptr;

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

    MoonsGeometry labelStrArea[6];
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




void CGuiMenu::initCallDialog(CEndState state)
{
    titleParams = GUI_EL_TEMP_LabelTitle;
    titleParams.element.align = {alignHCenter, alignTop};

    LabelParams params;
    params = GUI_EL_TEMP_CommonTextAreaLT;
    params.element.align = {alignHCenter, alignTop};
    MoonsGeometry labelStrArea[6];

    bool f;
    int i = 0;
    auto size = state.listItem.size();

    titleArea = {(GXT)(windowArea.xs + MARGIN),
                 (GYT)(windowArea.ys + MARGIN),
                 (GXT)(windowArea.xe - MARGIN),
                 (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )
                };

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

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

    itemArea[size] = {(GXT)(windowArea.xs + 40),
                      (GYT)(windowArea.ys + 17 + 5 + 4*(MARGIN + BUTTON_HEIGHT)),
                      (GXT)(windowArea.xe - 40),
                      (GYT)(windowArea.ys + 14 + 5 + 5*(MARGIN + BUTTON_HEIGHT) )
                     };
    item [size] = new GUI_EL_MenuItem(&itemParams, &itemArea[size],  (char*)trans, true, f, (GUI_Obj*)this);

    window.Draw();
    title.Draw();

    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
        if (item[i] != nullptr)
            item[i]->Draw();
    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
        if (label[i] != nullptr)
            label[i]->Draw();

    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
    {    if (item[i] != nullptr) delete item[i]; item[i] = nullptr; }
    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
    {    if (label[i] != nullptr) delete label[i]; label[i] = nullptr; }
}




void CGuiMenu::initTwoStateDialog()
{
    int i = 0;
    itemArea[0] = {(GXT)(windowArea.xs + 7*MARGIN),
                   (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                   (GXT)(windowArea.xe - 7*MARGIN),
                   (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                  };

    titleArea = {(GXT)(windowArea.xs + MARGIN),
                 (GYT)(windowArea.ys + MARGIN),
                 (GXT)(windowArea.xe - MARGIN),
                 (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )
                };

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    itemArea[0] = {  20,  20,  120,  90 };

    itemParams.label_params = GUI_EL_TEMP_LabelChannel;
    itemParams.label_params.element.align = {alignHCenter, alignTop};
    itemParams.icon_params.element  = GUI_EL_TEMP_CommonIcon;
    itemParams.icon_params.icon = sym_new_msg;

    item [0] = new GUI_EL_MenuItem(&itemParams, &itemArea[0],  (char*)useScanMenu[0], true, true, (GUI_Obj*)this);

    window.Draw();
    title.Draw();

    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
        if (item[i] != nullptr)
            item[i]->Draw();

    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
    {    if (item[i] != nullptr) delete item[i]; item[i] = nullptr; }
}

void CGuiMenu::initVolumeDialog()
{
    int i = 2;
    itemArea[0] = {(GXT)(windowArea.xs + 7*MARGIN),
                   (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                   (GXT)(windowArea.xe - 7*MARGIN),
                   (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                  };

    MoonsGeometry volume_geom  = {  20,  20,  120,  90 };
    GUI_EL_Label *volume = new GUI_EL_Label (&GUI_EL_TEMP_LabelChannel, &volume_geom,  NULL, (GUI_Obj*)this);

    char s[4]; sprintf(s,"%d",vol);
    std::string str;

    str.append(s);
    str.push_back(proc);
    volume->SetText((char *)str.c_str());
    str.clear();

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


void CGuiMenu::initAruarmDialog()
{
    int i = 2;
    itemArea[0] = {(GXT)(windowArea.xs + 7*MARGIN),
                   (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                   (GXT)(windowArea.xe - 7*MARGIN),
                   (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                  };

    MoonsGeometry volume_geom  = {  35,  40,  105,  70 };
    GUI_EL_Label *volume = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &volume_geom,  NULL, (GUI_Obj*)this);

    char s[4]; sprintf(s,"%d",vol);
    std::string str;

    if (aruArmAction == 1)
        volume->SetText((char *)useScanMenu[0]);
    else
        volume->SetText((char *)useScanMenu[1]);

//    str.push_back(proc);
//    volume->SetText((char *)str.c_str());
//    str.clear();

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


void CGuiMenu::initGpsCoordinateDialog()
{
    int i = 2;
    itemArea[0] = {(GXT)(windowArea.xs + 7*MARGIN),
                   (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                   (GXT)(windowArea.xe - 7*MARGIN),
                   (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                  };

    MoonsGeometry volume_geom[2];
            volume_geom[0]  = {  5,  30,  140,  60 };
            volume_geom[1]  = {  5,  60,  140,  90 };

    GUI_EL_Label* volume[2];
            volume[0] = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &volume_geom[0],  NULL, (GUI_Obj*)this);
            volume[1] = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &volume_geom[1],  NULL, (GUI_Obj*)this);

    coord.append("0123.4567,N");
    volume[0]->SetText((char *)coord.c_str());
    volume[1]->SetText((char *)coord.c_str());

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
    volume[0]->Draw();
    volume[1]->Draw();

    delete volume[0];
    delete volume[1];
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
                       (GYT)(windowArea.ys + 12 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
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

    for (int i = 0; i < 6; i++)
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
