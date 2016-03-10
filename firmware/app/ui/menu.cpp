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

    for (int i = 0; i < numItem; i++)
        item[i] = nullptr;
}

void CGuiMenu::Draw(){
    // title
    titleArea = {(GXT)(windowArea.xs + MARGIN),
                 (GYT)(windowArea.ys + MARGIN),
                 (GXT)(windowArea.xe - MARGIN),
                 (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )
                };

    for (int i = 0; i < numItem; i++)
        if (item[i] != nullptr)
            delete item[i];

    for (int i = 0; i < std::min(numItem, MAIN_MENU_LIST_SIZE); i++)
    {

        itemArea[i] = {(GXT)(windowArea.xs + MARGIN),
                       (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                       (GXT)(windowArea.xe - MARGIN),
                       (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                      };
        if (i == CGuiMenu::focus)
            item[i] = new GUI_EL_MenuItem(&itemParams, &itemArea[i], mainMenu[i], true, true, (GUI_Obj*)this);
        else
            item[i] = new GUI_EL_MenuItem(&itemParams, &itemArea[i], mainMenu[i], true, false, (GUI_Obj*)this);
    }

    GUI_EL_Window   window  (&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label    title   (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();

    for (int i = 0; i < std::min(numItem, MAIN_MENU_LIST_SIZE); i++)
        item[i]->Draw();
}

CGuiMenu::~CGuiMenu()
{
    for (int i = 0; i < numItem; i++)
        if (item[i] != nullptr)
            delete item[i];
}
