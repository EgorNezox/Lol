#include <stdio.h>
#include "menu.h"
#include <string.h>
#include "all_sym_indicators.h"

CGuiMenu::CGuiMenu(MoonsGeometry* area, const char *title, Alignment align):CGuiDialog(area),
    editing(false),
    focus(0),
    numItem(7)
{
    windowArea = {0,0,(GXT)(GEOM_W(this->area)),(GYT)(GEOM_H(this->area))};
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
        item [i] = nullptr;
        label[i] = nullptr;
    }

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

    int size = state.listItem.size();
    int i = 0;

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


void CGuiMenu::setCondCommParam(CEndState state, UI_Key key)
{
    int i = 0;
    for (auto &k: state.listItem)
    {
        if (i == focus)
        {
            switch (key)
            {
            case keyBack:
            {
                //if (k->inputStr.size() > 0){ k->inputStr.pop_back(); }
                break;
            }
            case keyEnter:
                break;
            default:
                if ( key > 5 && key < 16 && k->inputStr.size() < 2 )
                {
                    k->inputStr.push_back((char)(42+key));
                    // check
                    int rc = atoi(k->inputStr.c_str());
                    if ( state.listItem.size() == 2 )
                    {
                        if ( i == 0 && rc > 31 )
                        { k->inputStr.clear(); }
                        if ( i == 0 && rc > 99 )
                        { k->inputStr.clear(); }
                    }
                    if ( state.listItem.size() == 1 )
                    {
                        if ( i == 0 && rc > 99 )
                        { k->inputStr.clear(); }
                    }
                }
                break;
            }
        }
        i++;
    }
}

void CGuiMenu::initCondCommDialog(CEndState state)
{
    titleParams = GUI_EL_TEMP_LabelTitle;
    titleParams.element.align = {alignHCenter, alignTop};

    LabelParams params;
    params = GUI_EL_TEMP_CommonTextAreaLT;
    params.element.align = {alignLeft, alignTop};
    MoonsGeometry labelStrArea[6];

    itemParams.label_params = GUI_EL_TEMP_LabelMode; //GUI_EL_TEMP_LabelMode;
    itemParams.label_params.transparent = true;
    itemParams.label_params.element.align = {alignHCenter, alignTop};
    itemParams.icon_params.element  = GUI_EL_TEMP_CommonIcon;
    itemParams.icon_params.icon = sym_blank;

    int i = 0; bool f;
    int size = state.listItem.size();

    titleArea = {(GXT)(windowArea.xs + MARGIN),
                 (GYT)(windowArea.ys + MARGIN),
                 (GXT)(windowArea.xe - MARGIN),
                 (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )
                };

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    labelStrArea[0] = {  7, 25,  63, 55 };
    itemArea[0]     = { 65, 25, 150, 55 };

    labelStrArea[1] = {  7, 60,  63, 90 };
    itemArea[1]     = { 65, 60, 150, 90 };

    for (auto &k: state.listItem)
    {
        f = false;
        if (i == focus)
            f = true;

        label[i] = new GUI_EL_Label    (&params,     &labelStrArea[i], (char*)k->label,                     (GUI_Obj*)this);
        item [i] = new GUI_EL_MenuItem (&itemParams, &itemArea[i],     (char*)k->inputStr.c_str(), true, f, (GUI_Obj*)this);

        i++;
    }

    tButtonStatus status = tButtonStatus::bs_unselected;
    if (size == focus)
        status = tButtonStatus::bs_selected;

    LabelParams iParams = GUI_EL_TEMP_LabelButton;
    iParams.transparent = true;
    GUI_EL_Button *okItem;

    itemArea[size] = {40, 95, 120, 125 };
    okItem = new GUI_EL_Button(&iParams, &itemArea[size],  (char*)trans, status,  (GUI_Obj*)this);

    window.Draw();
    title.Draw();
    okItem->Draw();

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

     itemParams.label_params = GUI_EL_TEMP_LabelTitle;
     delete okItem;
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
    itemParams.icon_params.icon = sym_blank;

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
    MoonsGeometry volume_geom[6];
    volume_geom[0] = {  3,  20,  50,  48 };
    volume_geom[1] = {  3,  53,  50,  81 };
    volume_geom[2] = {  3,  86,  50, 114 };

    volume_geom[3] = { 70,  20, 140,  48 };
    volume_geom[4] = { 70,  53, 140,  81 };
    volume_geom[5] = { 70,  86, 140, 114 };

    GUI_EL_Label *volume[6];
    LabelParams param[6] = {GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode};

    for (int i = 0; i < 6; i++)
    {
        param[i].transparent = true;
        param[i].element.align = {alignHCenter, alignVCenter};
    }

    if ( focus == 0 ) param[3].transparent = false;
    if ( focus == 1 ) param[4].transparent = false;
    if ( focus == 2 ) param[5].transparent = false;

    for (int i = 0; i < 6; i++)
        volume[i] = new GUI_EL_Label (&param[i], &volume_geom[i],  NULL, (GUI_Obj*)this);

    volume[0]->SetText((char*)aruStr);
    volume[1]->SetText((char*)armStr);
    volume[2]->SetText((char*)ausStr);

    for ( int i = 0; i < 3; i++)
    {
        if (aruArmAsuStatus[i] == 1)
            volume[3+i]->SetText((char *)useScanMenu[0]);
        else
            volume[3+i]->SetText((char *)useScanMenu[1]);
    }

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

    for (int i = 0; i < 6; i++)
        volume[i]->Draw();

    for (int i = 0; i < 6; i++)
        delete volume[i];
}


void CGuiMenu::initIncludeDialog()
{
    MoonsGeometry volume_geom  = {  35,  40,  105,  70 };
    GUI_EL_Label *volume = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &volume_geom,  NULL, (GUI_Obj*)this);

    if (inclStatus == 1)
        volume->SetText((char *)useScanMenu[0]);
    else
        volume->SetText((char *)useScanMenu[1]);

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


void CGuiMenu::initGpsCoordinateDialog(std::string coord_lat, std::string coord_log)
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

    if (coord_lat.size() == 0)
    {
        coord_lat.append("0123.4567,N");
        coord_log.append("0123.4567,N");
    }

    volume[0]->SetText((char *)coord_lat.c_str());
    volume[1]->SetText((char *)coord_log.c_str());

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
    {
        if (label[i] != nullptr)
            label[i]->Draw();
        if (item[i] != nullptr)
            item[i]->Draw();
    }


    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
    {   if (item[i] != nullptr)
        { delete item[i]; item[i] = nullptr;}
        if (label[i] != nullptr)
        { delete label[i]; label[i] = nullptr;}
    }
}

CGuiMenu::~CGuiMenu()
{
    for (int i = 0; i < numItem; i++)
        if (item[i] != nullptr)
            delete item[i];

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
    default:
        break;
    }

    if ( key > 5 && key < 16)
    {
        value = 42+key;
        editing = true;
    }
    if ( focus == 0)
    {
        newDstAddr.push_back( value );
    }else if( focus == 1 ){
        newMessage.push_back( value );
    }
}

void CGuiMenu::setSttParam(CEndState state, UI_Key key)
{
    GuiWindowsSubType type = state.subType;
    std::string *str; str = &state.listItem.front()->inputStr;

    switch ( type )
    {
    case setDate:
    {
        break;
    }
    case setTime:
    {
        break;
    }
    case setFreq:
    case setSpeed:
    {
        if (key == keyBack && str->size() > 0)
        {
            str->pop_back();
        }
        else if (key == keyBack && str->size() == 0)
        {}
        if ( key > 5 && key < 16)
        {
            if ( str->size() == 0 && key != key0)
            {
                str->push_back(key+42);
            }
            else if ( str->size() > 0 && str->size() < 8 )
            {
                str->push_back(key+42);
            }
        }

        break;
    }
    default:
        break;
    }
}

void CGuiMenu::initSetParametersDialog(std::string text)
{
    MoonsGeometry volume_geom  = {  5,  45,  160,  95 };
    LabelParams label_param[2] = {GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode};

    titleArea = { 5, 10, 160, 35};
    label_param[0].transparent = true;
    label_param[1].transparent = true;

    label_param[0].element.align = { alignHCenter, alignTop };
    label_param[1].element.align = { alignHCenter, alignTop };

    GUI_EL_Window   window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                           (GUI_Obj *)this);
    GUI_EL_Label    title (&label_param[0],            &titleArea,   (char*)titleStr.c_str(), (GUI_Obj *)this);
    GUI_EL_TextArea volume(&label_param[1],            &volume_geom, (char*)text.c_str(),     (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    volume.Draw();
}

void CGuiMenu::initSetDateOrTimeDialog(std::string text)
{
    MoonsGeometry volume_geom  = {  5,  45,  160,  95 };
    LabelParams label_param[2] = {GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode};

    titleArea = { 5, 10, 160, 35};
    label_param[0].transparent = true;
    label_param[1].transparent = false;

    label_param[0].element.align = { alignHCenter, alignTop };
    label_param[1].element.align = { alignHCenter, alignTop };

    GUI_EL_Window   window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                           (GUI_Obj *)this);
    GUI_EL_Label    title (&label_param[0],            &titleArea,   (char*)titleStr.c_str(), (GUI_Obj *)this);
    GUI_EL_TextArea volume(&label_param[1],            &volume_geom, (char*)text.c_str(),     (GUI_Obj *)this);

    if ( text.size() == 8)
    {
        // нажмите enter для подтверждения
    }

    window.Draw();
    title.Draw();
    volume.Draw();
}

void CGuiMenu::inputGroupCondCmd( CEndState state, UI_Key key )
{
    auto elem = state.listItem.back();
    auto newTime = std::chrono::steady_clock::now();

    if ( ( newTime - ct ).count() < 800*(1000000) )
    {
        keyPressCount++;
        if ( keyPressCount > 1 )
            keyPressCount = 0;

        elem->inputStr.pop_back();
    }

    elem->inputStr.push_back(ch_key0[keyPressCount]);

    ct = std::chrono::steady_clock::now();
}

void CGuiMenu::inputSmsMessage( CEndState state, UI_Key key)
{
    auto newTime = std::chrono::steady_clock::now();

    if ( key > 5 && key < 16)
    {
        if (prevKey == key)
        {
            if ( ( newTime - ct ).count() < 800*(1000000) )
            {
                keyPressCount++;
                switch ( key )
                {
                case key0:
                    if ( keyPressCount > 1 ) keyPressCount = 0;
                    break;
                case key1:
                    if ( keyPressCount > 6) keyPressCount = 0;
                    break;
                default:
                    if ( keyPressCount > 4 ) keyPressCount = 0;
                    break;
                }

                auto elem = state.listItem.back();
                elem->inputStr.pop_back();
            }
        }
        else
        {
            keyPressCount = 0;
            prevKey = key;
        }

        auto elem = state.listItem.back();
//        if ( (elem->inputStr.size() > 0) && (elem->inputStr.size()%16 == 0) )
//        { elem->inputStr.push_back('\n'); }
        switch (key)
        {
        case key0:
            elem->inputStr.push_back(ch_key0[keyPressCount]);
            break;
        case key1:
            elem->inputStr.push_back(ch_key1[keyPressCount]);
            break;
        case key2:
            elem->inputStr.push_back(ch_key2[keyPressCount]);
            break;
        case key3:
            elem->inputStr.push_back(ch_key3[keyPressCount]);
            break;
        case key4:
            elem->inputStr.push_back(ch_key4[keyPressCount]);
            break;
        case key5:
            elem->inputStr.push_back(ch_key5[keyPressCount]);
            break;
        case key6:
            elem->inputStr.push_back(ch_key6[keyPressCount]);
            break;
        case key7:
            elem->inputStr.push_back(ch_key7[keyPressCount]);
            break;
        case key8:
            elem->inputStr.push_back(ch_key8[keyPressCount]);
            break;
        case key9:
            elem->inputStr.push_back(ch_key9[keyPressCount]);
            break;
        default:
            //
            break;
        }
        ct = std::chrono::steady_clock::now();
    }

}

void CGuiMenu::inputSmsAddr(CEndState state, UI_Key key)
{
    auto k = state.listItem.front();
    if ( key > 5 && key < 16 && k->inputStr.size() < 2 )
    {
        k->inputStr.push_back((char)(42+key));
        // check
        int rc = atoi(k->inputStr.c_str());

        if ( rc > 31 )
        { k->inputStr.clear(); }
    }
}

void CGuiMenu::initTxSmsDialog(const char* titleStr, std::string addrStr, std::string msgStr )
{
                  titleArea   = {  5,   5, 150,  20 };
    MoonsGeometry addrArea    = {  7,  20, 147,  40 };
    MoonsGeometry volume_geom = {  7,  35, 147, 100 };
    MoonsGeometry button_geom = { 40, 110, 110, 125 };

    LabelParams param[3] = {GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_LabelButton};
    param[0].element.align = {alignLeft, alignTop};
    param[1].element.align = {alignLeft, alignTop};

    for (int i = 0; i < 3; i++)
        param[i].transparent = true;

    if ( focus == 2 ) param[2].transparent = false;

    std::string str1, str2;

    if ( focus == 0 ){ str1.append("->"); }else{ /*if (focus != 2)*/str1.append("  "); }
    if ( focus == 1 ){ str2.append("->"); }else{ /*if (focus != 2)*/str2.append("  "); }
    str1.append(smsText[0]); str1.append(addrStr);
    str2.append(smsText[1]);/* str2.append(msgStr);*/

    int b;
    if ( msgStr.size() < 65 )
        b = 0;
    else
        b = msgStr.size()-65;

    for (int i = b; i < msgStr.size(); i++)
    {
        if ( (i-b)%16 == 0 && (i-b) != 0 )
            str2.push_back('\n');
        else
            str2.push_back( msgStr[i] );
    }

    GUI_EL_Window   window    (&GUI_EL_TEMP_WindowGeneral, &windowArea,                       (GUI_Obj *)this);
    GUI_EL_Label    title     ( &titleParams,              &titleArea,   (char*)titleStr,     (GUI_Obj *)this);
    GUI_EL_TextArea addr      (&param[0],                  &addrArea,    (char*)str1.c_str(), (GUI_Obj *)this);
    GUI_EL_TextArea volume    (&param[1],                  &volume_geom, (char*)str2.c_str(), (GUI_Obj *)this);
    GUI_EL_Label    ok_button (&param[2],                  &button_geom, (char*)trans,        (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    addr.Draw();
    volume.Draw();
    ok_button.Draw();
}

void CGuiMenu::initRxSmsDialog()
{
                  titleArea   = {  5,   5, 150,  20 };
    MoonsGeometry button_geom;
    LabelParams param = GUI_EL_TEMP_CommonTextAreaLT;
    param.element.align = {alignHCenter, alignVCenter};
    param.transparent = false;

    GUI_EL_Window   window    ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                           (GUI_Obj *)this);
    GUI_EL_Label    title     ( &titleParams,               &titleArea,   (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();

    if (recvStage == 0)
    {
        param.transparent = false;
        button_geom = { 30, 40, 130, 80 };
    }
    else
    {
        param.transparent = true;
        button_geom = { 30, 40, 130, 80 };
    }

    GUI_EL_Label    ok_button ( &param, &button_geom, (char*)receiveStatusStr[recvStage], (GUI_Obj *)this);
    ok_button.Draw();

}

void CGuiMenu::initGroupCondComm( CEndState state )
{
    switch( groupCondCommStage )
    {
    case 0: // set frequence
    {
                      titleArea  = {  5,   5, 150,  20 };
        MoonsGeometry freq1Area  = {  5,  21, 150,  41 };
        MoonsGeometry value1Area = {  5,  42, 150,  62 };
        MoonsGeometry freq2Area  = {  5,  63, 150,  83 };
        MoonsGeometry value2Area = {  5,  84, 150, 104 };
        MoonsGeometry buttonArea = {  5, 105, 150, 125 };

        LabelParams param[5] = {GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_LabelButton};

        for (int i = 0; i < 5; i++)
            param[i].transparent = true;

        if ( focus == 0 )
            param[1].transparent = false;
        if ( focus == 1 )
            param[3].transparent = false;
        if ( focus == 2 )
            param[4].transparent = false;

        param[0].element.align = {alignHCenter, alignVCenter};
        param[2].element.align = {alignHCenter, alignVCenter};
        param[4].element.align = {alignHCenter, alignVCenter};

        param[1].element.align = {alignRight, alignVCenter};
        param[3].element.align = {alignRight, alignVCenter};

        std::string str1, str2;
        str1.append(groupCondCommFreqStr); str1.push_back('1');
        str2.append(groupCondCommFreqStr); str2.push_back('2');

        auto freq = state.listItem.begin();
        std::string freq1 = (*freq)->inputStr; freq1.append(" ").append(freq_hz);
        *freq++;
        std::string freq2 = (*freq)->inputStr;  freq2.append(" ").append(freq_hz);

        GUI_EL_Window window     ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj*)this );
        GUI_EL_Label  title      ( &titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj*)this );
        GUI_EL_Label  freq1Title ( &param[0],                  &freq1Area,  (char*)str1.c_str(),     (GUI_Obj*)this );
        GUI_EL_Label  freq1Value ( &param[1],                  &value1Area, (char*)freq1.c_str(),    (GUI_Obj*)this );
        GUI_EL_Label  freq2Title ( &param[2],                  &freq2Area,  (char*)str2.c_str(),     (GUI_Obj*)this );
        GUI_EL_Label  freq2Value ( &param[3],                  &value2Area, (char*)freq2.c_str(),    (GUI_Obj*)this );
        GUI_EL_Label  ok_button  ( &param[4],                  &buttonArea, (char*)continueStr,      (GUI_Obj*)this );

        window.Draw();
        title.Draw();
        freq1Title.Draw();
        freq1Value.Draw();
        freq2Title.Draw();
        freq2Value.Draw();
        ok_button.Draw();
        break;
    }
    case 1:
    {
        auto iter = state.listItem.begin();
        (*iter)++;
        (*iter)++;
        MoonsGeometry addressTitleArea, addressValueArea, commandTitleArea, commandValueArea;
                      titleArea        = {  5,   5, 150,  20 };
      if ( state.listItem.size() == 4 )
      {
          addressTitleArea = {  5,  21, 150,  41 };
          addressValueArea = {  5,  42, 150,  62 };
          commandTitleArea = {  5,  63, 150,  83 };
          commandValueArea = {  5,  84, 150, 104 };
      }
      else
      {
          commandTitleArea = {  5,  21, 150,  41 };
          commandValueArea = {  5,  42, 150,  62 };
      }
          MoonsGeometry buttonArea       = {  5, 105, 150, 125};

        LabelParams param[5] = {GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_LabelButton};

        for (int i = 0; i < 5; i++)
            param[i].transparent = true;

        if ( focus == 0 && state.listItem.size() == 4)
            param[1].transparent = false;
        if ( (focus == 1 && state.listItem.size() == 4) || (focus == 0 && state.listItem.size() == 3) )
            param[3].transparent = false;
        if ( (focus == 2  && state.listItem.size() == 4) || (focus == 1 && state.listItem.size() == 3))
            param[4].transparent = false;

        param[0].element.align = {alignHCenter, alignVCenter};
        param[2].element.align = {alignHCenter, alignVCenter};
        param[4].element.align = {alignHCenter, alignVCenter};

        param[1].element.align = {alignLeft, alignVCenter};
        param[3].element.align = {alignLeft, alignVCenter};

        if ( state.listItem.size() == 4 )
        {
            GUI_EL_Window window       ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                                         (GUI_Obj*)this );
            GUI_EL_Label  title        ( &titleParams,               &titleArea,        (char*)titleStr.c_str(),          (GUI_Obj*)this );
            GUI_EL_Label  addressTitle ( &param[0],                  &addressTitleArea, (char*)callTitle[1],              (GUI_Obj*)this );
            GUI_EL_Label  addressValue ( &param[1],                  &addressValueArea, (char*)(*iter)->inputStr.c_str(), (GUI_Obj*)this );
            GUI_EL_Label  commandTitle ( &param[2],                  &commandTitleArea, (char*)reciveSubMenu[4],          (GUI_Obj*)this );
            (*iter)++; std::string str;
            if ((*iter)->inputStr.size() < 20)
                str.append((*iter)->inputStr);
            else
                str.append((*iter)->inputStr.substr( (*iter)->inputStr.size()-20, 20));

            GUI_EL_Label commandValue ( &param[3], &commandValueArea, (char*)str.c_str(), (GUI_Obj*)this );
            GUI_EL_Label ok_button    ( &param[4], &buttonArea,       (char*)trans,       (GUI_Obj*)this );

            window.Draw();
            title.Draw();
            addressTitle.Draw();
            addressValue.Draw();
            commandTitle.Draw();
            commandValue.Draw();
            ok_button.Draw();
        }

        if ( state.listItem.size() == 3 )
        {
            (*iter)++;

            GUI_EL_Window window       ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                                (GUI_Obj*)this );
            GUI_EL_Label  title        ( &titleParams,               &titleArea,        (char*)titleStr.c_str(), (GUI_Obj*)this );
            GUI_EL_Label  commandTitle ( &param[2],                  &commandTitleArea, (char*)reciveSubMenu[4], (GUI_Obj*)this );
            std::string str;
            if ((*iter)->inputStr.size() < 20)
                str.append((*iter)->inputStr);
            else
                str.append((*iter)->inputStr.substr( (*iter)->inputStr.size()-20, 20));
            GUI_EL_Label  commandValue ( &param[3],                  &commandValueArea, (char*)str.c_str(),   (GUI_Obj*)this );
            GUI_EL_Label  ok_button    ( &param[4],                  &buttonArea,       (char*)trans,         (GUI_Obj*)this );

            window.Draw();
            title.Draw();
            commandTitle.Draw();
            commandValue.Draw();
            ok_button.Draw();
        }
        break;
    }
    default:
        break;
    }
}
