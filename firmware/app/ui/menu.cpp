#include <stdio.h>
#include "menu.h"
#include <string.h>
#include "all_sym_indicators.h"

CGuiMenu::CGuiMenu(MoonsGeometry* area, const char *title, const char *text, Alignment align):CGuiDialog(area),
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

    auto size = state.listItem.size();
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
                //if (k->inputStr.size() > 0){ k->inputStr.pop_back(); }
                break;
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

//    item[0] = new GUI_EL_MenuItem(&itemParams, &itemArea[0],  (char*)useScanMenu[0], true, true, (GUI_Obj*)this);

    window.Draw();
    title.Draw();

//    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
//        if (item[i] != nullptr)
//            item[i]->Draw();

//    for (int i = 0; i < MAIN_MENU_MAX_LIST_SIZE; i++)
//    {    if (item[i] != nullptr) delete item[i]; item[i] = nullptr; }
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
    MoonsGeometry volume_geom[4];
    volume_geom[0] = {  3,  25,  50,  55 };
    volume_geom[1] = {  3,  70,  50, 100 };
    volume_geom[2] = { 54,  25, 140,  55 };
    volume_geom[3] = { 54,  70, 140, 100 };

    GUI_EL_Label *volume[4];
    LabelParams param[4] = {GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode};

    for (int i = 0; i < 4; i++)
        param[i].transparent = true;

    if ( focus == 0 ) param[2].transparent = false;
    if ( focus == 1 ) param[3].transparent = false;

    for (int i = 0; i < 4; i++)
        volume[i] = new GUI_EL_Label (&param[i], &volume_geom[i],  NULL, (GUI_Obj*)this);

    volume[0]->SetText((char*)aru);
    volume[1]->SetText((char*)arm);

    for ( int i = 0; i < 2; i++)
    {
        if (aruArmStatus[i] == 1)
            volume[2+i]->SetText((char *)useScanMenu[0]);
        else
            volume[2+i]->SetText((char *)useScanMenu[1]);
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

    for (int i = 0; i < 4; i++)
        volume[i]->Draw();

    for (int i = 0; i < 4; i++)
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

void CGuiMenu::incrAruArm(GuiWindowsSubType type)
{
    switch (type)
    {
    case GuiWindowsSubType::aruarm:
        aruArmStatus[focus] = 1;
        break;
    default:
        break;
    }
}

void CGuiMenu::decrAruArm(GuiWindowsSubType type)
{
    switch (type)
    {
    case GuiWindowsSubType::aruarm:
        aruArmStatus[focus] = 0;
        break;
    default:
        break;
    }
}

void CGuiMenu::setSttParam(CEndState state, UI_Key key)
{
    GuiWindowsSubType type = state.subType;
    std::string *str; str = &state.listItem.front()->inputStr;

    switch ( type )
    {
    case setDate:

        break;
    case setTime:
        break;
    case setFreq:
        break;
    case setSpeed:
        if (key == keyBack && str->size() > 0)
        {}
        else if (key == keyBack && str->size() == 0)
        {}
        if ( key > 5 && key < 17)
        {
            if ( str->size() < 8)
            {
                str->push_back(key+42);
            }
        }

        break;
    default:
        break;
    }
}

void CGuiMenu::initSetParametersDialog(std::string text)
{
    MoonsGeometry volume_geom  = {  35,  40,  105,  65 };
    LabelParams label_param = GUI_EL_TEMP_LabelMode;
    //label_param = ;

    titleArea = { 35, 15, 105, 34};

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                           (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,   (char*)titleStr.c_str(), (GUI_Obj *)this);
    GUI_EL_Label  volume(&label_param,               &volume_geom, (char *)text.c_str()   , (GUI_Obj*)this);

    window.Draw();
    title.Draw();
    volume.Draw();
}

void CGuiMenu::inputMessage( CEndState state, UI_Key key)
{
    auto newTime = std::chrono::steady_clock::now();

    if ( key > 5 && key < 16)
    {
        if (prevKey == key)
        {
            qDebug() << "  " << ( newTime - ct ).count() << "\n";

            if ( ( newTime - ct ).count() < 500*(1000000) )
            {
                keyPressCount++;
                if ( keyPressCount > 4 ) keyPressCount = 0;
                auto elem = state.listItem.back();
                elem->inputStr.pop_back();
            }

            auto elem = state.listItem.back();
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
        }
        else
        {
            keyPressCount = 0;
            auto elem = state.listItem.back();
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
            prevKey = key;
        }

        ct = std::chrono::steady_clock::now();
    }

}

void CGuiMenu::initSmsInputDialog(const char* titleStr, std::string addrStr, std::string msgStr )
{
                  titleArea   = { 5,  5, 140,  20 };
    MoonsGeometry addrArea    = { 5, 25, 140,  44 };
    MoonsGeometry volume_geom = { 5, 47, 140, 115 };

    LabelParams param[2] = {GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode};

    for (int i = 0; i < 2; i++)
        param[i].transparent = true;

    if ( focus == 0 ) param[0].transparent = false;
    if ( focus == 1 ) param[1].transparent = false;

    GUI_EL_Window   window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                           (GUI_Obj *)this);
    GUI_EL_Label    title (&titleParams,               &titleArea,   (char*)titleStr,         (GUI_Obj *)this);
    GUI_EL_Label    addr  (&param[0],                  &addrArea,    (char*)addrStr.c_str(),  (GUI_Obj *)this);
    GUI_EL_TextArea volume(&param[1],                  &volume_geom, (char*)msgStr.c_str(),   (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    addr.Draw();
    volume.Draw();
}
