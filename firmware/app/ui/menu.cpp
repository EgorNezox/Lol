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
    miniArea = {20,20,110,110};
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
    inputTimer.timeout.connect(sigc::mem_fun( this, &CGuiMenu::onInputTimer));
    inputTimer.setInterval(inputInterval);
    inputTimer.setSingleShot(true);
}

CGuiMenu::~CGuiMenu()
{
    if (tx != nullptr)
        delete tx;
}

MoonsGeometry CGuiMenu::getDefaultTitleArea()
{
   MoonsGeometry defaultTitleArea = {(GXT)(windowArea.xs + MARGIN),
                                     (GYT)(windowArea.ys + MARGIN),
                                     (GXT)(windowArea.xe - MARGIN),
                                     (GYT)(windowArea.ye - ( MARGIN + BUTTON_HEIGHT ) )};
   return defaultTitleArea;
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

void CGuiMenu::initCondCommDialog(CEndState state, bool isSynch, bool isWaitingAnswer ) // УК
{
    std::string str, labelStr;
    auto iter = state.listItem.begin();

    char count[5] = {0,0,0,0,0};
    sprintf(count, "%d", qwitCounter);
    std::string counterStr(" ");
    counterStr.append(count);

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
            str.append(useScanMenu[useCmdRetrans]);
        }
        else
        {
            labelStr.append(pressEnter);
            str.append("");
        }

        break;
    }
    case 3: (*iter)++; // ввод адреса ретранслятора    // num 0
    case 2: (*iter)++;// ввод адреса получателя        // num 1
    case 4: // ввод условной команды                   // num 2
    {

        if ((*iter)->inputStr.size() == 0)
            str.append("--");
        else if ((*iter)->inputStr.size() == 1)
        { str.append("-"); }

        str.append((*iter)->inputStr);
        uint8_t num = txCondCmdStage == 2 ? 1 : txCondCmdStage == 3 ? 0 : 2;
        labelStr.append(condCommStr[num]);
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
    	if (isSynch && !isWaitingAnswer && (command_tx30 == 0))
    	{
		    uint8_t percent = calcPercent(virtCounter, 120);
    		char syn[4] = {0,0,0,0};
    		sprintf(syn, "%d", percent);
    	    str.append("\t\t").append(syncWaitingStr).append("\n\t ").append(syn).append(" %");
    	}
    	else
    	{
    		if (isWaitingAnswer)
    			str.append(rxSmsResultStatus[3]);
    		else
    		{
    			uint8_t percent = calcPercent(command_tx30, 30);
				char pac[] = {0,0,0,0};
				sprintf(pac,"%i",percent);
				str.append(pac);
				str.append(" %");
    		}
    	}
        break;
    }
    }

    LabelParams params;
    if (txCondCmdStage == 4 || txCondCmdStage == 3 || txCondCmdStage == 2)
    	params = GUI_EL_TEMP_LabelChannel;
    else
    	params = GUI_EL_TEMP_LabelMode;
    params.element.align = {alignHCenter, alignVCenter};
    params.transparent = true;

    MoonsGeometry localLabelArea = { 7,  5, 120,  39 };
    MoonsGeometry localFieldArea = { 2, 40, 125, 100 };
    MoonsGeometry local1FieldArea = { 7, 90, 120, 120 };

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                               (GUI_Obj *)this);
    GUI_EL_Label  label (&GUI_EL_TEMP_LabelTitle,    &localLabelArea,  (char*)labelStr.c_str(), (GUI_Obj *)this);
    GUI_EL_Label  field (&params,                    &localFieldArea,  (char*)str.c_str(),      (GUI_Obj *)this);
    GUI_EL_Label  field1 (&params,                   &local1FieldArea,  (char*)counterStr.c_str(),      (GUI_Obj *)this);

    window.Draw();
    label.Draw();
    field.Draw();
    if (isWaitingAnswer)
    	field1.Draw();
}

void CGuiMenu::initTwoStateDialog()
{
    int i = 0;
    MoonsGeometry itemArea = {(GXT)(windowArea.xs + 7*MARGIN),
                              (GYT)(windowArea.ys + 17 + i*(MARGIN + BUTTON_HEIGHT)),
                              (GXT)(windowArea.xe - 7*MARGIN),
                              (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                             };

    titleArea = getDefaultTitleArea();

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
    GUI_EL_Label volume(&GUI_EL_TEMP_LabelChannel, &volume_geom,  NULL, (GUI_Obj*)this);

    char s[4]; sprintf(s,"%d",vol);
    std::string str;

    str.append(s);
    str.push_back(proc);
    volume.SetText((char *)str.c_str());
    str.clear();

    titleArea = getDefaultTitleArea();

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    volume.Draw();
}

void CGuiMenu::initAruarmDialog()
{
    MoonsGeometry volume_geom[6];
    volume_geom[0] = {  3,  20,  50,  48 };
    volume_geom[1] = {  3,  53,  50,  81 };
    volume_geom[2] = {  3,  86,  50, 114 };

    volume_geom[3] = { 70,  20, 110,  48 };
    volume_geom[4] = { 70,  53, 110,  81 };
    volume_geom[5] = { 70,  86, 110, 114 };

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
       volume[3+i]->SetText((char*)useScanMenu[aruArmAsuStatus[i]]);

    titleArea = getDefaultTitleArea();

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();

    for (int i = 0; i < 6; i++)
        volume[i]->Draw();

    MoonsGeometry rect = {volume_geom[focus].xs, volume_geom[focus].ys, volume_geom[focus + 3].xe, volume_geom[focus].ye};

    GUI_Painter::SetViewPort(windowArea.xs, windowArea.ys, windowArea.xe, windowArea.ye);
    GUI_Painter::DrawRect(rect, RDM_LINE, CST_DEFAULT);

    for (int i = 0; i < 6; i++)
        delete volume[i];
}

void CGuiMenu::initIncludeDialog()
{
    MoonsGeometry volume_geom  = {  35,  40,  105,  70 };
    GUI_EL_Label volume(&GUI_EL_TEMP_LabelMode, &volume_geom,  NULL, (GUI_Obj*)this);

    volume.SetText((char *)useScanMenu[inclStatus]);

    titleArea = getDefaultTitleArea();

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    volume.Draw();
}

void CGuiMenu::initSuppressDialog()
{
    MoonsGeometry volume_geom  = {  10,  45,  95,  70 };
    GUI_EL_Label volume (&GUI_EL_TEMP_LabelMode, &volume_geom,  NULL, (GUI_Obj*)this);

    char str[3];
    sprintf(str,"%d",inclStatus);
    str[2] = '\0';
    if (atoi(str) == 0)
        volume.SetText((char*)useScanMenu[0]);
    else
        volume.SetText((char*)str);

    titleArea = getDefaultTitleArea();

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    volume.Draw();
}

void CGuiMenu::initGpsCoordinateDialog(char* coord_lat, char* coord_log)
{
    MoonsGeometry volume_geom0  = {  5,  30,  110,  60 };
    MoonsGeometry volume_geom1  = {  5,  60,  110,  90 };

    GUI_EL_Label volume0(&GUI_EL_TEMP_LabelMode, &volume_geom0,  NULL, (GUI_Obj*)this);
    GUI_EL_Label volume1(&GUI_EL_TEMP_LabelMode, &volume_geom1,  NULL, (GUI_Obj*)this);

    if (atoi(coord_lat) == 0)
    {
        coord_lat =  "0000.0000,N";
        coord_log =  "0000.0000,N";
    }

    coord_lat[11] = '\0';
    volume1.SetText(coord_log);
    volume0.SetText(coord_lat);
    volume0.transparent = true;
    volume1.transparent = true;
    // title
    titleArea = getDefaultTitleArea();

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    volume0.Draw();
    volume1.Draw();
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
    titleArea = getDefaultTitleArea();

    titleParams.element.align = {alignHCenter, alignTop};
    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj *)this);

    bool isRepaintItem = false;

    if (isNeedClearWindow){
     window.Draw();
     isNeedClearWindow = false;
     isRepaintItem = true;
    }

    GUI_Painter::DrawText(10,5,titleParams.font,(char*)titleStr.c_str());

    itemParams.label_params.transparent = true;
    itemParams.label_params.element.align = {alignHCenter, alignVCenter};
    itemParams.icon_params.icon = sym_blank;
    int i = 0, j = 0; MoonsGeometry itemArea;

    if (text.size() < 4)
    {
        for (auto &k: text)
        {
            itemArea = {(GXT)(windowArea.xs + MARGIN),
                        (GYT)(windowArea.ys + 17 + i * (MARGIN + BUTTON_HEIGHT)),
                        (GXT)(windowArea.xe - MARGIN),
                        (GYT)(windowArea.ys + 14 + (i+1) * (MARGIN + BUTTON_HEIGHT) )
                       };

            if (i == focusItem)
            {
              GUI_Painter::DrawRect( itemArea, RDM_FILL, CST_INVERSE);
              GUI_Painter::DrawText(itemArea.xs,itemArea.ys,itemParams.label_params.font,(char*)k.c_str(),CST_INVERSE);
            }
            else if (i == oldFocus || isRepaintItem){
              GUI_Painter::DrawRect(itemArea, RDM_FILL, CST_DEFAULT);
              GUI_Painter::DrawRect(itemArea, RDM_LINE, CST_DEFAULT);
              GUI_Painter::DrawText(itemArea.xs,itemArea.ys,itemParams.label_params.font,(char*)k.c_str());
            }

            i++;
        }
    }
    else
    {
        MoonsGeometry sliderArea  = { 118, 25, 125, 110};
        SliderParams  sliderParams = {(int32_t)text.size(), (int32_t)1, (int32_t)focusItem};
        GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this);
        GUI_Painter::DrawRect(120, 30, 123, 100, RDM_FILL); //clear slider
        slider.Draw();

        for (auto &k: text)
        {
            if (j < offset) { ++j; continue;}

            if (i > 4) break;

            itemArea = {(GXT)(windowArea.xs + MARGIN),
                        (GYT)(windowArea.ys + 17 + i * (MARGIN + BUTTON_HEIGHT)),
                        (GXT)(windowArea.xe - 3 * MARGIN),
                        (GYT)(windowArea.ys + 14 + (i+1)*(MARGIN + BUTTON_HEIGHT) )
                       };

            if (focusItem-offset == i)
                itemParams.label_params.color_sch = {GENERAL_BACK_COLOR, GENERAL_TEXT_COLOR};
            else
                itemParams.label_params.color_sch = {GENERAL_TEXT_COLOR, GENERAL_BACK_COLOR};

            GUI_EL_MenuItem item(&itemParams, &itemArea, (char*)k.c_str(), true, (GUI_Obj*)this);
            item.Draw();
            i++;
        }
    }
    itemParams.label_params.color_sch = {GENERAL_TEXT_COLOR, GENERAL_BACK_COLOR};
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
				str->pop_back();
			else if (key == keyBack && str->size() == 0)
			{}

			if ( key > 5 && key < 16)
			{
				if ( str->size() == 0 && key != key0)
					str->push_back(key+42);
				else if ( str->size() > 0 && str->size() < 8 )
					str->push_back(key+42);
			}
			break;
		}
    }
}

void CGuiMenu::initSetParametersDialog(std::string text)
{
	initDialog(text);
}

void CGuiMenu::initSetSpeedDialog(std::string speed)
{
	speed.append(" ").append(speed_bit);
	initDialog(speed);
}

void CGuiMenu::initSetDateOrTimeDialog(std::string text)
{
	initDialog(text);
}

void CGuiMenu::initDialog(std::string text)
{
    MoonsGeometry volume_geom  = {  5,  65,  120,  95 };
    LabelParams label_param[2] = {GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode};

    titleArea = { 5, 10, 120, 35};
    label_param[0].transparent = true;
    label_param[1].transparent = false;

    label_param[0].element.align = { alignHCenter, alignTop };
    label_param[1].element.align = { alignHCenter, alignTop };

    GUI_EL_Window   window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                           (GUI_Obj *)this);
    GUI_EL_Label    title (&label_param[0],            &titleArea,   (char*)titleStr.c_str(), (GUI_Obj *)this);
    GUI_EL_Label    volume(&label_param[1],            &volume_geom, (char*)text.c_str(),     (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    volume.Draw();
}


//--------------------------------------------------------------------

void CGuiMenu::VoiceDialogClearWindow()
{
    if (!toVoiceMail){
        GUI_Painter::SelectViewPort(0);
        GUI_Painter::SetColorScheme(CST_DEFAULT);
        GUI_Painter::SetViewPort(0,0,127,127);
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

    if (voiceMailSource != VMS_CHANNEL)
    {
        if (!isClear)
        {
            cst = CST_DEFAULT;
            str = voiceMailSource == VMS_TX_FILE ? "Tx" : "Rx";
            str.push_back('\0');
            strOld = str;
        }
		else
		{
			cst = CST_INVERSE;
			str = strOld;
		}
    }
    else
    {
		if (!isClear)
		{
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
		else
		{
			str = strOld;
			cst = CST_INVERSE;
		}
    }

    GUI_Painter::DrawText(15,35,voiceFont,(char*)voiceRxTxLabelStr[0],cst);
    if (!isClear)
    	GUI_Painter::DrawText(55,75,voiceMailSource == VMS_CHANNEL ? voiceDigitFont : voiceFont,(char*)str.c_str(),cst);
    else
    {
    	GUI_Painter::DrawText(55,75,voiceFont,(char*)str.c_str(),cst);
        GUI_Painter::DrawText(55,75,voiceDigitFont,(char*)str.c_str(),cst);
    }
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

    if (!isClear)
    {
    	cst = CST_DEFAULT;
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
    	strOld = str;
    }
    else
    {
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

void CGuiMenu::RxSmsStatusPost(int value, bool clear, bool clearAll)
{
   static std::string strTodo;
   ColorSchemeType cst;
   cst = CST_DEFAULT;
   //cst =  clear ? CST_INVERSE: CST_DEFAULT;
   strTodo = "";

	uint8_t percent = calcPercent(value, 82);
	char pac[] = {0,0,0,0};
	sprintf(pac,"%u",percent);
	strTodo.append(pac).append(" %");

   if (clearAll)
   {
	   GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea, (GUI_Obj *)this);
	   window.Draw();
   }
   else
   {
	   GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &miniArea, (GUI_Obj *)this);
	   window.Draw();
   }

   GUI_Painter::DrawText(40,50,voiceFont,(char*)strTodo.c_str(),cst);
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

void CGuiMenu::initEditRnKeyDialog()
{
    MoonsGeometry labelArea   = { 5, 5, 120, 40 };
    MoonsGeometry addrArea    = { 7, 40, 117, 80 };

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
    GUI_EL_Label    label     ( &titleParams,               &labelArea,  (char*)rnKey, (GUI_Obj *)this);
    GUI_EL_TextArea addr      ( &param,                     &addrArea,   (char*)str.c_str(),          (GUI_Obj *)this);

    window.Draw();
    label.Draw();
    addr.Draw();
}

void CGuiMenu::initSheldureDialog(std::vector<std::string>* data, uint8_t sessionCount)
{
    MoonsGeometry labelArea   = { 5, 3, 120, 15 };
    MoonsGeometry length_geom = { 100, 2, 120, 15};

    LabelParams param_length = GUI_EL_TEMP_CommonTextAreaLT;
    param_length.element.align = {alignHCenter, alignTop};

    MenuItemParams param = GUI_EL_TEMP_DefaultMenuItem;
    param.label_params.element.align = {alignHCenter, alignTop};
    param.label_params.transparent = true;
    param.label_params.font = &Tahoma26;

    GUI_EL_Window   window( &GUI_EL_TEMP_WindowGeneral, &windowArea, (GUI_Obj *)this );
    window.Draw();

    Margins margins = {0,0,0,0};
    MoonsGeometry scroll_geom = {0, 20, 127, 120};

    Alignment align = { alignHCenter, alignVCenter};
    GUI_EL_ScrollArea ScrollArea(&scroll_geom, &align, &margins, (GUI_Obj*)this);
    MenuItemParams item_param;
    MoonsGeometry  item_geom;
    item_geom = {(GXT)(0),(GYT)(0),(GXT)(105),(GYT)(48)};

    switch (sheldureStage) {
    case 0: // session list
    {
        GUI_EL_Label    label ( &titleParams,&labelArea,  (char*)Sheldure_label, (GUI_Obj *)this);


        for (uint8_t i = 0; i < data->size(); i++)
        {
            if (sheldureStageFocus[sheldureStage] == i){
                item_param = GUI_EL_TEMP_ActiveMenuItem;
            } else
                item_param = GUI_EL_TEMP_DefaultMenuItem;
            item_param.label_params.font = &Tahoma26;
            item_param.label_params.transparent = true;

            std::string ex = data->at(i);
            GUI_EL_MenuItem *item = new GUI_EL_MenuItem( &item_param, &item_geom, (char*)ex.c_str(), true, (GUI_Obj*)this );
            ScrollArea.addGuiElement(item);
        }

        ScrollArea.setFirstVisElem(firstVisSheldureElem[sheldureStage]);
        ScrollArea.setFocus(sheldureStageFocus[sheldureStage]);


        if (sheldureStageFocus[sheldureStage] < sessionCount){
            length_message.clear();
            char str_len[] = {0,0,0};
            sprintf(str_len,"%d", sheldureStageFocus[sheldureStage]+1);
            length_message.append(str_len);
            length_message.append( "/" );
            sprintf(str_len,"%d",sessionCount);
            length_message.append(str_len);
            GUI_EL_TextArea length (&param_length, &length_geom, (char*)length_message.c_str(), (GUI_Obj *)this);
           length.Draw();
        }
        label.Draw();
        ScrollArea.Draw();
        break;
    }
    case 1: // type
    {
        GUI_EL_Label    label ( &titleParams,&labelArea,  (char*)newSheldure_label, (GUI_Obj *)this);
        item_geom = {(GXT)(0),(GYT)(0),(GXT)(105),(GYT)(32)};
        for (uint8_t subMenu = 0; subMenu < 5; subMenu++)
        {
            if (sheldureStageFocus[sheldureStage] == subMenu){
                item_param = GUI_EL_TEMP_ActiveMenuItem;
            } else
                item_param = GUI_EL_TEMP_DefaultMenuItem;
            item_param.label_params.font = &Tahoma26;
            item_param.label_params.transparent = true;

            GUI_EL_MenuItem *item = new GUI_EL_MenuItem( &item_param, &item_geom, (char*)tmpParsing[subMenu], true, (GUI_Obj*)this );
            ScrollArea.addGuiElement(item);
        }

        ScrollArea.setFirstVisElem(0);
        ScrollArea.setFocus(sheldureStageFocus[sheldureStage]);

        ScrollArea.Draw();
        label.Draw();

        break;
    }
    case 2: // time
    {
        LabelParams label_param[3] = {GUI_EL_TEMP_LabelText, GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode};

        MoonsGeometry timeArea     = { 5, 20, 120, 45 };
        MoonsGeometry volume_geom  = { 5, 50, 120, 100 };

        label_param[0].transparent = true;
        label_param[1].transparent = false;
        label_param[2].transparent = false;

        label_param[0].element.align = { alignHCenter, alignTop };
        label_param[1].element.align = label_param[0].element.align;
        label_param[2].element.align = label_param[0].element.align;

        GUI_EL_Label    title (&label_param[0],            &titleArea,   (char*)newSheldure_label,       (GUI_Obj *)this);
        GUI_EL_Label    label (&label_param[1],            &timeArea,    (char*)dataAndTime[1],          (GUI_Obj *)this);
        GUI_EL_TextArea volume(&label_param[2],            &volume_geom, (char*)sheldureTimeStr.c_str(), (GUI_Obj *)this);

        title.Draw();
        label.Draw();
        volume.Draw();

        break;
    }
    case 3: // freq
    {
        LabelParams label_param[3] = {GUI_EL_TEMP_LabelText, GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode};

        MoonsGeometry freqArea     = { 5, 20, 120, 45 };
        MoonsGeometry valueArea    = { 5, 50, 120, 100 };

        label_param[0].transparent = true;
        label_param[1].transparent = false;
        label_param[2].transparent = false;

        label_param[0].element.align = { alignHCenter, alignTop };
        label_param[1].element.align = label_param[0].element.align;
        label_param[2].element.align = label_param[0].element.align;

        GUI_EL_Label  title ( &label_param[0],  &titleArea,  (char*)editSheldure_label, (GUI_Obj*)this );
        GUI_EL_Label  label ( &label_param[1],  &freqArea,   (char*)groupCondCommFreqStr, (GUI_Obj*)this );
        GUI_EL_Label  value ( &label_param[2],  &valueArea,  (char*)sheldureFreqStr.c_str(), (GUI_Obj*)this );

        title.Draw();
        label.Draw();
        value.Draw();

        break;
    }
    case 4: // edit
    {
      GUI_EL_Label    label ( &titleParams,&labelArea,  (char*)editSheldure_label, (GUI_Obj *)this);

        for (uint8_t i = 0; i < 2; i++)
        {
            if (sheldureStageFocus[sheldureStage] == i){
                item_param = GUI_EL_TEMP_ActiveMenuItem;
            } else
                item_param = GUI_EL_TEMP_DefaultMenuItem;
            item_param.label_params.font = &Tahoma26;
            item_param.label_params.transparent = true;

            const char* edtStr[2] = {editSheldure,delSheldure};
            GUI_EL_MenuItem *item = new GUI_EL_MenuItem( &item_param, &item_geom, (char*)edtStr[i], true, (GUI_Obj*)this );
            ScrollArea.addGuiElement(item);
        }
        ScrollArea.setFirstVisElem(0);
        ScrollArea.setFocus(sheldureStageFocus[sheldureStage]);

        window.Draw();
        label.Draw();
        ScrollArea.Draw();
        break;
    }
    case 5:     // delite
    {
        MoonsGeometry titleArea     = { 0, 15, 125, 70 };
        MoonsGeometry scroll_geom = {0, 70, 127, 127};
        item_geom = {(GXT)(0),(GYT)(0),(GXT)(105),(GYT)(25)};

        LabelParams titleParam = GUI_EL_TEMP_LabelMode;
        titleParam.transparent = false;
        titleParam.element.align = { alignHCenter, alignTop };
        GUI_EL_Label    label ( &titleParams,   &labelArea,  (char*)delSheldure_lable, (GUI_Obj *)this);
        GUI_EL_Label    title ( &titleParam,  &titleArea,  (char*)askDelSheldure, (GUI_Obj*)this );
        GUI_EL_ScrollArea ScrollArea(&scroll_geom, &align, &margins, (GUI_Obj*)this);

          for (uint8_t i = 0; i < 2; i++)
          {
              if (sheldureStageFocus[sheldureStage] == i){
                  item_param = GUI_EL_TEMP_ActiveMenuItem;
              } else
                  item_param = GUI_EL_TEMP_DefaultMenuItem;
              item_param.label_params.font = &Tahoma26;
              item_param.label_params.transparent = true;

              GUI_EL_MenuItem *item = new GUI_EL_MenuItem( &item_param, &item_geom, (char*)yesNo[i], true, (GUI_Obj*)this );
              ScrollArea.addGuiElement(item);
          }
          ScrollArea.setFirstVisElem(0);
          ScrollArea.setFocus(sheldureStageFocus[sheldureStage]);

          window.Draw();
          label.Draw();
          title.Draw();
          ScrollArea.Draw();
          break;
    }
    default:
        break;
    }

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

void CGuiMenu::inputSmsMessage(std::string *field, UI_Key key)
{
    uint8_t keyNum = key-6;

    if ( keyNum >= 0 && keyNum <= 9)
    {
        if (prevKey == key && isInRepeatIntervalInput)
        {
            keyPressCount++;
            if (keyPressCount >= keyCharsCount[keyNum])
                keyPressCount = 0;
            if (field->size() > 0)
                field->pop_back();
        }
        else
            keyPressCount = 0;

        prevKey = key;

        if (field->size() == 100 && keyPressCount == 0)
        	return;
        char ch = (keyChars[keyNum][keyPressCount]);
        field->push_back(ch);
    }
    isInRepeatIntervalInput = true;
    inputTimer.start();
}

void CGuiMenu::inputSmsAddr(std::string *field, UI_Key key)
{
    if ( key > 5 && key < 16 && field->size() < 2 )
    {
        field->push_back((char)(42+key));
        if ( atoi(field->c_str()) > 31 )
         field->clear();
    }
}

void CGuiMenu::initTxSmsDialog(std::string titleStr, std::string fieldStr )
{
    GUI_EL_Window window (&GUI_EL_TEMP_WindowGeneral, &windowArea, (GUI_Obj *)this);
    MoonsGeometry title_geom  = {  5,   5, 110,  20 };
    MoonsGeometry field_geom  = {  7,  40, 110,  100 };
    MoonsGeometry length_geom = { 100,  5,  120,  20};

    LabelParams param[3] = {GUI_EL_TEMP_CommonTextAreaLT, GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelChannel};
    param[0].element.align = {alignHCenter, alignTop};
    param[1].element.align = param[2].element.align = {alignHCenter, alignVCenter};

    LabelParams fieldParam = param[1];

    bool isDrawTitle = true;
    bool isDrawLength = false;
    bool isDrawScroll = false;

    switch(smsTxStage)
    {
    case 3:
        fieldParam = param[2];
    case 2:
        if (fieldStr.size() == 0)
            fieldStr.append("--");
        break;
    case 4:
    {
    	if (fieldStr.size() > 100)
    		fieldStr.resize(100);

        field_geom  = {  5,  18, 125,  124 };
        fieldParam = param[1];
        fieldParam.element.align = {alignLeft, alignTop};

        length_message.clear();
        char str_len[] = {0,0,0,0};
        sprintf(str_len,"%d", fieldStr.size());
        length_message.append(str_len).append( "/100" );

        isDrawLength = true;
        isDrawScroll = true;
        break;
    }
    case 5:
    case 6:
        isDrawTitle = false;
        break;
    }

    GUI_EL_Label title  (&param[0], &title_geom, (char*)titleStr.c_str(), (GUI_Obj *)this);
    GUI_EL_Label length (&param[0], &length_geom, (char*)length_message.c_str(), (GUI_Obj *)this);
    if(smsTxStage == 2)
        fieldParam = param[2];
    GUI_EL_TextArea field  (&fieldParam, &field_geom, (char*)fieldStr.c_str(), (GUI_Obj *)this);
    field.setVisibleScroll(isDrawScroll);
    smsScrollIndex = field.SetScrollIndex(smsScrollIndex);

    window.Draw();
    if (isDrawTitle) title.Draw();
    if (isDrawLength)length.Draw();
    field.Draw();

    if (smsTxStage == 4 && index_store_sms != 0)
    {
    	std::string txrx;
    	if (index_store_sms > 0)
    	{
    		txrx = "Tx ";
    	}
    	if (index_store_sms < 0)
    	{
    		txrx = "Rx ";
    	}

    	char num[] = {0,0,0,0};
    	sprintf(num, "%u", abs(index_store_sms));
    	txrx.append(num);

        MoonsGeometry rxTxLabelArea = { 0, 0, 35, 20 };
        GUI_EL_Label  label (&GUI_EL_TEMP_LabelTitle,    &rxTxLabelArea,  (char*)txrx.c_str(), (GUI_Obj *)this);
        label.Draw();
    }
}

void CGuiMenu::initRxSmsDialog(std::string str, uint8_t stage) // + guc rx (hack)
{
    MoonsGeometry button_geom = { 10, 40, 120, 100 };
    LabelParams param = GUI_EL_TEMP_LabelMode;
    param.element.align = {alignHCenter, alignVCenter};
    param.transparent = (bool)recvStage;

    GUI_EL_Window   window    ( &GUI_EL_TEMP_WindowGeneral, &windowArea, (GUI_Obj *)this);
    GUI_EL_Label    ok_button ( &param, &button_geom, (char*)str.c_str(),(GUI_Obj *)this);

    window.Draw();
    if (stage == 1)
    {
        LabelParams titleParam = GUI_EL_TEMP_CommonTextAreaLT;
        titleParam.element.align = {alignHCenter, alignTop};
        MoonsGeometry title_geom  = {  5, 5, 150,  20 };
        std::string titleString = "CMC";
        GUI_EL_Label title  (&titleParam, &title_geom, (char*)titleString.c_str(), (GUI_Obj *)this);
        title.Draw();
    }
//    if (stage == 10)// guc qwit tx
//    {
//    	char counter[4] = {0,0,0,0};
//    	sprintf(counter, "%d", qwitCounter);
//    	std::string counterStr(counter);
//	    MoonsGeometry local1FieldArea = { 7, 90, 150, 120 };
//	    GUI_EL_Label  field1 (&param, &local1FieldArea,  (char*)counterStr.c_str(),      (GUI_Obj *)this);
//    	field1.Draw();
//    }
    ok_button.Draw();
}

void CGuiMenu::initRxCondCmdDialog(bool isSynch, bool isStart)        // Прием УК
{
    titleArea   = { 5, 5, 120, 20 };

    LabelParams param = GUI_EL_TEMP_LabelMode;
    param.element.align = {alignHCenter, alignVCenter};
    param.transparent = false;
    MoonsGeometry buttonArea  = { 5, 30, 120, 90 };
    MoonsGeometry local1FieldArea = { 7, 90, 120, 120 };

    GUI_EL_Window   window    ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                         (GUI_Obj *)this);
    GUI_EL_Label    title     ( &titleParams,               &titleArea,   (char*)callSubMenu[0], (GUI_Obj *)this);


    std::string str;
    std::string counterStr;

    if (recvStage == 3) // tx qwit
    {
    	str.append(txQwit);

    	char counter[4] = {0,0,0,0};
    	sprintf(counter, "%d", qwitCounter);
    	counterStr.append(counter);
    }
    else
    {
		if (isSynch && !isStart && (recvStage > 0))
		{
    	    uint8_t percent = calcPercent(virtCounter, 120);
			char syn[4] = {0,0,0,0};
			sprintf(syn, "%d", percent);
			if (virtCounter)
				str.append("\t\t").append(syncWaitingStr).append("\n\t ").append(syn).append(" %");
			else
				str.append(receiveStatusStr[1]);
		}
		else
		{
			str = (char*)receiveStatusStr[recvStage];
		}
    }

    GUI_EL_Label button ( &param, &buttonArea, (char*)str.c_str(), (GUI_Obj *)this);
    GUI_EL_Label  field1 (&param, &local1FieldArea,  (char*)counterStr.c_str(),      (GUI_Obj *)this);

    window.Draw();
    if (recvStage != 1 && recvStage != 3)
        title.Draw();
    button.Draw();
    if (recvStage == 3)
    	field1.Draw();
}

void CGuiMenu::initGroupCondCmd( CEndState state, bool isWaitingAnswer)  // ГУК
{
    // 1 - set frequency
    // 2 - group vs. indiv.
    // 3 - set address
    // 4 - set command
    // 5 - print report

    std::string labelStr, valueStr;
    std::string labels[7] = {coordinateStr, groupCondCommFreqStr, "\0", callTitle[1], callTitle[0], "\0", "\0"};
    std::string values[7] = {NoYesGucCoord[useSndCoord], "", GucIndividGroup[sndMode], "", "", StartGucTx, StartCmd};

    if (not isCoordValid)
    {
    	values[0] = notExistStr;
    	useSndCoord = false;
    }

    switch( groupCondCommStage ) {
        case 1:
        {
            auto frequency = state.listItem.begin();
            valueStr = (*frequency)->inputStr;
            if (valueStr.size() != 0 )
                valueStr.append(" ").append(freq_hz);
            break;
        }
        case 3:
        case 4:
        {
            auto iter = state.listItem.begin();
            (*iter)++; if (groupCondCommStage == 4)(*iter)++;
            if ( (*iter)->inputStr.size() > 0 )
                valueStr = (*iter)->inputStr;
            else
                valueStr.append("--\0");
            break;
        }
    }

    labelStr.append(labels[groupCondCommStage]);
    valueStr.append(values[groupCondCommStage]);
    std::string valueStr1;

    if (groupCondCommStage == 6 and not sndMode && isWaitingAnswer)
    {
    	valueStr = rxSmsResultStatus[3];
    	char counter[5] = {0,0,0,0,0};
    	sprintf(counter, "%d", qwitCounter);
    	valueStr1.append("\t ").append(counter);
    }

                  titleArea = {  5,   5, 120,  18 };
    MoonsGeometry labelArea = {  5,  18, 120,  43 };
    MoonsGeometry valueArea = {  5,  40, 120,  105 };
    MoonsGeometry valueArea1 = {  5,  90, 120,  124 };

    LabelParams param[2] = { GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode };

    param[0].transparent = param[1].transparent = true;
    param[0].element.align = param[1].element.align = {alignHCenter, alignVCenter};

    GUI_EL_Window window ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj*)this );
    GUI_EL_Label  title  ( &titleParams,               &titleArea,  (char*)titleStr.c_str(), (GUI_Obj*)this );
    GUI_EL_Label  label  ( &param[0],                  &labelArea,  (char*)labelStr.c_str(), (GUI_Obj*)this );
    GUI_EL_Label  value ( &param[1],&valueArea,  (char*)valueStr.c_str(), (GUI_Obj*)this );
    GUI_EL_Label  value1 ( &param[1],&valueArea1,  (char*)valueStr1.c_str(), (GUI_Obj*)this );

    window.Draw();
    if (groupCondCommStage != 5 && groupCondCommStage != 6)
        title.Draw();
    label.Draw();

    if (groupCondCommStage == 4)
    {
        MoonsGeometry textGeom = {3, 44, 124, 122};
        TextAreaParams textParams = GUI_EL_TEMP_LabelMode;
        textParams.element.align.align_h = alignHCenter;

        GUI_EL_TextArea cmdText(&textParams, &textGeom, (char*)valueStr.c_str(), (GUI_Obj*)this);
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
    if (isWaitingAnswer)
    	value1.Draw();
}

void CGuiMenu::initSelectVoiceModeParameters(bool use)
{
    MoonsGeometry labelArea = {  5,  21, 120,  51 };
    MoonsGeometry valueArea = {  7,  52, 120,  85 };

    LabelParams param[2] = { GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode };

    param[0].transparent = true;
    param[1].transparent = false;

    param[0].element.align = param[1].element.align = {alignHCenter, alignVCenter};

    GUI_EL_Window   window ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                          (GUI_Obj*)this );
    GUI_EL_Label    label  ( &param[0],                  &labelArea,  (char*)setConnParam[3],  (GUI_Obj*)this );
    GUI_EL_TextArea value  ( &param[1],                  &valueArea,  (char*)"\0",             (GUI_Obj*)this );

    value.SetText(mode_txt[!use]);

    window.Draw();
    label.Draw();
    value.Draw();
}

void CGuiMenu::initSelectChEmissTypeParameters(bool use)
{
    MoonsGeometry labelArea = {  5,  21, 120,  51 };
    MoonsGeometry valueArea = {  7,  52, 120,  85 };

    LabelParams param[2] = { GUI_EL_TEMP_LabelMode, GUI_EL_TEMP_LabelMode };

    param[0].transparent = true;
    param[1].transparent = false;

    param[0].element.align = param[1].element.align = {alignHCenter, alignVCenter};

    GUI_EL_Window   window ( &GUI_EL_TEMP_WindowGeneral, &windowArea,                        (GUI_Obj*)this );
    GUI_EL_Label    label  ( &param[0],                  &labelArea, (char*)setConnParam[2], (GUI_Obj*)this );
    GUI_EL_TextArea value  ( &param[1],                  &valueArea, (char*)"\0",            (GUI_Obj*)this );

    value.SetText((char*)ch_em_type_str[!use]);

    window.Draw();
    label.Draw();
    value.Draw();
}
void CGuiMenu::initFailedSms(int stage)
{
	std::string str;
	switch(stage)
    {
        case 0: str.append(sms_quit_fail1); break;
        case 1: str.append(sms_quit_fail2); break;
        case 3: str.append(sms_crc_fail);   break;
    }

	LabelParams params = GUI_EL_TEMP_LabelMode;
	params.element.align = {alignHCenter, alignVCenter};
    MoonsGeometry fieldArea = { 7, 40, 115, 90 };

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
    MoonsGeometry window_geom = {0, 0, 127, 127};
    GUI_EL_Window window (&GUI_EL_TEMP_WindowGeneralBack, &window_geom, (GUI_Obj*)this);

    MoonsGeometry label_geom  = { 2, 4, 117, 16};
    LabelParams   label_params = GUI_EL_TEMP_LabelTitle;

    Margins margins = {0,0,0,0};
    MoonsGeometry scroll_geom = {0, 22, 127, 120};

    Alignment align = { alignHCenter, alignVCenter};
    GUI_EL_ScrollArea ScrollArea(&scroll_geom, &align, &margins, (GUI_Obj*)this);

    MoonsGeometry  item_geom;
    item_geom = {(GXT)(0),(GYT)(0),(GXT)(115),(GYT)(30)};

    MenuItemParams item_param;

    window.Draw();
    const char* titleChar;

    switch(stage)
    {
    case 0: // mode list

        titleChar = files[0];

        for (uint8_t subMenu = 1; subMenu < 5; subMenu++)
        {
            if (filesStageFocus[stage] == subMenu)
                item_param = GUI_EL_TEMP_ActiveMenuItem;
            else
                item_param = GUI_EL_TEMP_DefaultMenuItem;

            item_param.label_params.font = GUI_EL_TEMP_LabelChannel.font;
            item_param.label_params.transparent = true;

            GUI_EL_MenuItem *item = new GUI_EL_MenuItem( &item_param, &item_geom, (char*)tmpParsing[subMenu], true, (GUI_Obj*)this );
            ScrollArea.addGuiElement(item);
        }

        if (filesStageFocus[stage] == 3)
            ScrollArea.setFirstVisElem(1);
        else
            ScrollArea.setFirstVisElem(0);

        ScrollArea.setFocus(filesStageFocus[stage] - 1);

        ScrollArea.Draw();
        break;

    case 1: // Rx / Tx

    	titleChar = tmpParsing[fileType];

        for (uint8_t subMenu = 0; subMenu < 2; subMenu++)
        {
            if (filesStageFocus[stage] == subMenu)
                item_param = GUI_EL_TEMP_ActiveMenuItem;
            else
                item_param = GUI_EL_TEMP_DefaultMenuItem;

            item_param.label_params.font = GUI_EL_TEMP_LabelChannel.font;
            item_param.label_params.transparent = true;

            GUI_EL_MenuItem *item = new GUI_EL_MenuItem( &item_param, &item_geom, (char*)fileRxTx[subMenu], true, (GUI_Obj*)this );
            ScrollArea.addGuiElement(item);
        }

        ScrollArea.setFirstVisElem(firstVisFileElem);

        ScrollArea.setFocus(filesStageFocus[stage]);
        firstVisFileElem = ScrollArea.getFirstVisElem();

        ScrollArea.Draw();
        break;

    case 2:  // file names
    {
    	std::string tmpStr(tmpParsing[fileType]);
    	titleChar = tmpStr.c_str();

    	        if (filesStageFocus[stage] > tFiles[fileType].size() - 1 )
    	            filesStageFocus[stage] = tFiles[fileType].size() - 1;

    	        if (tFiles[fileType].size() > 0)
    	        {
    	        	calcFilesCount();

    	            for (uint8_t file = 0, fileActualNum = 0; file < tFiles[fileType].size(); file++)
    	            {
    	            	std::string fileName = tFiles[fileType].at(file);
    	            	std::string transType(transitionfileType == DataStorage::FS::TFT_RX ? "r" : "t");

    	            	if (file < minTransTypeCount || file >= maxTransTypeCount)
    	            		continue;

    	            	fileActualNum++;

    	            	char num[3] = {0,0,0};
    	            	sprintf(num, "%u", fileActualNum);

    	            	std::string newFileName(fileRxTx[transitionfileType]);
    	            	newFileName.append(num);

    	                if (filesStageFocus[stage] == fileActualNum - 1)
    	                    item_param = GUI_EL_TEMP_ActiveMenuItem;
    	                 else
    	                    item_param = GUI_EL_TEMP_DefaultMenuItem;

    	                item_param.label_params.font = &Tahoma26;
    	                item_param.label_params.transparent = true;

    	                GUI_EL_MenuItem *item = new GUI_EL_MenuItem( &item_param, &item_geom, (char*)newFileName.c_str(), true, (GUI_Obj*)this );
    	                ScrollArea.addGuiElement(item);
    	            }
    	            ScrollArea.setFirstVisElem(firstVisFileElem);

    	            ScrollArea.setFocus(filesStageFocus[stage]);
    	            firstVisFileElem = ScrollArea.getFirstVisElem();
    	            ScrollArea.Draw();
    	        }
    	        break;
    }
    case 3: // file content
    {
        titleChar = tmpParsing[fileType];
        TextAreaParams textArea_Params = GUI_EL_TEMP_LabelMode;
        textArea_Params.element.align.align_h = alignLeft;
        textArea_Params.element.align.align_v = alignTop;
        MoonsGeometry textArea_Geom = {  5,  18, 125,  124 };

        GUI_EL_TextArea textArea(&textArea_Params, &textArea_Geom, fileMessage, (GUI_Obj*)this);
        textArea.setVisibleScroll(true);
        textAreaScrollIndex = textArea.SetScrollIndex(textAreaScrollIndex);
        if (tFiles[fileType].size() > 0)
        	textArea.Draw();
        break;
    }
    }

    GUI_EL_Label title( &label_params, &label_geom, (char*)titleChar, (GUI_Obj *)this);
    title.Draw();
}

void CGuiMenu::initDisplayBrightnessDialog()
{
    MoonsGeometry brightness_geom  = {5,  45,  120,  70 };
    LabelParams brightnessParams = GUI_EL_TEMP_LabelMode;
    brightnessParams.element.align.align_h = alignHCenter;
    GUI_EL_Label brightness(&brightnessParams, &brightness_geom,  (char*)displayBrightnessStr[displayBrightness], (GUI_Obj*)this);

    titleArea = getDefaultTitleArea();

    GUI_EL_Window window(&GUI_EL_TEMP_WindowGeneral, &windowArea,                                   (GUI_Obj *)this);
    GUI_EL_Label  title (&titleParams,               &titleArea,  (char*)displayBrightnessTitleStr, (GUI_Obj *)this);

    window.Draw();
    title.Draw();
    brightness.Draw();
}

void CGuiMenu::onInputTimer()
{
    isInRepeatIntervalInput = false;
}

void CGuiMenu::calcFilesCount()
{
	uint8_t rxCountFiles = storageFs->getTransmitFileTypeCount(fileType, DataStorage::FS::TFT_RX);
	uint8_t txCountFiles = storageFs->getTransmitFileTypeCount(fileType, DataStorage::FS::TFT_TX);

	if (transitionfileType == DataStorage::FS::TFT_TX)
	{
		minTransTypeCount = rxCountFiles;
		maxTransTypeCount = rxCountFiles + txCountFiles;
	}
	else
	{
	    minTransTypeCount = 0;
		maxTransTypeCount = rxCountFiles;
	}
}

// inverse numbers 0->9, 1->8, ... 9->0
uint8_t CGuiMenu::recalcFileFocus(uint8_t focus, DataStorage::FS::FileType f, DataStorage::FS::TransitionFileType t)
{
	uint8_t rxCountFiles = storageFs->getTransmitFileTypeCount(f, DataStorage::FS::TFT_RX);
	uint8_t txCountFiles = storageFs->getTransmitFileTypeCount(f, DataStorage::FS::TFT_TX);

	uint8_t sumFiles = rxCountFiles + txCountFiles;

	uint8_t newFocus = 0;

	if (t == DataStorage::FS::TFT_TX)
	{
		newFocus = rxCountFiles + txCountFiles - focus - 1;
	}
	else
	{
	    newFocus = rxCountFiles - focus - 1;
	}
	return newFocus;
}

uint8_t CGuiMenu::calcPercent(uint8_t a, uint8_t b)
{
	float resf = (float)a / (float)b;
	uint8_t res = (uint8_t)(resf * 100);
	if (res > 100)
		res = 100;
	return res;
}


