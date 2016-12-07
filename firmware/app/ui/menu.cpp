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

    //tFiles[0].push_back("sms0t");
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

void CGuiMenu::initCondCommDialog(CEndState state) // УК
{
    std::string str, labelStr;
    auto iter = state.listItem.begin();

    //[0] - CMD, [1] -retrans , [2] - R_ADDR
    switch (txCondCmdStage)
    {
    case 0: /* Group <-> Individual <-> Ticket */
    {
        labelStr.append(typeCondCmd);
        str.append(smplSubMenu[condCmdModeSelect]);
        break;
    }
    case 1:
    {
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
           //labelStr.append(condCommSendStr);
           str.append(startStr);
        break;
    }
    case 6:
    { //stage send
        char pac[] = {0,0};
        sprintf(pac,"%i",command_tx30);
        str.append(pac);
        str.append("/30");
        break;
    }
    default:
    {break;}
    }

    LabelParams params;
    if (txCondCmdStage == 4 || txCondCmdStage == 3)
    params = GUI_EL_TEMP_LabelChannel;
    else
    params = GUI_EL_TEMP_LabelMode;
    params.element.align = {alignHCenter, alignVCenter};
    params.transparent = true;

    MoonsGeometry localLabelArea = { 7,  5, 150,  39 };
    MoonsGeometry localFieldArea = { 7, 40, 150, 90 };

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

void CGuiMenu::initSuppressDialog()
{
    MoonsGeometry volume_geom  = {  10,  45,  95,  70 };
    GUI_EL_Label *volume = new GUI_EL_Label (&GUI_EL_TEMP_LabelMode, &volume_geom,  NULL, (GUI_Obj*)this);

    char str[3];
    sprintf(str,"%d",inclStatus);
    str[2] = '\0';
    if (atoi(str) == 0) volume->SetText((char*)useScanMenu[1]);
    else
    volume->SetText((char *)str);

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
   // GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

//    window.Draw();
//    title.Draw();

    bool isRepaintItem = false;

    if (isNeedClearWindow){
     window.Draw();
     isNeedClearWindow = false;
     isRepaintItem = true;
    }

    GUI_Painter::DrawText(30,5,titleParams.font,(char*)titleStr.c_str());

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

            if (i == focusItem){
              GUI_Painter::DrawRect( itemArea, RDM_FILL, CST_INVERSE);
              GUI_Painter::DrawText(itemArea.xs,itemArea.ys,itemParams.label_params.font,(char*)k.c_str(),CST_INVERSE);
            }
            else if (i == oldFocus || isRepaintItem){
              GUI_Painter::DrawRect( itemArea, RDM_FILL, CST_DEFAULT);
              GUI_Painter::DrawRect(itemArea, RDM_LINE, CST_DEFAULT);
              GUI_Painter::DrawText(itemArea.xs,itemArea.ys,itemParams.label_params.font,(char*)k.c_str());
            }

            i++;
        }
    }
    else
    {
        MoonsGeometry sliderArea  = { 150, 25, 157, 110};
        SliderParams  sliderParams = {(int32_t)text.size(), (int32_t)1, (int32_t)focusItem};
        GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this);
        GUI_Painter::DrawRect(152, 30, 155, 100, RDM_FILL); //clear slider
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

void CGuiMenu::setFS(DataStorage::FS *fs)
{
    storageFs = fs;
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
	auto item = state.listItem.begin(); item++; item++;
    auto elem = state.listItem.back();
    auto newTime = std::chrono::steady_clock::now();

    auto command = (*item);

    if ( ( newTime - ct ).count() < 900*(1000000) )
    {
            keyPressCount++;
            if ( keyPressCount > 1 )
                keyPressCount = 0;

            if (elem->inputStr.size() > 0)
            elem->inputStr.pop_back();
            elem->inputStr.push_back(ch_key0[keyPressCount]);
            command->inputStr.push_back(ch_key0[keyPressCount]);
        }
    else
    {
        keyPressCount = 0;
        elem->inputStr.push_back(ch_key0[keyPressCount]);
        command->inputStr.push_back(ch_key0[keyPressCount]);
    }



    ct = std::chrono::steady_clock::now();
}

bool CGuiMenu::getIsInRepeatInterval()
{
    auto newTime = std::chrono::steady_clock::now();
    bool isRepeat = false;
    if ( ( newTime - ct ).count() < 900*(1000000) )
    {
        isRepeat = true;
    }
    ct = std::chrono::steady_clock::now();
    return isRepeat;
}

void CGuiMenu::initTxPutOffVoiceDialog(int status)  //  ГП
{
    MoonsGeometry titleArea  = { 7,  6, 147,  20 };
    MoonsGeometry labelArea  = { 7, 21, 147,  35 };
    MoonsGeometry fieldArea  = { 5, 40, 147, 140 };

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
        param = GUI_EL_TEMP_LabelMode;//GUI_EL_TEMP_CommonTextAreaLT;
        param.element.align = {alignLeft,alignTop};
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
        param   = GUI_EL_TEMP_LabelMode;
        param.element.align = {alignLeft, alignTop};
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
        	char ch[100];
        	sprintf(ch, "%3d%%       \n            \n            \n", vmProgress);
        	str.append(ch);
        }

        GUI_EL_Label label( &titleParams, &labelArea, (char*)"",          (GUI_Obj *)this);
        GUI_EL_Label field( &param,       &fieldArea, (char*)str.c_str(), (GUI_Obj *)this);
        if (status != 0) {
        	label.setSkipTextBackgronundFilling(true);
        	field.setSkipTextBackgronundFilling(true);
        }

        label.Draw();
        field.Draw();

        break;
    }
    default:
        break;
    }
}

//--------------------------------------------------------------------

void CGuiMenu::VoiceDialogClearWindow()
{
    if (!toVoiceMail){
        GUI_Painter::SelectViewPort(0);
        GUI_Painter::SetColorScheme(CST_DEFAULT);
        GUI_Painter::SetViewPort(0,0,159,128);
        GUI_Painter::SetMode(DM_NORMAL);

        GUI_Painter::ClearViewPort();

//        GUI_Painter::DrawRect(5, 15, 150, 128, RDM_FILL); // clear label
//        GUI_Painter::DrawRect(45, 5, 110, 15, RDM_FILL); // clear menuitems
//        GUI_Painter::DrawRect(150, 25, 158, 112, RDM_FILL); //clear slider



        toVoiceMail = true;
    }
    GUI_Painter::SetMode(DM_TRANSPARENT);
    GUI_Painter::SelectFont(voiceFont);
}

void CGuiMenu::TxVoiceDialogInitPaint(bool isClear)
{
    GUI_Painter::SelectViewPort(0);
    if (!toVoiceMail){
        if (!isClear)
            GUI_Painter::SetColorScheme(CST_DEFAULT);
        else
            GUI_Painter::SetColorScheme(CST_INVERSE);
    }
}

void CGuiMenu::TxVoiceDialogStatus1(int status, bool isClear)
{
    static std::string str;
    static std::string strOld;
    str = "";
    ColorSchemeType cst;

    if (!isClear){
        cst = CST_DEFAULT;
        if (channalNum.size() == 0)
            str.append("__");
        else if (channalNum.size() == 1){
            str.push_back('_');
            str.append(channalNum);
        }
        else
            str.append(channalNum);

        str.push_back('\0');
        strOld = str;
    }
    else{
        str = strOld;
        cst = CST_INVERSE;
    }


    GUI_Painter::DrawText(15,35,voiceFont,(char*)voiceRxTxLabelStr[0],cst);
    GUI_Painter::DrawText(55,75,voiceDigitFont,(char*)str.c_str(),cst);
}

void CGuiMenu::TxVoiceDialogStatus2(int status, bool isClear)
{
    static int oldStatus;
    ColorSchemeType cst;
    int curStatus;
    if (!isClear){
        cst = CST_DEFAULT;
        curStatus = status;
        oldStatus = status;
    }
    else
    {
        curStatus = oldStatus;
        cst = CST_INVERSE;
    }

    GUI_Painter::DrawText(12,25,voiceFont,(char*)voiceRxTxLabelStr[2],cst);
   // GUI_Painter::DrawLine(0,50,159,50,cst);
    GUI_Painter::DrawText(3,48,voiceFont,(char*)smatrHSStateStr[curStatus],cst);
}

void CGuiMenu::TxVoiceDialogStatus3(int status, bool isClear)
{
    static std::string str;
    static std::string strOld;
    ColorSchemeType cst;
    str = "";

    if (!isClear){
        cst = CST_DEFAULT;
    if (voiceAddr.size() == 0)
        str.append("__");
    else if (voiceAddr.size() == 1){
        str.push_back('_');
        str.append(voiceAddr);
    }
    else
        str.append(voiceAddr);

    str.push_back('\0');
    strOld = str;
    }
    else{
    str = strOld;
    cst = CST_INVERSE;
    }
    GUI_Painter::DrawText(15,27,voiceFont,(char*)voiceRxTxLabelStr[4],cst);
    GUI_Painter::DrawText(50,75,voiceDigitFont,(char*)str.c_str(),cst);
}

void CGuiMenu::TxVoiceDialogStatus4(int status, bool isClear)
{
    ColorSchemeType cst;
    if (!isClear)
        cst = CST_DEFAULT;
    else
        cst = CST_INVERSE;

    GUI_Painter::DrawText(3,35,voiceFont,(char*)startAleTxVoiceMailStr,cst);
}

void CGuiMenu::TxVoiceDialogStatus5(int status, bool isClear )
{
    static std::string str;
    static std::string strOld;
    static std::string strDigit;
    static std::string strDigitOld;
    str = "";
    strDigit = "";

    ColorSchemeType cst;

    if (!isClear){
        cst = CST_DEFAULT;

         str.append(aleStateStr[status]);
        if (status == 13){
            char ch[10];
            sprintf(ch, "%3d", vmProgress);
            strDigit.append(ch).append(" %");
        }
        strOld = str;
        strDigitOld = strDigit;
    }
    else{
        strDigit = strDigitOld;
        str = strOld;
        cst = CST_INVERSE;
}
    if (status != 13)
        GUI_Painter::DrawText(10,25,voiceFont,(char*)str.c_str(),cst);
    GUI_Painter::DrawText(40,50,voiceDigitFont,(char*)strDigit.c_str(),cst);
}

void CGuiMenu::initTxPutOffVoiceDialogTest(int status)
{
    static int voiceStatusOld;
    static int statusOld;
    int voiceStatusCur = putOffVoiceStatus;
    int statusCur = status;
    int paintCount = 1;
    int voiceStatus[2];
    int argStatus[2];

    bool isClear[2];
    //GUI_Painter::ClearViewPort();
    VoiceDialogClearWindow();

    GUI_Painter::DrawText(15,0,voiceFont,(char*)voicePostTitleStr[0],CST_DEFAULT);
   // GUI_Painter::DrawLine(0,27,159,27,CST_DEFAULT);


    if (inVoiceMail){
        paintCount = 2;
        isClear[0] = true;
        isClear[1] = false;
        voiceStatus[0] = voiceStatusOld;
        voiceStatus[1] = voiceStatusCur;
        argStatus[0] = statusOld;
        argStatus[1] = statusCur;
    }
    else{
        voiceStatus[0] = voiceStatusCur;
        argStatus[0] = statusCur;
        isClear[0] = false;
    }

    for (int i = 0; i < paintCount; i++){
        switch (voiceStatus[i])
        {
            case 1: { TxVoiceDialogStatus1(argStatus[i], isClear[i]); break; }
            case 2: { TxVoiceDialogStatus2(argStatus[i], isClear[i]); break; }
            case 3: { TxVoiceDialogStatus3(argStatus[i], isClear[i]); break; }
            case 4: { TxVoiceDialogStatus4(argStatus[i], isClear[i]); break; }
            case 5: { TxVoiceDialogStatus5(argStatus[i], isClear[i]); break; }
            default: { break; }
        }
    }
    voiceStatusOld = voiceStatusCur;
    statusOld = statusCur;

}

//----------------------------------------------------------------------

void CGuiMenu::initRxPutOffVoiceDialogTest(int status)
{
    static int voiceStatusOld;
    static int statusOld;
    int voiceStatusCur = putOffVoiceStatus;
    int statusCur = status;
    int paintCount = 1;
    int voiceStatus[2];
    int argStatus[2];
    bool isClear[2];

    VoiceDialogClearWindow();
    //GUI_Painter::ClearViewPort();

   // GUI_Painter::DrawLine(0,27,159,27,CST_DEFAULT);
    GUI_Painter::DrawText(30,0,voiceFont,(char*)voicePostTitleStr[1],CST_DEFAULT);

    if (inVoiceMail){
        paintCount = 2;
        isClear[0] = true;
        isClear[1] = false;
        voiceStatus[0] = voiceStatusOld;
        voiceStatus[1] = voiceStatusCur;
        argStatus[0] = statusOld;
        argStatus[1] = statusCur;
    }
    else{
        voiceStatus[0] = voiceStatusCur;
        argStatus[0] = statusCur;
        isClear[0] = false;
    }

    for (int i = 0; i < paintCount; i++){
        switch (voiceStatus[i])
        {
            case 1: { RxVoiceDialogStatus1(argStatus[i], isClear[i]); break; }
            case 2: { RxVoiceDialogStatus2(argStatus[i], isClear[i]); break; }
            case 3: { RxVoiceDialogStatus3(argStatus[i], isClear[i]); break; }
            case 4: { RxVoiceDialogStatus4(argStatus[i], isClear[i]); break; }
            case 5: { RxVoiceDialogStatus5(argStatus[i], isClear[i]); break; }
            default: { break; }
        }
    }
    voiceStatusOld = voiceStatusCur;
    statusOld = statusCur;

}

void CGuiMenu::RxVoiceDialogStatus1(int status, bool isClear )
{
    ColorSchemeType cst;

    if (!isClear)
        cst = CST_DEFAULT;
    else
        cst = CST_INVERSE;

     GUI_Painter::DrawText(5,28,voiceFont,(char*)voiceRxStr[0],cst);
}

void CGuiMenu::RxVoiceDialogStatus2(int status, bool isClear )
{
    static std::string str;
    static std::string strOld;
    static std::string strDigit;
    static std::string strDigitOld;
    str = "";
    strDigit = "";

    ColorSchemeType cst;

    if (!isClear){
        cst = CST_DEFAULT;

        str.append(aleStateStr[status]);
        if (status == 9)
        {
            char ch[3];
            sprintf(ch, "%3d", vmProgress);
            strDigit.append(ch).append(" %");
        }
        strOld = str;
        strDigitOld = strDigit;
    }
    else
    {
        strDigit = strDigitOld;
        str = strOld;
        cst = CST_INVERSE;
    }

    if (status == 9)
        GUI_Painter::DrawText(40,50,voiceDigitFont,(char*)strDigit.c_str(),cst);
    else
    	GUI_Painter::DrawText(10,50,voiceFont,(char*)str.c_str(),cst);

}

void CGuiMenu::RxVoiceDialogStatus3(int status, bool isClear )
{
    static std::string strDigit;
    static std::string strDigitOld;
    strDigit = "";
    ColorSchemeType cst;

    if (!isClear){
        cst = CST_DEFAULT;

    if (voiceAddr.size() < 1)
        strDigit.append("--\0");
    else
        strDigit.append(voiceAddr);

    strDigitOld = strDigit;
    }
    else{
        strDigit = strDigitOld;
        cst = CST_INVERSE;
    }

    GUI_Painter::DrawText(10,28,voiceFont,(char*)voiceRxTxLabelStr[5],cst);
    GUI_Painter::DrawText(20,70,voiceFont,(char*)voiceRxStr[1],cst);
    GUI_Painter::DrawText(70,65,voiceDigitFont,(char*)strDigit.c_str(),cst);
    GUI_Painter::DrawText(5,100,voiceFont,(char*)voiceRxStr[2],cst);

}

void CGuiMenu::RxVoiceDialogStatus4(int status, bool isClear )
{
    static std::string strDigit;
    static std::string strDigitOld;
    strDigit = "";
    ColorSchemeType cst;

    if (!isClear){
        cst = CST_DEFAULT;

    if (channalNum.size() == 0)
        strDigit.append("__");
    else if (channalNum.size() == 1)
    {
        strDigit.push_back('_');
        strDigit.append(channalNum);
    }
    else
        strDigit.append(channalNum);

    strDigit.push_back('\0');

    strDigitOld = strDigit;
    }
    else{
        strDigit = strDigitOld;
        cst = CST_INVERSE;
    }

    GUI_Painter::DrawText(5,30,voiceFont,(char*)voiceRxTxLabelStr[1],cst);
    GUI_Painter::DrawText(60,87,voiceDigitFont,(char*)strDigit.c_str(),cst);
}

void CGuiMenu::RxVoiceDialogStatus5(int status, bool isClear )
{

    ColorSchemeType cst;

    if (!isClear)
        cst = CST_DEFAULT;
    else
         cst = CST_INVERSE;

    GUI_Painter::DrawText(0,25,voiceFont,(char*)voiceRxTxLabelStr[3],cst);
    GUI_Painter::DrawText(0,48,voiceFont,(char*)smatrHSStateStr[status],cst);
}

//-----------------------------------------------------------

void CGuiMenu::initRxPutOffVoiceDialog(int status)
{
    MoonsGeometry title_geom  = { 5,  5, 150,  19 };
    MoonsGeometry label_geom  = { 5, 20, 150,  34 };
    MoonsGeometry field_geom  = { 30, 35, 107, 110 };

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
        param.element.align = {alignLeft, alignTop};
        param.transparent = true;

        std::string str; str.append(aleStateStr[status]);
        if (status == 9)
        {
        	char ch[100];
        	sprintf(ch, "%3d%%       \n            \n            \n", vmProgress);
        	str.append(ch);
        }

        GUI_EL_Label label( &label_param, &label_geom, (char*)"", (GUI_Obj *)this);
        GUI_EL_Label field( &param,       &field_geom, (char*)str.c_str(),      (GUI_Obj *)this);
        if (status != 0) {
        	label.setSkipTextBackgronundFilling(true);
        	field.setSkipTextBackgronundFilling(true);
        }

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

void CGuiMenu::initSheldureDialog(int focus, std::vector<std::string> &data)
{
    MoonsGeometry itemArea;
    MoonsGeometry addrArea    = { 17, 5, 140, 70 };
    MoonsGeometry labelArea   = { 7, 5, 140, 70 };

    MenuItemParams param = GUI_EL_TEMP_DefaultMenuItem;
    param.label_params.element.align = {alignHCenter, alignTop};
    param.label_params.transparent = true;
    param.label_params.font = &Tahoma26;

    GUI_EL_Window   window( &GUI_EL_TEMP_WindowGeneral, &windowArea, (GUI_Obj *)this );

    GUI_EL_Label    label ( &titleParams,&labelArea,  (char*)Sheldure_label, (GUI_Obj *)this);

    window.Draw();
    label.Draw();

    if(data.size() == 0)
    {
        LabelParams params;
        params = GUI_EL_TEMP_LabelMode;
        params.element.align = {alignHCenter, alignVCenter};
    	std::string str;
        str.append(NoSheldure);
        MoonsGeometry localFieldArea = { 15, 45, 145, 100 };
        GUI_EL_Label  field (&params, &localFieldArea,  (char*)str.c_str(),      (GUI_Obj *)this);
        field.Draw();
    } else
    {
		for(int i = 0; i < data.size(); i++)
		{
			if (i > 1) break;

			itemArea = {(GXT)(windowArea.xs + 5),
						(GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT+20)),
						(GXT)(windowArea.xe - MARGIN - 15),
						(GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT+20) )
					   };

			std::string ex = data.at(offset+i);
			bool select = (focus - offset == i) ? true : false;
			GUI_EL_MenuItem addr( &param, &itemArea, (char*)ex.c_str() ,false, select,(GUI_Obj *)this);
			addr.Draw();
		}
		MoonsGeometry sliderArea  = { 150, 25, 157, 110};
		SliderParams  sliderParams = {(int32_t)data.size(), (int32_t)1, (int32_t)focus};
		GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this);
		slider.Draw();
    }
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
    GUI_EL_Window   window (&GUI_EL_TEMP_WindowGeneral, &windowArea,         (GUI_Obj *)this);
    MoonsGeometry title_geom = {  5,   5, 150,  20 };
    MoonsGeometry field_geom  = {  7,  40, 147,  60 };
    MoonsGeometry length_geom = { 110,  5,  160,  20};
    MoonsGeometry sliderArea  = { 150, 25, 157, 110};

    LabelParams param[3] = {GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_LabelMode,GUI_EL_TEMP_LabelChannel};
    param[0].element.align = {alignHCenter, alignTop};
    param[1].element.align = {alignHCenter, alignVCenter};
    param[2].element.align = {alignHCenter, alignVCenter};


    length_message.clear();

    switch(smsTxStage)
    {
    case 1:
    {
        GUI_EL_Label    title  (&param[0], &title_geom, (char*)titleStr.c_str(), (GUI_Obj *)this);
        GUI_EL_TextArea field  (&param[1], &field_geom, (char*)fieldStr.c_str(), (GUI_Obj *)this);
        window.Draw();
        title.Draw();
        field.Draw();
        break;
    }
    case 2:
    {
        if (fieldStr.size() == 0)
            fieldStr.append("--");
        else
            fieldStr.append(fieldStr);

        GUI_EL_Label    title  (&param[0], &title_geom, (char*)titleStr.c_str(), (GUI_Obj *)this);
        GUI_EL_TextArea field  (&param[1], &field_geom, (char*)fieldStr.c_str(), (GUI_Obj *)this);
        window.Draw();
        title.Draw();
        field.Draw();

        break;
    }
    case 3:
    {
        if (fieldStr.size() == 0)
            fieldStr.append("--");

        GUI_EL_Label    title  (&param[0], &title_geom, (char*)titleStr.c_str(), (GUI_Obj *)this);
        GUI_EL_TextArea field  (&param[2], &field_geom, (char*)fieldStr.c_str(), (GUI_Obj *)this);
        window.Draw();
        title.Draw();
        field.Draw();

        break;
    }
    case 4:
    {
        int32_t all_line = 1;
        std::string str;
        str.clear();

        for (uint8_t i = 0; i < fieldStr.size(); i++)
        {
            if ( (i%8 == 0) && (i != 0) )
            {
                str.push_back('\n');
                all_line++;                 // кол-во строк
            }

            str.push_back( fieldStr[i] );
        }
        field_geom  = {  7,  20, 158,  120 };
        param[1].element.align = {alignLeft, alignTop};
        //length_message.append(std::to_string(fieldStr.size()));

        int len = fieldStr.size();
        char str_len[] = {0,0,0,0};
        sprintf(str_len,"%d",len);
        length_message.append(str_len);
        length_message.append( "/100" );


        if(all_line > max_line || focus_line > all_line-4)
        {
                focus_line = all_line-4;
        }
        max_line = all_line;
        char* pointer = (char*)str.c_str();
        GUI_EL_Label    title  (&param[0], &title_geom, (char*)titleStr.c_str(), (GUI_Obj *)this);
        SliderParams  sliderParams;
        if(all_line <= 4)
        {
            sliderParams = { (int32_t)1, (int32_t)1, (int32_t)1 };                      // { из какого количества элементов, 1, позиция }
        }
        else
        {
            str = (char*)&pointer[9*focus_line];
            strncpy((char*)str.c_str(),(char*)str.c_str(),(9*focus_line));
            sliderParams = { (int32_t)all_line-3, (int32_t)1, (int32_t)focus_line};      // { из какого количества элементов, 1, позиция с прокруткой }
        }
        GUI_EL_TextArea field  (&param[1], &field_geom, (char*)str.c_str(), (GUI_Obj *)this);
        GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this);
        GUI_EL_TextArea length (&param[0], &length_geom, (char*)length_message.c_str(), (GUI_Obj *)this);
        window.Draw();
        title.Draw();
        field.Draw();
        slider.Draw();
        length.Draw();
        break;
    }
    case 5:
    {
        GUI_EL_TextArea field  (&param[1], &field_geom, (char*)fieldStr.c_str(), (GUI_Obj *)this);
        window.Draw();
        field.Draw();
        break;
    }
    case 6:
    {

        GUI_EL_TextArea field  (&param[1], &field_geom, (char*)fieldStr.c_str(), (GUI_Obj *)this);
        window.Draw();
        field.Draw();
        break;
    }
    default:
    {break;}
    }
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

void CGuiMenu::initRxCondCmdDialog()        // Прием УК
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

void CGuiMenu::initGroupCondCmd( CEndState state )  // ГУК
{
    std::string labelStr, valueStr;

    switch( groupCondCommStage )
    {
    case 0: // use coordinate ?
    {
        labelStr.append(coordinateStr);

        if (useSndCoord)
            valueStr.append(YesGucCoord);
        else
            valueStr.append(NoGucCoord);

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
            valueStr.append(GucGroup);
        else
            valueStr.append(GucIndivid);

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
            valueStr = (*iter)->inputStr;
        else
            valueStr.append("--\0");
        break;
    }
    case 5: // print report
    {
        labelStr.append("\0");

//        if (useCoordinatel)
//            valueStr.append("OK");
//        else
//            valueStr.append("ERROR");

        valueStr.append(StartGucTx);

        break;
    }
    default:
        break;
    }

                  titleArea = {  5,   5, 150,  18 };
    MoonsGeometry labelArea = {  5,  18, 150,  43 };
    MoonsGeometry valueArea = {  5,  52, 150,  85 };

    TextAreaParams textParams = GUI_EL_TEMP_LabelMode;
    textParams.element.align.align_h = alignHCenter;
    MoonsGeometry textGeom = {3, 44, 156, 122};

    LabelParams param[2] = { GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode };

    param[0].transparent = true;
    param[1].transparent = true;

    param[0].element.align = {alignHCenter, alignVCenter};
    param[1].element.align = {alignHCenter, alignVCenter};

    GUI_EL_Window window ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj*)this );
    GUI_EL_Label  title  ( &titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj*)this );
    GUI_EL_Label  label  ( &param[0],                  &labelArea,  (char*)labelStr.c_str(), (GUI_Obj*)this );
    GUI_EL_Label  value ( &param[1],&valueArea,  (char*)valueStr.c_str(), (GUI_Obj*)this );

    GUI_EL_TextArea cmdText(&textParams, &textGeom, (char*)valueStr.c_str(), (GUI_Obj*)this);

    window.Draw();
    title.Draw();
    label.Draw();

    if (groupCondCommStage == 4){
        cmdText.setVisibleScroll(true);
        cmdScrollIndex = cmdText.SetScrollIndex(cmdScrollIndex);
        cmdText.Draw();

        char comCount[] = {0,0,0};
        sprintf(comCount,"%03d/100", cmdCount);
        std::string commandCountStr(comCount);

        GUI_Painter::DrawText(110,7,titleParams.font,(char*)commandCountStr.c_str());
    }
    else
        value.Draw();
}

void CGuiMenu::initSelectVoiceModeParameters(bool use)
{
    MoonsGeometry labelArea = {  5,  21, 150,  51 };
    MoonsGeometry valueArea = {  7,  52, 150,  85 };

    LabelParams param[2] = { GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode };

    param[0].transparent = true;
    param[1].transparent = false;

    param[0].element.align = {alignHCenter, alignVCenter};
    param[1].element.align = {alignHCenter, alignVCenter};

    GUI_EL_Window   window ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj*)this );
    GUI_EL_Label    label  ( &param[0],                  &labelArea,  (char*)setConnParam[3],  (GUI_Obj*)this );
    GUI_EL_TextArea value  ( &param[1],                  &valueArea,  (char*)"\0",             (GUI_Obj*)this );

    if (use)
        value.SetText(mode_txt[0]);
    else
        value.SetText(mode_txt[1]);

    window.Draw();
    label.Draw();
    value.Draw();
}

void CGuiMenu::initSelectChEmissTypeParameters(bool use)
{
    MoonsGeometry labelArea = {  5,  21, 150,  51 };
    MoonsGeometry valueArea = {  7,  52, 150,  85 };

    LabelParams param[2] = { GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode };

    param[0].transparent = true;
    param[1].transparent = false;

    param[0].element.align = {alignHCenter, alignVCenter};
    param[1].element.align = {alignHCenter, alignVCenter};

    GUI_EL_Window   window ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                        (GUI_Obj*)this );
    GUI_EL_Label    label  ( &param[0],                  &labelArea, (char*)setConnParam[2], (GUI_Obj*)this );
    GUI_EL_TextArea value  ( &param[1],                  &valueArea, (char*)"\0",            (GUI_Obj*)this );

    if (use)
        value.SetText((char*)ch_em_type_str[0]);
    else
        value.SetText((char*)ch_em_type_str[1]);

    window.Draw();
    label.Draw();
    value.Draw();
}
void CGuiMenu::initFailedSms(int stage)
{
	std::string str;
	switch(stage)
	{
		case 0:
		{
			str.append(sms_quit_fail1);
			break;
		}
		case 1:
		{
			str.append(sms_quit_fail2);
			break;
		}
		case 3:
		{
			str.append(sms_crc_fail);
			break;
		}
		default:
		{
			break;
		}
	}

	LabelParams params = GUI_EL_TEMP_LabelMode;
	params.element.align = {alignHCenter, alignVCenter};
	MoonsGeometry fieldArea = { 7, 40, 150, 90 };

	GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                               (GUI_Obj *)this);
	GUI_EL_Label  field (&params,                    &fieldArea,  (char*)str.c_str(),      (GUI_Obj *)this);

	window.Draw();
	field.Draw();

}

void CGuiMenu::TxCondCmdPackage(int value)  // Передача УК  пакеты
{
   command_tx30 = value;
}

void CGuiMenu::initFileManagerDialog(uint8_t stage)
{
    MoonsGeometry window_geom = {0, 0, 159, 127};
    GUI_EL_Window window (&GUI_EL_TEMP_WindowGeneralBack, &window_geom, (GUI_Obj*)this);

    MoonsGeometry label_geom  = { 2, 4, 157, 16};
    LabelParams   label_params = GUI_EL_TEMP_LabelTitle;

    Margins margins = {0,0,0,0};
    MoonsGeometry scroll_geom = {0, 22, 159, 120};

    Alignment align = { alignHCenter, alignVCenter};
    GUI_EL_ScrollArea ScrollArea(&scroll_geom, &align, &margins, (GUI_Obj*)this);

    MoonsGeometry  item_geom;
    item_geom = {(GXT)(0),(GYT)(0),(GXT)(145),(GYT)(30)};

    MenuItemParams item_param;

   // item_param.label_params = GUI_EL_TEMP_LabelChannel;
//    item_param.label_params.element.align = {alignHCenter, alignVCenter};
//    item_param.label_params.transparent = true;
//    item_param.icon_params.icon = sym_blank;

    window.Draw();
    const char* titleChar;

    switch(stage){
    case 0:

        titleChar = files[0];


        for (uint8_t subMenu = 0; subMenu < 4; subMenu++)
        {
            if (filesStageFocus[stage] == subMenu){
                item_param = GUI_EL_TEMP_ActiveMenuItem;
            } else
                item_param = GUI_EL_TEMP_DefaultMenuItem;
            item_param.label_params.font = &Consolas25x35;
            item_param.label_params.transparent = true;

            GUI_EL_MenuItem *item = new GUI_EL_MenuItem( &item_param, &item_geom, (char*)reciveSubMenu[subMenu + 1], true, true, (GUI_Obj*)this );
            ScrollArea.addGuiElement(item);
        }

        if (filesStageFocus[stage] == 3)
          ScrollArea.setFirstVisElem(1);
        else
            ScrollArea.setFirstVisElem(0);
        ScrollArea.setFocus(filesStageFocus[stage]);
        ScrollArea.Draw();
        break;

    case 1:

        titleChar = reciveSubMenu[fileType + 1];

        if (filesStageFocus[stage] > tFiles[fileType].size() - 1 )
            filesStageFocus[stage] = tFiles[fileType].size() - 1;

        if (tFiles[fileType].size() > 0){
            for (uint8_t file = 0; file < tFiles[fileType].size(); file++)
            {
                std::string fileName = tFiles[fileType].at(file);

                if (filesStageFocus[stage] == file){
                    item_param = GUI_EL_TEMP_ActiveMenuItem;
                } else
                    item_param = GUI_EL_TEMP_DefaultMenuItem;
                item_param.label_params.font = &Tahoma26;
                item_param.label_params.transparent = true;

                GUI_EL_MenuItem *item = new GUI_EL_MenuItem( &item_param, &item_geom, (char*)fileName.c_str(), true, true, (GUI_Obj*)this );
                ScrollArea.addGuiElement(item);
            }
            ScrollArea.setFirstVisElem(firstVisFileElem);

            ScrollArea.setFocus(filesStageFocus[stage]);
            firstVisFileElem = ScrollArea.getFirstVisElem();
            ScrollArea.Draw();
        }
        break;

    case 2:

        titleChar = reciveSubMenu[fileType + 1];
        TextAreaParams textArea_Params = GUI_EL_TEMP_LabelMode;
        textArea_Params.element.align.align_h = alignLeft;
        textArea_Params.element.align.align_v = alignTop;
        MoonsGeometry textArea_Geom = {2, 30, 157, 126};

        GUI_EL_TextArea textArea(&textArea_Params, &textArea_Geom, fileMessage, (GUI_Obj*)this);
        textAreaScrollIndex = textArea.SetScrollIndex(textAreaScrollIndex);
        textArea.Draw();
        break;
    }
    GUI_EL_Label title( &label_params, &label_geom, (char*)titleChar, (GUI_Obj *)this);
    title.Draw();
}

void CGuiMenu::initDisplayBrightnessDialog()
{
    MoonsGeometry brightness_geom  = {5,  45,  160,  70 };
    LabelParams brightnessParams = GUI_EL_TEMP_LabelMode;
    brightnessParams.element.align.align_h = alignHCenter;
    GUI_EL_Label brightness(&brightnessParams, &brightness_geom,  (char*)displayBrightnessStr[displayBrightness], (GUI_Obj*)this);

    titleArea = {(GXT)(windowArea.xs + MARGIN),
                 (GYT)(windowArea.ys + MARGIN),
                 (GXT)(windowArea.xe - MARGIN),
                 (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )
                };

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)displayBrightnessTitleStr, (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    brightness.Draw();
}
