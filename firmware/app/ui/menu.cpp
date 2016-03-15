#include "menu.h"
#include <string.h>
#include "all_sym_indicators.h"

CGuiMenu::CGuiMenu(MoonsGeometry* area, const char *title, const char *text, Alignment align):CGuiDialog(area)
{
    windowArea = {0,0,(GXT)(GEOM_W(this->area)-5),(GYT)(GEOM_H(this->area)-5)};
    textAreaParams = GUI_EL_TEMP_CommonTextAreaLT;
    textAreaParams.element.align = align;
    strcpy((char*)this->titleStr.c_str(), title);

    titleParams = GUI_EL_TEMP_LabelTitle;
    titleParams.element.align = align;

    itemParams.label_params = GUI_EL_TEMP_LabelText;
    itemParams.label_params.element.align = {alignHCenter, alignTop};
    itemParams.icon_params.element  = GUI_EL_TEMP_CommonIcon;
    itemParams.icon_params.icon = sym_new_msg;

    focus = 0;

    numItem = 7;
    for (int i = 0; i < numItem; i++)
        item[i] = nullptr;

    inputStr[0] = nullptr;
    inputStr[1] = nullptr;

    label[0] = nullptr;
    label[1] = nullptr;

    tx = new char[100];
}

void CGuiMenu::initDialog()
{
    titleParams = GUI_EL_TEMP_LabelTitle;
    titleParams.element.align = {alignHCenter, alignTop};

    LabelParams params;
    params = GUI_EL_TEMP_CommonTextAreaLT;
    params.element.align = {alignHCenter, alignTop};

    MoonsGeometry inputStrArea[2];

    int i = 0;
    inputStrArea[i] = {(GXT)(windowArea.xs + MARGIN),
                       (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                       (GXT)(windowArea.xe - MARGIN),
                       (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                      };


    inputStr[i] = new GUI_EL_InputString(&params, &inputStrArea[i], tx, 100, 0, (GUI_Obj*)this);
}

void CGuiMenu::setTitle(const char* title)
{
    titleStr.clear();
    titleStr.append(title);
}

void CGuiMenu::initItems(QList<std::string> text, const char* title, int focusItem)
{
    setTitle(title);

    for (int i = 0; i < text.size(); i++)
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

        item[i] = new GUI_EL_MenuItem(&itemParams, &itemArea[i], (char*)text[i].c_str(), true, f, (GUI_Obj*)this);
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

    GUI_EL_Window   window  (&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label    title   (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();

    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
        if (item[i] != nullptr)
            item[i]->Draw();

    for (int i = 0; i < 2; i++)
        if (inputStr[i] != nullptr)
            inputStr[i]->Draw();

    for (int i = 0; i < std::min(numItem, MAIN_MENU_MAX_LIST_SIZE); i++)
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

    delete tx;
}
