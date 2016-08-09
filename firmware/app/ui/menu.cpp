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

    itemParams.label_params = /*GUI_EL_TEMP_LabelTitle*/GUI_EL_TEMP_LabelChannel/*GUI_EL_TEMP_LabelText*/;
    itemParams.label_params.element.align = {alignHCenter, alignVCenter};
    itemParams.icon_params.element  = GUI_EL_TEMP_CommonIcon;
    itemParams.icon_params.icon = sym_blank;

    tx = nullptr;
    tx = new char[100];

    GUI_EL_TEMP_WindowGeneral.frame_thick = 0;
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
    std::string str, labelStr;
    auto iter = state.listItem.begin();

    //[0] - CMD, [1] - R_ADDR, [2] - retrans
    switch (txCondCmdStage)
    {
    case 0: /* Group <-> Individual <-> Ticket */
    {
        labelStr.append("Mode\0");
        str.append(smplSubMenu[condCmdModeSelect]);
        break;
    }
    case 1:
    {
        // simple, individual
        if (state.subType == condCommand && state.listItem.size() == 3)
        {
            // с ретранслятором/ без ретранстятора
            labelStr.append(condCommStr[3]);
            if (useCmdRetrans)
                str.append(useScanMenu[0]);
            else
                str.append(useScanMenu[1]);
        }
        else
        {
            labelStr.append(pressEnter);
            str.append("");
        }

        break;
    }
    case 2:
    { // ввод адреса ретранслятора
        (*iter)++;
        if ((*iter)->inputStr.size() == 0)
            str.append("--");
        else if ((*iter)->inputStr.size() == 1)
        { str.append("-"); }

        str.append((*iter)->inputStr);
        labelStr.append(condCommStr[1]);
        break;
    }
    case 3:
    { // ввод адреса получателя
        (*iter)++;(*iter)++;
        if ((*iter)->inputStr.size() == 0)
            str.append("--");
        else if ((*iter)->inputStr.size() == 1)
        { str.append("-"); }

        str.append((*iter)->inputStr);
        labelStr.append(condCommStr[0]);
        break;
    }
    case 4:
    { // ввод условной команды
        if ((*iter)->inputStr.size() == 0)
            str.append("--");
        else if ((*iter)->inputStr.size() == 1)
        { str.append("-"); }

        str.append((*iter)->inputStr);
        labelStr.append(condCommStr[2]);
        break;
    }
    case 5:
    { // send
        labelStr.append(condCommSendStr);
        break;
    }
    default:
    {break;}
    }

    LabelParams params = GUI_EL_TEMP_LabelMode;
    params.element.align = {alignHCenter, alignVCenter};
    params.transparent = true;

    MoonsGeometry localLabelArea = { 7,  5, 150,  39 };
    MoonsGeometry localFieldArea = { 7, 40, 150, 125 };

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                               (GUI_Obj *)this);
    GUI_EL_Label  label (&GUI_EL_TEMP_LabelTitle,    &localLabelArea,  (char*)labelStr.c_str(), (GUI_Obj *)this);
    GUI_EL_Label  field (&params,                    &localFieldArea,  (char*)str.c_str(),      (GUI_Obj *)this);

    window.Draw();
    label.Draw();
    field.Draw();
}

void CGuiMenu::initTwoStateDialog()
{
    int i = 0;
    MoonsGeometry itemArea = {(GXT)(windowArea.xs + 7*MARGIN),
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

    itemArea = {  20,  20,  120,  90 };

    itemParams.label_params = GUI_EL_TEMP_LabelChannel;
    itemParams.label_params.element.align = {alignHCenter, alignTop};
    itemParams.icon_params.element  = GUI_EL_TEMP_CommonIcon;
    itemParams.icon_params.icon = sym_blank;

    window.Draw();
    title.Draw();
}

void CGuiMenu::initVolumeDialog()
{
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
    MoonsGeometry volume_geom[2];
    volume_geom[0]  = {  5,  30,  140,  60 };
    volume_geom[1]  = {  5,  60,  140,  90 };

    GUI_EL_Label* volume[2];

    volume[0] = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &volume_geom[0],  NULL, (GUI_Obj*)this);
    volume[1] = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &volume_geom[1],  NULL, (GUI_Obj*)this);

    if (coord_lat.size() == 0)
    {
        coord_lat.append("0000.0000,N");
        coord_log.append("0000.0000,N");
    }

    volume[0]->SetText((char *)coord_lat.c_str());
    volume[1]->SetText((char *)coord_log.c_str());
    volume[0]->transparent = true;
    volume[1]->transparent = true;
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

void CGuiMenu::initItems(std::list<std::string> text, const char* title_str, int focusItem)
{
    setTitle(title_str);
    // title
    titleArea = {(GXT)(windowArea.xs + MARGIN),
                 (GYT)(windowArea.ys + MARGIN),
                 (GXT)(windowArea.xe - MARGIN),
                 (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )
                };

    titleParams.element.align = {alignHCenter, alignTop};
    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();

    itemParams.label_params.transparent = true;
    itemParams.label_params.element.align = {alignHCenter, alignVCenter};
    itemParams.icon_params.icon = sym_blank;
    int i = 0, j = 0; MoonsGeometry itemArea;


    if (text.size() < 4)
    {
        for (auto &k: text)
        {

            itemArea = {(GXT)(windowArea.xs + MARGIN),
                        (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                        (GXT)(windowArea.xe - MARGIN),
                        (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                       };

            if (i == focusItem)
                itemParams.label_params.color_sch = {GENERAL_BACK_COLOR, GENERAL_TEXT_COLOR};
            else
                itemParams.label_params.color_sch = {GENERAL_TEXT_COLOR, GENERAL_BACK_COLOR};

            GUI_EL_MenuItem item(&itemParams, &itemArea, (char*)k.c_str(), false, true, (GUI_Obj*)this);
            item.Draw();
            i++;
        }
    }
    else
    {
        MoonsGeometry sliderArea  = { 150, 25, 157, 110};
        SliderParams  sliderParams = {(int32_t)text.size(), (int32_t)1, (int32_t)focusItem};
        GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this);
                      slider.Draw();

        for (auto &k: text)
        {
            if (j < offset) { ++j; continue;}

            if (i > 4) break;

            itemArea = {(GXT)(windowArea.xs + MARGIN),
                        (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                        (GXT)(windowArea.xe - 3*MARGIN),
                        (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                       };
            if (focusItem-offset == i)
                itemParams.label_params.color_sch = {GENERAL_BACK_COLOR, GENERAL_TEXT_COLOR};
            else
                itemParams.label_params.color_sch = {GENERAL_TEXT_COLOR, GENERAL_BACK_COLOR};

            GUI_EL_MenuItem item(&itemParams, &itemArea, (char*)k.c_str(), false, true, (GUI_Obj*)this);
            item.Draw();
            i++;
        }
    }
    itemParams.label_params.color_sch = {GENERAL_TEXT_COLOR, GENERAL_BACK_COLOR};
}

CGuiMenu::~CGuiMenu()
{
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

void CGuiMenu::initSetSpeedDialog()
{
    MoonsGeometry spbox_geom  = {  5,  45,  160,  95 };
    LabelParams label_param[2] = {GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode};

    titleArea = { 5, 10, 160, 35};
    label_param[0].transparent = true;
    label_param[1].transparent = true;

    label_param[0].element.align = { alignHCenter, alignTop };
    label_param[1].element.align = { alignHCenter, alignTop };

    GUI_EL_Window   window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                           (GUI_Obj *)this);
    GUI_EL_Label    title (&label_param[0],            &titleArea,   (char*)titleStr.c_str(), (GUI_Obj *)this);

    SpBoxParams spbox_params = GUI_EL_TEMP_CommonSpBox;

    SpBoxSettings spbox_settings;

                  spbox_settings.value = 600;
                  spbox_settings.min = 600;
                  spbox_settings.max = 4800;
                  spbox_settings.step = 600;
                  spbox_settings.spbox_len = 4;
                  spbox_settings.cyclic = false;

    GUI_EL_SpinBox  volume(&spbox_geom, &spbox_params, &spbox_settings, (GUI_Obj*)this);
    volume.lab_params->font = GUI_EL_TEMP_LabelMode.font;
    volume.SetActiveness(true);
    //GUI_EL_TextArea volume(&label_param[1],            &volume_geom, (char*)text.c_str(),     (GUI_Obj *)this);

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

    if ( text.size() == 8 )
    {
        // нажмите enter для подтверждения
    }

    window.Draw();
    title.Draw();
    volume.Draw();
}

void CGuiMenu::inputGroupCondCmd( CEndState state )
{
    auto elem = state.listItem.back();
    auto newTime = std::chrono::steady_clock::now();

    if ( ( newTime - ct ).count() < 900*(1000000) )
    {
        keyPressCount++;
        if ( keyPressCount > 1 )
            keyPressCount = 0;

        elem->inputStr.pop_back();
        elem->inputStr.push_back(ch_key0[keyPressCount]);
    }
    else
    {
        keyPressCount = 0;
        elem->inputStr.push_back(ch_key0[keyPressCount]);
    }

    ct = std::chrono::steady_clock::now();
}

void CGuiMenu::initTxPutOffVoiceDialog(int status)
{
    MoonsGeometry titleArea  = { 7,  6, 147,  20 };
    MoonsGeometry labelArea  = { 7, 21, 147,  35 };
    MoonsGeometry fieldArea  = { 7, 36, 147, 125 };

    LabelParams param;

    switch (putOffVoiceStatus)
    {
    case 1:
    {
        LabelParams param = GUI_EL_TEMP_LabelChannel;
        param.element.align = {alignHCenter, alignTop};
        param.transparent = true;

        std::string str;
        if (channalNum.size() == 0)
            str.append("__");
        else if (channalNum.size() == 1)
        {
            str.push_back('_');
            str.append(channalNum);
        }
        else
            str.append(channalNum);

        str.push_back('\0');

        GUI_EL_Window window ( &GUI_EL_TEMP_WindowGeneralBack, &windowArea,                              (GUI_Obj *)this);
        GUI_EL_Label  title  ( &titleParams,                   &titleArea,  (char*)voicePostTitleStr[0], (GUI_Obj *)this);
        GUI_EL_Label  label  ( &titleParams,                   &labelArea,  (char*)voiceRxTxLabelStr[0], (GUI_Obj *)this);
        GUI_EL_Label  field  ( &param,                         &fieldArea,  (char*)str.c_str(),          (GUI_Obj *)this);

        window.Draw();
        title.Draw();
        label.Draw();
        field.Draw();
        break;
    }
    case 2:
    {
        param = GUI_EL_TEMP_CommonTextAreaLT;
        param.element.align = {alignLeft, alignTop};
        param.transparent = true;

        GUI_EL_Label label( &titleParams, &labelArea, (char*)voiceRxTxLabelStr[2], (GUI_Obj *)this);
        GUI_EL_Label field( &param,       &fieldArea, (char*)smatrHSStateStr[status], (GUI_Obj *)this);

        label.Draw();
        field.Draw();
        break;
    }
    case 3:
    {
        param = GUI_EL_TEMP_LabelChannel;
        param.element.align = {alignHCenter, alignTop};
        param.transparent = true;

        std::string str;
        if (voiceAddr.size() == 0)
            str.append("__");
        else if (voiceAddr.size() == 1)
        {
            str.push_back('_');
            str.append(voiceAddr);
        }
        else
            str.append(voiceAddr);

        str.push_back('\0');

        GUI_EL_Label label( &titleParams, &labelArea, (char*)voiceRxTxLabelStr[4], (GUI_Obj *)this);
        GUI_EL_Label field( &param,       &fieldArea, (char*)str.c_str(),          (GUI_Obj *)this);

        label.Draw();
        field.Draw();
        break;
    }
    case 4:
    {
        param   = GUI_EL_TEMP_CommonTextAreaLT;
        param.element.align = {alignHCenter, alignTop};
        param.transparent   = true;

        GUI_EL_Label label( &titleParams, &labelArea, (char*)"", (GUI_Obj *)this);
        GUI_EL_Label field( &param, &fieldArea, (char*)startAleTxVoiceMailStr, (GUI_Obj *)this);

        label.Draw();
        field.Draw();
        break;
    }
    case 5:
    {
        param = GUI_EL_TEMP_CommonTextAreaLT;
        param.element.align = {alignLeft, alignTop};
        param.transparent   = true;

        std::string str; str.append(aleStateStr[status]);
        if (status == 13)
        {
            char ch[4];
            sprintf(ch, "%d", vmProgress);
            if (vmProgress != 100) ch[2] = '\0'; ch[3] = '\0';
            str.append(" ");
            str.append(ch);
            str.push_back('%');
        }

        GUI_EL_Label label( &titleParams, &labelArea, (char*)"",          (GUI_Obj *)this);
        GUI_EL_Label field( &param,       &fieldArea, (char*)str.c_str(), (GUI_Obj *)this);

        label.Draw();
        field.Draw();

        break;
    }
    default:
        break;
    }
}

void CGuiMenu::initRxPutOffVoiceDialog(int status)
{
    MoonsGeometry title_geom  = { 5,  5, 150,  19 };
    MoonsGeometry label_geom  = { 5, 20, 150,  34 };
    MoonsGeometry field_geom  = { 7, 35, 147, 110 };

    LabelParams label_param = GUI_EL_TEMP_CommonTextAreaLT;
    label_param.element.align = {alignHCenter, alignTop};
    label_param.transparent = true;

    switch(putOffVoiceStatus)
    {
    case 1:
    {
        LabelParams param = GUI_EL_TEMP_CommonTextAreaLT;
        param.element.align = {alignHCenter, alignTop};
        param.transparent = true;

        GUI_EL_Window window( &GUI_EL_TEMP_WindowGeneral, &windowArea,                              (GUI_Obj *)this);
        GUI_EL_Label  title ( &label_param,               &title_geom, (char*)voicePostTitleStr[1], (GUI_Obj *)this);
        GUI_EL_Label  label ( &label_param,               &label_geom, (char*)"",                   (GUI_Obj *)this);
        GUI_EL_Label  field ( &param,                     &field_geom, (char*)voiceRxStr[0],        (GUI_Obj *)this);

        window.Draw();
        title.Draw();
        label.Draw();
        field.Draw();
        break;
    }
    case 2:
    {
        LabelParams param = GUI_EL_TEMP_CommonTextAreaLT;
        param.element.align = {alignLeft, alignVCenter};
        param.transparent = true;

        std::string str; str.append(aleStateStr[status]);
        if (status == 9)
        {
            char ch[4];
            sprintf(ch, "%d", vmProgress);
            if (vmProgress != 100) ch[2] = '\0'; ch[3] = '\0';
            str.append(" ");
            str.append(ch);
            str.push_back('%');
        }

        GUI_EL_Label label( &label_param, &label_geom, (char*)"", (GUI_Obj *)this);
        GUI_EL_Label field( &param,       &field_geom, (char*)str.c_str(),      (GUI_Obj *)this);

        label.Draw();
        field.Draw();
        break;
    }
    case 3:
    {
        MoonsGeometry labelArea   = {  7, 20, 147,  24 };
        MoonsGeometry textArea    = {  7, 25,  59,  84 };
        MoonsGeometry addrArea    = { 60, 25, 147,  84 };
        MoonsGeometry volume_geom = {  7, 85, 147, 125 };

        LabelParams param[3] = {GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_LabelChannel, GUI_EL_TEMP_CommonTextAreaLT};
        param[0].element.align = {alignRight,   alignVCenter};
        param[1].element.align = {alignLeft,    alignTop};
        param[2].element.align = {alignHCenter, alignTop};

        for (int i = 0; i < 3; i++)
            param[i].transparent = true;

        std::string str;
        if (voiceAddr.size() < 1)
            str.append("--\0");
        else
            str.append(voiceAddr);

        GUI_EL_Label    label     ( &titleParams, &labelArea,   (char*)voiceRxTxLabelStr[5], (GUI_Obj *)this);
        GUI_EL_Label    text      ( &param[0],    &textArea,    (char*)voiceRxStr[1],        (GUI_Obj *)this);
        GUI_EL_Label    addr      ( &param[1],    &addrArea,    (char*)str.c_str(),          (GUI_Obj *)this);
        GUI_EL_Label    prompt    ( &param[2],    &volume_geom, (char*)voiceRxStr[2],        (GUI_Obj *)this);

        label.Draw();
        text.Draw();
        addr.Draw();
        prompt.Draw();
        break;
    }
    case 4:
    {
        LabelParams param = GUI_EL_TEMP_LabelChannel;
        param.element.align = {alignHCenter, alignVCenter};
        param.transparent = true;

        std::string str;
        if (channalNum.size() == 0)
            str.append("__");
        else if (channalNum.size() == 1)
        {
            str.push_back('_');
            str.append(channalNum);
        }
        else
            str.append(channalNum);

        str.push_back('\0');

        GUI_EL_Label label( &label_param, &label_geom, (char*)voiceRxTxLabelStr[1], (GUI_Obj *)this);
        GUI_EL_Label field( &param,       &field_geom, (char*)str.c_str(),          (GUI_Obj *)this);

        label.Draw();
        field.Draw();
        break;
    }
    case 5:
    {
        LabelParams param = GUI_EL_TEMP_CommonTextAreaLT;
        param.element.align = {alignLeft, alignVCenter};
        param.transparent = true;

        GUI_EL_Label label     ( &titleParams, &label_geom, (char*)voiceRxTxLabelStr[3],      (GUI_Obj *)this);
        GUI_EL_Label field     ( &param,       &field_geom, (char*)smatrHSStateStr[status],   (GUI_Obj *)this);

        label.Draw();
        field.Draw();
        break;
    }
    default:
    {
        break;
    }
    }
}

void CGuiMenu::initEditRnKeyDialog()
{
    MoonsGeometry labelArea   = { 7, 10, 147, 30 };
    MoonsGeometry addrArea    = { 7, 40, 147, 80 };

    LabelParams param = GUI_EL_TEMP_LabelChannel;
    param.element.align = {alignHCenter, alignVCenter};
    param.transparent = true;

    std::string str;
    if (RN_KEY.size() == 0)
        str.append("000");
    else if (RN_KEY.size() == 1)
    {
        str.push_back('0');
        str.push_back('0');
        str.append(RN_KEY);
    }
    else if (RN_KEY.size() == 2)
    {
        str.push_back('0');
        str.append(RN_KEY);
    }
    else
        str.append(RN_KEY);

    str.push_back('\0');

    GUI_EL_Window   window    ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                              (GUI_Obj *)this);
    GUI_EL_Label    label     ( &titleParams,               &labelArea,  (char*)voiceRxTxLabelStr[1], (GUI_Obj *)this);
    GUI_EL_TextArea addr      ( &param,                     &addrArea,   (char*)str.c_str(),          (GUI_Obj *)this);

    window.Draw();
    label.Draw();
    addr.Draw();
}

void CGuiMenu::inputSmsMessage(std::string *field, UI_Key key)
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

                if (field->size() > 0) // todo: это вызывает сбой
                    field->pop_back();
            }
            else
                keyPressCount = 0;
        }
        else
        {
            keyPressCount = 0;
        }

        prevKey = key;

        switch (key)
        {
        case key0:
            field->push_back(ch_key0[keyPressCount]);
            break;
        case key1:
            field->push_back(ch_key1[keyPressCount]);
            break;
        case key2:
            field->push_back(ch_key2[keyPressCount]);
            break;
        case key3:
            field->push_back(ch_key3[keyPressCount]);
            break;
        case key4:
            field->push_back(ch_key4[keyPressCount]);
            break;
        case key5:
            field->push_back(ch_key5[keyPressCount]);
            break;
        case key6:
            field->push_back(ch_key6[keyPressCount]);
            break;
        case key7:
            field->push_back(ch_key7[keyPressCount]);
            break;
        case key8:
            field->push_back(ch_key8[keyPressCount]);
            break;
        case key9:
            field->push_back(ch_key9[keyPressCount]);
            break;
        default:
            //
            break;
        }
        ct = std::chrono::steady_clock::now();
    }

}

void CGuiMenu::inputSmsAddr(std::string *field, UI_Key key)
{
    if ( key > 5 && key < 16 && field->size() < 2 )
    {
        field->push_back((char)(42+key));
        // check
        int rc = atoi(field->c_str());

        if ( rc > 31 )
        { field->clear(); }
    }
}

void CGuiMenu::initTxSmsDialog(std::string titleStr, std::string fieldStr )
{

    MoonsGeometry title_geom = {  5,   5, 150,  20 };
    MoonsGeometry field_geom  = {  7,  40, 147,  60 };

    LabelParams param[2] = {GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_CommonTextAreaLT};
    param[0].element.align = {alignHCenter, alignTop};
    param[1].element.align = {alignHCenter, alignVCenter};

    switch(smsTxStage)
    {
    case 1:
    {
        break;
    }
    case 2:
    {
        if (fieldStr.size() == 0)
            fieldStr.append("--");
//        else if (fieldStr.size() == 1)
//        {
//            fieldStr.push_back('-');
//            fieldStr.append(fieldStr);
//        }
        else
            fieldStr.append(fieldStr);

        break;
    }
    case 3:
    {
        if (fieldStr.size() == 0)
            fieldStr.append("--");
//        else if (fieldStr.size() == 1)
//        {
//            fieldStr.push_back('--');
//            fieldStr.append(fieldStr);
//        }
        else
            fieldStr.append(fieldStr);

        break;
    }
    case 4:
    {
        std::string tmp; tmp.append(fieldStr);
        fieldStr.clear();

        for (uint8_t i = 0; i < fieldStr.size(); i++)
        {
            if ( (i%15 == 0) && (i != 0) )
                fieldStr.push_back('\n');

            fieldStr.push_back( fieldStr[i] );
        }

        field_geom = {  7,  20, 158,  120 };
        param[1] = GUI_EL_TEMP_CommonTextAreaLT;

        break;
    }
    case 5:
    {
        break;
    }
    default:
    {break;}
    }

    GUI_EL_Window   window (&GUI_EL_TEMP_WindowGeneral, &windowArea,         (GUI_Obj *)this);
    GUI_EL_Label    title  (&param[0], &title_geom, (char*)titleStr.c_str(), (GUI_Obj *)this);
    GUI_EL_TextArea field  (&param[1], &field_geom, (char*)fieldStr.c_str(), (GUI_Obj *)this);
    window.Draw();
    title.Draw();
    field.Draw();
}

void CGuiMenu::initTxGroupCondComm(CEndState state)
{
    std::string str, buttonStr;
    auto iter = state.listItem.begin();

    switch (txGroupCondCommStatus)
    {
    case 1:
    { // ввод частоты передачи
        str = (*iter)->inputStr; str.append(" ")\
                .append(freq_hz);
        buttonStr.append("next");
        break;
    }
    case 2:
    { // ввод частоты приема
        (*iter)++;
        str = (*iter)->inputStr;  str.append(" ")\
                .append(freq_hz);
        buttonStr.append("next");
        break;
    }
    case 3:
    { // ввод адреса получателя
        (*iter)++;
        (*iter)++;
        str = (*iter)->inputStr;
        buttonStr.append("next");
        break;
    }
    case 4:
    { // ввод сообщения
        (*iter)++;
        (*iter)++;
        (*iter)++;
        str = (*iter)->inputStr;
        buttonStr.append("send");
        break;
    }
    default:
    {
        break;
    }
    }
    titleArea   = { 5,  5, 150,  20 };
    MoonsGeometry addrArea    = { 7, 20, 147,  40 };
    MoonsGeometry volume_geom = { 7, 35, 147, 100 };

    LabelParams param[2] = {GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_CommonTextAreaLT};
    param[0].element.align = {alignLeft, alignTop};
    param[1].element.align = {alignLeft, alignTop};

    for (int i = 0; i < 2; i++)
        param[i].transparent = true;

    if ( focus == 2 ) param[2].transparent = false;

    GUI_EL_Window   window    (&GUI_EL_TEMP_WindowGeneral, &windowArea,                            (GUI_Obj *)this);
    GUI_EL_Label    title     (&titleParams,               &titleArea,   (char*)titleStr.c_str(),  (GUI_Obj *)this);
    GUI_EL_TextArea field     (&param[0],                  &addrArea,    (char*)str.c_str(),       (GUI_Obj *)this);
    GUI_EL_Label    ok_button (&param[1],                  &volume_geom, (char*)buttonStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    field.Draw();
    ok_button.Draw();
}

void CGuiMenu::initRxSmsDialog(std::string str)
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

    GUI_EL_Label    ok_button ( &param, &button_geom, (char*)str.c_str(), (GUI_Obj *)this);
    ok_button.Draw();
}

void CGuiMenu::initRxCondCmdDialog()
{
    titleArea   = { 5,  5, 150, 20 };

    LabelParams param = GUI_EL_TEMP_CommonTextAreaLT;
    param.element.align = {alignHCenter, alignVCenter};
    param.transparent = false;

    GUI_EL_Window   window    ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                    (GUI_Obj *)this);
    GUI_EL_Label    title     ( &titleParams,               &titleArea,   (char*)ticketStr[0], (GUI_Obj *)this);

    window.Draw();


    //    if (rxCondCmdStatus == 1)
    //    {
    //        title.Draw();
    //        param = GUI_EL_TEMP_LabelMode;
    //        param.transparent = true;
    //        MoonsGeometry buttonArea  = { 9, 40, 110, 80 };
    //        GUI_EL_Label    button ( &param, &buttonArea, (char*)useScanMenu[useTicket], (GUI_Obj *)this);
    //        button.Draw();
    //    }
    //    else
    {
        title.SetText((char*)callSubMenu[0]);
        title.Draw();
        if (recvStage == 0)
            param.transparent = false;
        else
            param.transparent = true;

        MoonsGeometry buttonArea  = { 29, 40, 129, 80 };
        GUI_EL_Label button ( &param, &buttonArea, (char*)receiveStatusStr[recvStage], (GUI_Obj *)this);
        button.Draw();
    }

}

void CGuiMenu::initGroupCondCmd( CEndState state )
{
                  titleArea = {  5,   5, 150,  20 };
    MoonsGeometry labelArea = {  5,  21, 150,  51 };
    MoonsGeometry valueArea = {  7,  52, 150,  85 };

    LabelParams param[2] = { GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode };

    param[0].transparent = true;
    param[1].transparent = false;

    param[0].element.align = {alignHCenter, alignVCenter};
    param[1].element.align = {alignHCenter, alignVCenter};

    std::string labelStr, valueStr;
    int offset = 0;

    switch( groupCondCommStage )
    {
    case 0: // use coordinate ?
    {
        labelStr.append(coordinateStr);

        if (useSndCoord)
            valueStr.append("ON");
        else
            valueStr.append("OFF");

        break;
    }
    case 1: // set frequency
    {
        labelStr.append(groupCondCommFreqStr);

        auto frequency = state.listItem.begin();
        valueStr = (*frequency)->inputStr;
        if (valueStr.size() != 0 )
            valueStr.append(" ")\
                    .append(freq_hz);

        break;
    }
    case 2: // group vs. indiv.
    {
        labelStr.append("\0");

        if (sndMode)
            valueStr.append("Group");
        else
            valueStr.append("Indiv");

        break;
    }
    case 3: // set address
    {
        labelStr.append( callTitle[1] );

        auto iter = state.listItem.begin();
        (*iter)++;
        if ( (*iter)->inputStr.size() > 0 )
            valueStr = (*iter)->inputStr;
        else
            valueStr.append("--\0");
        break;
    }
    case 4: // set command
    {
        labelStr.append( callTitle[0] );

        auto iter = state.listItem.begin();
        (*iter)++; (*iter)++;
        if ( (*iter)->inputStr.size() > 0 )
        {
            if ((*iter)->inputStr.size() > 16)
            {
                offset = (*iter)->inputStr.size() - 16;
            }

            for (int i = offset; i < (*iter)->inputStr.size(); i++)
            {
                if ( (i-offset) % 8 == 0 && (i-offset) != 0 )
                {
                    valueStr.push_back('\n');
                    //valueStr.push_back(' ');
                }

                valueStr.push_back( (*iter)->inputStr[ i ] );
            }

            valueStr.push_back('\0');
        }
        else
            valueStr.append("--\0");

        valueArea = {  5,  52, 150,  125 };

        break;
    }
    case 5: // print report
    {
        labelStr.append("\0");

        if (1)
            valueStr.append("OK");
        else
            valueStr.append("ERROR");

        break;
    }
    default:
        break;
    }



    GUI_EL_Window   window ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj*)this );
    GUI_EL_Label    title  ( &titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj*)this );
    GUI_EL_Label    label  ( &param[0],                  &labelArea,  (char*)labelStr.c_str(), (GUI_Obj*)this );
    GUI_EL_TextArea value  ( &param[1],                  &valueArea,  (char*)valueStr.c_str(), (GUI_Obj*)this );

    window.Draw();
    title.Draw();

    if ( groupCondCommStage == 4 && offset > 0)
    {
        auto iter = state.listItem.begin();
        (*iter)++; (*iter)++;
        uint32_t position = 0;
        std::size_t data_size = offset+16;
        position = (uint32_t)data_size;

        MoonsGeometry sliderArea   = { (uint8_t)(windowArea.xe - 4*MARGIN), labelArea.ye, (uint8_t)(windowArea.xe - 2*MARGIN), (uint8_t)(115)};
        SliderParams  sliderParams = {(int32_t)data_size, (int32_t)16, (int32_t)position};
        GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this);
        slider.Draw();
    }

    label.Draw();
    value.Draw();
}
