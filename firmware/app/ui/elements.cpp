/**
  ******************************************************************************
  * @file     elements.cpp
  * @author  Egor Dudyak, PMR dept. software team, ONIIP, PJSC
  * @date    08 окт. 2013 г.
  * @brief   Реализация классов GUI_Element и его наследников
  ******************************************************************************
  */

//----------INCLUDES-----------

#include <string.h>
#include <stdio.h>
#include "elements.h"
#include "all_sym_indicators.h"
#include <stdio.h>


//----------DEFINES------------

#define VP_COMMON	0

//----------TYPES--------------

//----------GLOBAL_VARS--------

static Margins zero_margins={0,0,0,0};
static Alignment no_align={alignLeft, alignTop};
static Alignment bottom_align={alignLeft, alignBottom};

//----------PROTOTYPES---------

//----------CODE---------------


void convertStrToHackEncoding(uint8_t* text, uint16_t size, PGFONT font)
{
#if memory_hack
    // recalc usual encoding to reduced (hacked) encoding for memory saving
        if (font == (PGFONT)&HackConsolas25x35)
        {
            for (uint16_t i = 0; i < size; i++){
                uint8_t sym = text[i];
                if ( sym >= 0xC0 ) // letter
                    text[i] = sym - 145;
                else if (sym >= 0x30 && sym <= 0x39) // digits
                    text[i] = sym - 11;
                else
                    switch(sym){
                        case 0x25: text[i] = 0x22; break; //"%"
                        case 0x2D: text[i] = 0x23; break; //"-"
                        case 0x2E: text[i] = 0x24; break; //"."
                        case 0x5F: text[i] = 0x20; break; //"_" to " "
                    }
            }
        }
#endif
}

//+++++++++++++Element+++++++++++++++++++++

GUI_Element::GUI_Element(MoonsGeometry *geom, Alignment *align, Margins *margins, GUI_Obj *parent_obj){
	this->parent_obj=parent_obj;
	this->align=*align;
	this->geom=*geom;
	this->el_geom.xs=geom->xs+parent_obj->area.xs;
	this->el_geom.ys=geom->ys+parent_obj->area.ys;
	this->el_geom.xe=geom->xe+parent_obj->area.xs;
	this->el_geom.ye=geom->ye+parent_obj->area.ys;
	this->margins=*margins;
	content={0,0,0,0};
}

//-----------------------------

GUI_Element::~GUI_Element(){ }

//-----------------------------

void GUI_Element::PrepareContent(){
	CalcContentGeom();
	AlignContent();
}

//-----------------------------

void GUI_Element::PrepareViewport(){
	gselvp(VP_COMMON);
	gsetvp(el_geom.xs,el_geom.ys,el_geom.xe, el_geom.ye);
}

//-----------------------------

MoonsGeometry GUI_Element::GetContentGeomOnElem(){
	MoonsGeometry abs_content;
	abs_content.xs=content.x+geom.xs;
	abs_content.ys=content.y+geom.ys;
	abs_content.xe=abs_content.xs+content.W-1;
	abs_content.ye=abs_content.ys+content.H-1;
	return abs_content;
}

//-----------------------------

void GUI_Element::AlignContent(){	//В зависимости от типа выравнивания, вычисляем дополнительное смещение онтента внутри элемента
	content.x=margins.Left;
	content.y=margins.Top;
	switch(align.align_h){
		case alignLeft:
			break;
		case alignHCenter:
			content.x+=(GEOM_W(el_geom)- margins.Right-margins.Left-content.W)/2;
			break;
		case alignRight:
			content.x+=(GEOM_W(el_geom)- margins.Right-margins.Left-content.W);
			break;
		default:
			break;
	}

	switch(align.align_v){
		case alignTop:
			break;
		case alignVCenter:
			content.y+=(GEOM_H(el_geom)- margins.Top-margins.Bottom-content.H)/2;
			break;
		case alignBottom:
			content.y+=(GEOM_H(el_geom)-margins.Top-margins.Bottom-content.H);
			break;
		default:
			break;
	}
}



//+++++++++++++Label+++++++++++++++++++++

GUI_EL_Label::GUI_EL_Label(LabelParams *params, MoonsGeometry *geom, char *text, GUI_Obj *parent_obj):GUI_Element(geom, &params->element.align, &params->element.margins, parent_obj){
    skip_text_bg_filling = false;
    font = params->font;
    transparent = params->transparent;
    transparent =true;
    color_sch = params->color_sch;
	SetText(text);
	if(text!=0)PrepareContent();
}

//-----------------------------

void GUI_EL_Label::setSkipTextBackgronundFilling(bool enabled){
    skip_text_bg_filling = enabled;
}

//-----------------------------

void GUI_EL_Label::SetText(char *text){
    if(text!=0)
    {
    	this->text = std::string(text);
        //this->text.append( text );//, sizeof(this->text));

        convertStrToHackEncoding((uint8_t*)&this->text[0], this->text.size(), this->font);
	}
    else
    {
        this->text.clear();
    }
}

void GUI_EL_Label::SetParams(LabelParams *params)
{
    font = params->font;
    transparent = params->transparent;
    color_sch = params->color_sch;
}

//-----------------------------

void GUI_EL_Label::Draw(){
	if(text.c_str()!=0){
		PrepareContent();
		PrepareViewport();
		gselfont(font);
		gsetcolorb(color_sch.background);
		gsetcolorf(color_sch.foreground);
		SGUCHAR skipflags;
		if(!transparent){
			gsetmode(COMMON_ELEMENT_VP_MODE);
            skipflags = GLINE;
		}
		else {
            gsetmode(COMMON_ELEMENT_VP_MODE | GTRANSPERANT);
			skipflags = GFILL;
		}

        if (!skip_text_bg_filling)
            groundrect(0,0,GEOM_W(el_geom)-1,GEOM_H(el_geom)-1,0,skipflags);
        else
            gsetmode(COMMON_ELEMENT_VP_MODE);
        gsetpos(content.x, CONTENT_YE(content));
        gputs(text.c_str());
		if(transparent){
			gsetmode(COMMON_ELEMENT_VP_MODE);
		}
	}
}

//-----------------------------

void GUI_EL_Label::CalcContentGeom(){
	int32_t i = 0, size=(text.size());
	content.W=0;
	content.H=0;
	gselfont(font);
	for (i = 0; i < size; i++)		//Вычисляем ширину и высоту
	{
		content.W += ggetsymw(text[i]);
	}
	content.H=ggetfh();

}

void GUI_EL_Label::SetInputFocus(bool isFocus)
{
}

//+++++++++++++TextArea+++++++++++++++++++++

GUI_EL_TextArea::GUI_EL_TextArea(TextAreaParams *params, MoonsGeometry *geom, char *text, GUI_Obj *parent_obj):GUI_Element(geom, &params->element.align, &params->element.margins, parent_obj){
 this->params= *params;
 lines_count=0;
 SetText(text);
}

GUI_EL_TextArea::GUI_EL_TextArea(TextAreaParams *params, MoonsGeometry *geom, std::vector<uint8_t> *data, GUI_Obj *parent_obj):GUI_Element(geom, &params->element.align, &params->element.margins, parent_obj){
    this->params = *params;
    lines_count = 0;
    isData = true;
    this->data = data;
    CalcContentGeom();
}

GUI_EL_TextArea::~GUI_EL_TextArea(){
    if (!isData && (text > 0) && strlen(text)>0)
        delete []text;
}

//-----------------------------

void GUI_EL_TextArea::SetText(char *text){
    if(text != NULL){
        uint16_t len = strlen((const char*)text);
        if (len > 0){
            this->text = new char[len + 1];
            strncpy(this->text, text, len + 1);
        }
        CalcContentGeom();
    }
}

void GUI_EL_TextArea::copyStrFromData(char *dest, uint32_t index, uint32_t count)
{
 if (isData){
    for (uint16_t i = 0; i < count; i++)
      dest[i] = (char)data->at(index + i);
 }
 else{
     strncpy(dest, &text[index], count);
 }
}

char GUI_EL_TextArea::getChar(uint32_t index)
{
    if (isData)
        if (index != data->size())
            return (char)(data->at(index));
        else
            return 0;
    else
        return text[index];
}

uint32_t GUI_EL_TextArea::getDataSize()
{
    if (isData){
       return data->size();
    }
    else if(text > 0)
       return strlen(text);
    else
       return 0;
}

//-----------------------------

void GUI_EL_TextArea::Draw(){
    if((text != 0 && !isData) || (data != 0 && isData)){
        uint32_t i = 0, j = 0, k = 0, str_width = 0, last_space = 0, sym_to_cp = 0;
        MoonsGeometry local_content, line_geom;
        char line_str[30];

        LabelParams label_params = params;
        label_params.element.align.align_v = alignTop;
        label_params.element.margins = {0,0,0,0};

        PrepareContent();
        PrepareViewport();

        local_content = GetContentGeomOnElem();

        line_geom = local_content;
        line_geom.ye = line_geom.ys + line_height-1;

        uint32_t curVisLine = 0;

        for(i = 0; i < lines_count; ++i, ++curVisLine)
        {
            for(j = 0, str_width = 0, last_space = 0 ; ; ++j, ++k){
                if(getChar(k)== '\n' || getChar(k) == 0){
                    copyStrFromData((char*)&line_str, k-j, j);
                    line_str[j] = 0;
                    ++k;
                    break;
                }
                else{
                    if(getChar(k) == ' '){
                        last_space = k;
                    }
                    str_width += ggetsymw(getChar(k));
                    if(str_width > GEOM_W(el_geom)){
                        if(last_space == 0 || getChar(k) == ' '){
                            sym_to_cp = j;
                            copyStrFromData((char*)&line_str, k-j, sym_to_cp);
                            line_str[sym_to_cp] = 0;
                            //j--;
                            //++k;
                            break;
                        }
                        else{
                            sym_to_cp = j - (k - last_space);
                            copyStrFromData((char*)&line_str, k-j, sym_to_cp);
                            line_str[sym_to_cp] = 0;
                            k = last_space + 1;
                            break;
                        }

                    }
                }
            }
            if ((curVisLine >= visLineBegin) && (curVisLine <= visLineBegin + visLinesCount) ){
                GUI_EL_Label text_line(&label_params, &line_geom, line_str, parent_obj);
                text_line.Draw();
                line_geom.ys += line_height;
                line_geom.ye += line_height;
            }
        }
        if (isScroll)
  //      {
        //if (direction == VDir)
        {
            MoonsGeometry sliderArea  = { (GXT)el_geom.xe, (GYT)el_geom.ys , (GXT)(el_geom.xe + 7), (GYT) el_geom.ye};
            SliderParams  sliderParams = {lines_count - visLinesCount + 1, (int32_t)1, visLineBegin};
            GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this);
            slider.Draw();
        }
        //else
//        {
//                MoonsGeometry sliderArea  = { 15, 10, 150, 20};
//                SliderParams  sliderParams = {lines_count - visLinesCount + 1, (int32_t)1, visLineBegin};
//                GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this);
//                slider.SetDirection(HDir);
//                slider.Draw();
//        }
    }
}

//-----------------------------

void GUI_EL_TextArea::CalcContentGeom(){
    int32_t lf_count=0, max_str_width=0, str_width=0, last_space=0, last_str_with=0;

    int32_t size = getDataSize();

    content.W = GEOM_W(el_geom);
    content.H = GEOM_H(el_geom);

    allContent.W = 0;
    allContent.H = 0;

    if (size > 0){
    gselfont(params.font);

    for( uint32_t i = 0; i <= size; ++i){ //	подсчет количества строк
        if(getChar(i) == ' '){
            last_space = i;
            last_str_with = str_width;
        }
        if(getChar(i) == '\n' || getChar(i) == 0){
            ++lf_count;
            if(str_width > max_str_width){
                max_str_width = str_width;
            }
            str_width =0;
        }
        else {
            str_width += ggetsymw(getChar(i));
            if(str_width > GEOM_W(el_geom)){		//если строка длиннее чем область элемента
                ++lf_count;						//cчитаем перенос строки
                if(last_space == 0 || getChar(i) == ' '){
                    str_width -= ggetsymw(getChar(i));	//и отменяем сложение длины этого символа
                    i--;
                }
                else{
                    str_width=last_str_with;
                    i=last_space;
                }

                if(str_width>max_str_width){
                    max_str_width=str_width;
                }
                str_width=0;
                last_space=0;
                last_str_with=0;
            }
        }
    }
    }
    lines_count = lf_count;
    line_height = ggetfh();
    allContent.H = ggetfh()*lines_count;
    allContent.W = max_str_width;

    if (allContent.H > GEOM_H(el_geom) && !isScroll && isVisibleScroll)
    {
        el_geom.xe -= 10;
        isScroll = true;
        CalcContentGeom();
    }
    visLinesCount = (geom.ye - geom.ys) / line_height ;
}

void GUI_EL_TextArea::ScrollUp(){
   if (visLineBegin > 0){
       --visLineBegin;
       Draw();
   }
}

void GUI_EL_TextArea::ScrollDown(){
    if (visLineBegin < lines_count - visLinesCount){
        ++visLineBegin;
        Draw();
    }
}

uint32_t GUI_EL_TextArea::GetScrollIndex(){
    return visLineBegin;
}

uint32_t GUI_EL_TextArea::SetScrollIndex(uint32_t index){
    uint32_t oldIndex;

    if (isScroll){
        if (index <= 0 ){
            oldIndex = 0;
        }
        else if (index >= lines_count - visLinesCount ){
            oldIndex = lines_count - visLinesCount;
        }
        else
            oldIndex = index;

        if (visLineBegin != oldIndex){
            visLineBegin = oldIndex;
        }
    }
    return visLineBegin;
}

void GUI_EL_TextArea::SetInputFocus(bool isFocus)
{

}

void GUI_EL_TextArea::setVisibleScroll(bool isVisible)
{
    isVisibleScroll = isVisible;
    PrepareContent();
}



//+++++++++++++Icon+++++++++++++++++++++

GUI_EL_Icon::GUI_EL_Icon(ElementParams *params, MoonsGeometry *geom, PGSYMBOL icon, GUI_Obj *parent_obj):GUI_Element(geom,  &params->align, &params->margins, parent_obj){
	this->icon=icon;
	PrepareContent();
}

//-----------------------------

void GUI_EL_Icon::CalcContentGeom(){
	content.H=gsymh(icon);
	content.W=gsymw(icon);
}

//-----------------------------

void GUI_EL_Icon::Draw(){
	PrepareViewport();
	gputsym(content.x,content.y, icon);
}

void GUI_EL_Icon::SetInputFocus(bool isFocus)
{
}

//-----------------------------



//+++++++++++++Battery+++++++++++++++++++++

#define BATTERY_HIGH_THRESHOLD	60
#define BATTERY_MID_THRESHOLD	30
#define BATTERY_W	14
#define BATTERY_H	22

GUI_EL_Battery::GUI_EL_Battery(ElementParams *params, int charge, MoonsGeometry *geom, GUI_Obj *parent_obj):GUI_Element(geom, &params->align, &params->margins, parent_obj){
	this->charge=charge;
	PrepareContent();
}

//-----------------------------

void GUI_EL_Battery::Draw(){
	PrepareViewport();
	gsetcolorb(GENERAL_BACK_COLOR);
	gsetcolorf(GENERAL_FORE_COLOR);
	/*
	int16_t shpenek_w=content.W/2;		//"тяжелый" код для отрисовки шпенька батарейки, нужен для отрисовки батареек произвольного размера.
										//я написал его на всякий случай, подозреваю что он не пригодится
	if((shpenek_w%2)^(content.W%2)) --shpenek_w;	//хакерская штука для определния длины контакта
	GXT shpenek_x=content.x+(content.W-shpenek_w)/2;
	groundrect(shpenek_x, content.y, shpenek_x+shpenek_w-1,content.y,0,0);	//рисуем шпенек(контакт) батарейки
	*/
	groundrect(content.x+4, content.y, CONTENT_XE(content)-4,content.y,0,0);	//рисуем шпенек(контакт) батарейки
	groundrect(content.x, content.y+1, CONTENT_XE(content), CONTENT_YE(content), 0, GFRAME);	//рисуем корпус батарейки

	if(charge>BATTERY_HIGH_THRESHOLD){	//определяем цвет заряда батарейки
		gsetcolorb(BATTERY_HIGH_COLOR);
	}
	else{
		if(charge>BATTERY_MID_THRESHOLD){
			gsetcolorb(BATTERY_MID_COLOR);
		}
		else{
			gsetcolorb(BATTERY_LOW_COLOR);
		}
	}
	GYT battery_column_h=(GYT)(((int32_t)(content.H-3)*(int32_t)charge)/(int32_t)100);
	if(battery_column_h>0){
		groundrect(content.x+1, CONTENT_YE(content)-battery_column_h, CONTENT_XE(content)-1, CONTENT_YE(content)-1, 0, GFILL);//рисуем заряд батарейки
	}
}

//-----------------------------

void GUI_EL_Battery::CalcContentGeom(){
	content.H=BATTERY_H;
	content.W=BATTERY_W;
}

void GUI_EL_Battery::SetInputFocus(bool isFocus)
{
}

//+++++++++++++VolumeTuner+++++++++++++++++++++

GUI_EL_VolumeTuner::GUI_EL_VolumeTuner(VolumeTunerParams* params, MoonsGeometry* geom, GUI_Obj* parent_obj, uint8_t level) :
		GUI_Element(geom, &params->el_params.align, &params->el_params.margins, parent_obj){
	this->level = level;
	this->bar_count=params->bar_count;
	this->bar_interval=params->bar_interval;
	PrepareContent();
}

//-----------------------------

void GUI_EL_VolumeTuner::setLevel(uint8_t level){
	this->level = level;
}

//-----------------------------

void GUI_EL_VolumeTuner::Draw(){
	int i=0;
	GYT h_step,y_pos;
	GXT w_step, bar_w,x_pos;
	SGUCHAR flag=0;
	PrepareContent();
	PrepareViewport();

	h_step=content.H/bar_count;
	bar_w=(GXT)(((int)content.W-((int)bar_interval*(bar_count-1)))/bar_count);
	w_step=bar_w+bar_interval;
	gsetcolorb(GENERAL_FORE_COLOR);
	gsetcolorf(GENERAL_FORE_COLOR);

	for(i=0,x_pos=content.x,y_pos=CONTENT_YE(content)-h_step;i<bar_count;++i,x_pos+=w_step,y_pos-=h_step){
		if(i<(level/bar_count)){
			flag=GFILL;
			gsetcolorb(GENERAL_FORE_COLOR);
		}
		else{
			flag=GFRAME;
			gsetcolorb(GENERAL_BACK_COLOR);
		}
		groundrect(x_pos,y_pos,x_pos+bar_w-1,CONTENT_YE(content),0,flag);
	}
	gsetcolorb(GENERAL_BACK_COLOR);
}

//-----------------------------

void GUI_EL_VolumeTuner::CalcContentGeom(){
	content.H = GEOM_H(el_geom);
	content.W = GEOM_W(el_geom);
}

void GUI_EL_VolumeTuner::SetInputFocus(bool isFocus)
{
}


//+++++++++++++Spin Box+++++++++++++++++++++

#define SPBOX_FONT	&Lucida_Console_8x12
#define SPBOX_UP	sym_arrow_up
#define SPBOX_DOWN	sym_arrow_down

GUI_EL_SpinBox::GUI_EL_SpinBox(MoonsGeometry* geom, SpBoxParams *spbox_params, SpBoxSettings *spbox_settings, GUI_Obj* parent_obj):
			GUI_Element(geom,&no_align, &zero_margins, parent_obj){
	this->active=0;
	this->value=spbox_settings->value;
	this->min=spbox_settings->min;
	this->max=spbox_settings->max;
	this->step=spbox_settings->step;
	this->spbox_len=spbox_settings->spbox_len;
	this->cyclic=spbox_settings->cyclic;
	this->ValueToStr=spbox_settings->ValueToStr;
	this->lab_params=&GUI_EL_TEMP_LabelSpBoxInactive;
	this->up_arrow=spbox_params->up_arrow;
	this->down_arrow=spbox_params->down_arrow;
	CalcContentGeom();
}

void GUI_EL_SpinBox::Draw(){
    //(*ValueToStr)(value,str);
    sprintf(str, "%d", value);
	MoonsGeometry up_arr_geom={geom.xs,geom.ys,geom.xe,(GYT)(geom.ys+(GEOM_H(geom)-label_h)/2-1)};
	MoonsGeometry down_arr_geom={geom.xs,(GYT)(geom.ye-(GEOM_H(geom)-label_h)/2+1),geom.xe,geom.ye};
	MoonsGeometry label_geom={geom.xs,(GYT)(geom.ys+(GEOM_H(geom)-label_h)/2),geom.xe,0};
	label_geom.ye=label_geom.ys+label_h-1;

	if(active){
		lab_params=&GUI_EL_TEMP_LabelSpBoxActive;
		GUI_EL_Icon up_arr_icon(&GUI_EL_TEMP_IconSpBoxUp,&up_arr_geom,up_arrow,parent_obj);
		GUI_EL_Icon down_arr_icon(&GUI_EL_TEMP_IconSpBoxDown,&down_arr_geom,down_arrow,parent_obj);
		up_arr_icon.Draw();
		down_arr_icon.Draw();
	}
	else{
		lab_params=&GUI_EL_TEMP_LabelSpBoxInactive;
	}
	GUI_EL_Label label(lab_params, &label_geom , str, parent_obj);
	label.Draw();

}

void GUI_EL_SpinBox::SetActiveness(bool active){
	this->active=active;
}

void GUI_EL_SpinBox::Inc(){
	if(!cyclic){
		if(value<max){
			value+=step;
		}
		if(value>max){
			value=max;
		}
	}
	else{
		if(value<=max){
			value+=step;
		}
		if(value>max){
			value=min;
		}
	}
}

void GUI_EL_SpinBox::Dec(){
	if(!cyclic){
		if(value>min){
			value-=step;
		}
		if(value<min){
			value=min;
		}
	}
	else{
		if(value>=min){
			value-=step;
		}
		if(value<min){
			value=max;
		}
	}
}

int32_t GUI_EL_SpinBox::GetValue(){
	return value;
}

void GUI_EL_SpinBox::SetValue(int32_t value){
	this->value=value;
}

void GUI_EL_SpinBox::CalcContentGeom(){
	gselfont(lab_params->font);
	label_h=ggetfh()+lab_params->element.margins.Bottom+lab_params->element.margins.Top;
	content.W=GEOM_W(geom);
	content.H=label_h+gsymh(up_arrow)+gsymh(down_arrow)+4;//+4 это марджины иконок
}

void GUI_EL_SpinBox::SetInputFocus(bool isFocus)
{
}


//+++++++++++++Window+++++++++++++++++++++

GUI_EL_Window::GUI_EL_Window(WindowParams *params, MoonsGeometry *geom, GUI_Obj *parrent_obj):GUI_Element(geom, &no_align, &zero_margins, parrent_obj){
	frame_thick=params->frame_thick;
	color_sch=params->color_sch;
	round_corners=params->round_corners;

}

//-----------------------------

#define GLOBAL_WINDOW_ARC	5

void GUI_EL_Window::Draw(){
	int32_t i=0;
	PrepareContent();
	PrepareViewport();
	gsetcolorf(color_sch.foreground);
	gsetcolorb(color_sch.background);

	GXT arc=0;
	if(round_corners){
		arc=GLOBAL_WINDOW_ARC;
	}

	for(i=0;i<frame_thick;++i){
		groundrect(content.x+i,content.y+i, CONTENT_XE(content)-i, CONTENT_YE(content)-i,arc,GLINE);
	}

	groundrect(content.x+i,content.y+i, CONTENT_XE(content)-i, CONTENT_YE(content)-i,arc,GFILL);
}

//-----------------------------

void GUI_EL_Window::CalcContentGeom()
{
    content.H = GEOM_H(el_geom);
    content.W = GEOM_W(el_geom);
}

void GUI_EL_Window::SetInputFocus(bool isFocus)
{
}


//+++++++++++++Menu Item+++++++++++++++++++++

GUI_EL_MenuItem::GUI_EL_MenuItem(MenuItemParams *params,
                                 MoonsGeometry  *geom,
                                 char           *text,
                                 bool            rec_flag,
                                 GUI_Obj        *parrent_obj
                                ):
                                 GUI_EL_Label(&params->label_params, geom, text, parrent_obj),
                                 mark(&params->icon_params.element, geom, params->icon_params.icon, parrent_obj)
{
    this->rec_flag = rec_flag;
    this->params = *params;
}

void GUI_EL_MenuItem::Draw()
{
	GUI_EL_Label::Draw();
	PrepareViewport();
    gsetcolorf(GENERAL_FORE_COLOR);
    if (rec_flag)
        groundrect(0,0,GEOM_W(el_geom)-1,GEOM_H(el_geom)-1,0,GLINE);

}

//-----------------------------

void GUI_EL_MenuItem::CalcContentGeom()
{
    content.W = GEOM_W(el_geom) - margins.Left-margins.Right;
    content.H = GEOM_H(el_geom) - margins.Top-margins.Bottom;
}

void GUI_EL_MenuItem::SetInputFocus(bool isFocus)
{
    MenuItemParams item_params;
    if (isFocus)
    {
        //item_params = params.label_params;
        // item_params =&GUI_EL_TEMP_ActiveMenuItem;
         //item_params->label_params.font = &Consolas25x35;
         //GUI_EL_Label::SetParams(&item_params->label_params);

        GUI_EL_Label::SetParams(&params.label_params);
    }
    else
    {
        item_params = params;
        //item_params =&GUI_EL_TEMP_DefaultMenuItem;
        //item_params->label_params.font = &Consolas25x35;
        item_params.label_params.element.align = {alignHCenter, alignVCenter};
        item_params.label_params.transparent = true;
        item_params.icon_params.icon = sym_blank;
        GUI_EL_Label::SetParams(&item_params.label_params);
    }
}


//+++++++++++++Slider+++++++++++++++++++++

GUI_EL_Slider::GUI_EL_Slider( SliderParams  *params,
                              MoonsGeometry *geom,
                              GUI_Obj       *parrent_obj
                              ):
                            GUI_Element(geom, &no_align, &zero_margins, parrent_obj),
                            up_arrow(&GUI_EL_TEMP_SliderUpArrow, geom, sym_arrow_up, parrent_obj),
                            down_arrow(&GUI_EL_TEMP_SliderDownArrow, geom, sym_arrow_down, parrent_obj)
{
    list_size   = params->list_size;
    window_size = params->window_size;
	SetWindowOffset(params->window_offset);
}

//-----------------------------

#define MIN_SLIDER_HEIGHT	1

void GUI_EL_Slider::Draw(){
	int32_t temp_wind_size=window_size;
	if(temp_wind_size>list_size){
		temp_wind_size=list_size;
	}
	up_arrow.Draw();
	down_arrow.Draw();


	PrepareViewport();
	gsetcolorf(GENERAL_FORE_COLOR);
	gsetcolorb(GENERAL_FORE_COLOR);
	GXT xs=1;
	GXT xe=GEOM_W(el_geom)-2;
	GYT ys=gsymh(sym_arrow_up)+1;
	GYT ye=GEOM_H(el_geom)-gsymh(sym_arrow_up)-2;
	groundrect(xs,ys,xe,ye,0,GLINE);	//рамка слайдера

	GYT bar_heigth=ye-ys-3;	//высота области в которой движется слайдер
	GYT slider_heigth=(GYT)((temp_wind_size*(int32_t)bar_heigth)/(list_size));	//длина слайдера в пикселях
	if(slider_heigth<MIN_SLIDER_HEIGHT){
		slider_heigth=MIN_SLIDER_HEIGHT;
	}

	xs+=2;	//настройка координат на слайдер
	xe-=2;
	if(window_offset<list_size-temp_wind_size){
		ys=ys+2+((window_offset*bar_heigth)/list_size);
		ye=ys+slider_heigth-1;
	}
	else{	//избавляемся из-за возможного "недохода" слайдера до конца из-за ошибок округления
		ye=ye-2;
		ys=ye-slider_heigth+1;
	}
	groundrect(xs,ys,xe,ye,0,GFILL);

}

//-----------------------------

void GUI_EL_Slider::SetWindowOffset(int32_t offset){
	window_offset=offset;
}

//-----------------------------

void GUI_EL_Slider::CalcContentGeom(){
	content.W=GEOM_W(el_geom)- margins.Left-margins.Right;
	content.H=GEOM_H(el_geom)- margins.Top-margins.Bottom;
}

void GUI_EL_Slider::SetInputFocus(bool isFocus)
{
}

//-----------------------------

GUI_EL_InputString::GUI_EL_InputString(LabelParams *params, MoonsGeometry *geom, char *text, int length, int position, GUI_Obj *parrent_obj):GUI_EL_Label(params, geom, text, parrent_obj){
	this->position=position;
	this->length=length;
}

//-----------------------------

void GUI_EL_InputString::Draw(){
	int i=0;
	PrepareContent();
	PrepareViewport();

	GUI_EL_Label::Draw();

	//рисуем подчеркивания
	GXT offset;
	GYT y_=content.y+ggetfh();
	GXT x_=content.x;
	for(i=0;i<length;++i, x_+=offset){
		offset=ggetsymw(text[i]);
		groundrect(x_+1,y_+1,x_+offset-1,y_,0,GFRAME);
		if(i==position){
			gputsym((x_+(offset-sym_arrow_up->sh.cxpix)/2),y_+3,sym_arrow_up);
		}
	}
}

//-----------------------------

void GUI_EL_InputString::CalcContentGeom(){
	GUI_EL_Label::CalcContentGeom();
}

//+++++++++++++Button+++++++++++++++++++++

GUI_EL_Button::GUI_EL_Button(LabelParams *params, MoonsGeometry *geom, char *text, tButtonStatus status, GUI_Obj *parrent_obj):GUI_EL_Label(params, geom, text, parrent_obj){
	this->status=status;
}

//-----------------------------

void GUI_EL_Button::Draw(){
	int i=0;
	GXT thick=1;
	PrepareContent();
	PrepareViewport();
	GUI_EL_Label::Draw();
	if(status==bs_selected) thick=2;
	for(i=0;i<thick;++i){
		groundrect(0+i,0+i,GEOM_W(el_geom)-1-i,GEOM_H(el_geom)-1-i,2,GLINE);
	}
}

//-----------------------------

void GUI_EL_Button::CalcContentGeom(){
	GUI_EL_Label::CalcContentGeom();
}

//+++++++++++++Bar+++++++++++++++++++++

GUI_EL_Bar::GUI_EL_Bar(BarParams *params, MoonsGeometry *geom, GUI_Obj *parrent_obj):GUI_Element(geom,&bottom_align, &zero_margins,parrent_obj){
	this->min_val=params->min_val;
	this->max_val=params->max_val;
	this->cur_val=params->init_val;
}

//-----------------------------

void GUI_EL_Bar::Draw(){
	GXT bar_len;
	PrepareContent();
	PrepareViewport();
	gsetcolorf(GENERAL_FORE_COLOR);
	gsetcolorb(GENERAL_FORE_COLOR);
	bar_len=((content.W-5)*(cur_val-min_val))/(max_val-min_val); //актуальная длина бара
	groundrect(content.x,content.y,CONTENT_XE(content),CONTENT_YE(content),0,GLINE);
	if(bar_len>0){
		groundrect(content.x+2,content.y+2,content.x+2+bar_len,CONTENT_YE(content)-2,0,GFILL);
	}
}

//-----------------------------

void GUI_EL_Bar::UpdateVal(int32_t val){
	cur_val=val;
}

//-----------------------------

void GUI_EL_Bar::CalcContentGeom(){
	content.W=GEOM_W(el_geom)- margins.Left-margins.Right;
	content.H=GEOM_H(el_geom)- margins.Top-margins.Bottom;
}

void GUI_EL_Bar::SetInputFocus(bool isFocus)
{
}

//+++++++++++++Bar with marked lvl+++++++++++++++++++++


GUI_EL_MarkLvlBar::GUI_EL_MarkLvlBar(BarParams *params, MoonsGeometry *geom,int32_t mark_lvl, GUI_Obj *parrent_obj):GUI_EL_Bar(params,geom,parrent_obj){
	this->mark_lvl=mark_lvl;

}

//-----------------------------

void GUI_EL_MarkLvlBar::Draw(){
	GUI_EL_Bar::Draw();
	GXT mark_x=content.x + (GXT)( ((int)content.W*(mark_lvl-min_val))/(max_val-min_val) );
	groundrect(mark_x,content.y-MARK_HEIGHT,mark_x,content.y,0,GFRAME);
}

//-----------------------------

void GUI_EL_MarkLvlBar::CalcContentGeom(){
	content.W=GEOM_W(el_geom)- margins.Left-margins.Right;
	content.H=GEOM_H(el_geom)- margins.Top-margins.Bottom-MARK_HEIGHT;
}

//-----------------------------

GUI_Painter::GUI_Painter()
{

}

void GUI_Painter::SetMode(DrawMode drawMode)
{
    GMODE mode;
    switch (drawMode){
        case DM_NORMAL:      mode = GNORMAL; break;
        case DM_TRANSPARENT: mode = GTRANSPERANT; break;
    }
    gsetmode(mode);
}

void GUI_Painter::ClearViewPort()
{
    gclrvp();
}

void GUI_Painter::SetColorScheme(ColorSchemeType colorScheme)
{
    switch (colorScheme){
        case CST_DEFAULT: gsetcolorb(GENERAL_BACK_COLOR); gsetcolorf(GENERAL_FORE_COLOR); break;
        case CST_INVERSE: gsetcolorb(GENERAL_FORE_COLOR); gsetcolorf(GENERAL_BACK_COLOR); break;
    }
}

void GUI_Painter::SelectViewPort(unsigned char vp)
{
    gselvp(vp);
}

void GUI_Painter::SelectFont(PGFONT font)
{
    gselfont(font);
}

void GUI_Painter::SetViewPort(unsigned char xs, unsigned char ys, unsigned char xe, unsigned char ye)
{
    gsetvp(xs, ys, xe, ye);
}

void GUI_Painter::DrawLine(unsigned char xs,
                           unsigned char ys,
                           unsigned char xe,
                           unsigned char ye,
                           ColorSchemeType color_scheme)
{
    switch (color_scheme){
        case CST_DEFAULT: gsetcolorb(GENERAL_BACK_COLOR); gsetcolorf(GENERAL_FORE_COLOR); break;
        case CST_INVERSE: gsetcolorb(GENERAL_FORE_COLOR); gsetcolorf(GENERAL_BACK_COLOR); break;
    }
    gmoveto(xs,ys);
    glineto(xe,ye);
}

void GUI_Painter::DrawLine(GPoint start,
                           GPoint end,
                           ColorSchemeType color_scheme)
{

    DrawLine(start.x,start.y,end.x,end.y,color_scheme);
}

void GUI_Painter::DrawLine(MoonsGeometry coord,
                           ColorSchemeType color_scheme)
{
    DrawLine(coord.xs,coord.ys,coord.xe,coord.ye,color_scheme);
}

void GUI_Painter::DrawRect(unsigned char xs,
                           unsigned char ys,
                           unsigned char xe,
                           unsigned char ye,
                           RectDrawMode drm,
                           ColorSchemeType color_scheme,
                           unsigned char radius)
{
    SGUCHAR mode;
    switch (drm){
        case RDM_LINE :  mode = GLINE; break;
        case RDM_FRAME : mode = GFRAME; break;
        case RDM_FILL :  mode = GFILL; break;
    }

    switch (color_scheme){
        case CST_DEFAULT: gsetcolorb(GENERAL_BACK_COLOR); gsetcolorf(GENERAL_TEXT_COLOR); break;
        case CST_INVERSE: gsetcolorb(GENERAL_TEXT_COLOR); gsetcolorf(GENERAL_BACK_COLOR); break;
    }

    groundrect(xs,ys,xe,ye,radius,mode);
}

void GUI_Painter::DrawRect(MoonsGeometry rect,
                           RectDrawMode drm,
                           ColorSchemeType color_scheme,
                           unsigned char radius)
{
    DrawRect(rect.xs,rect.ys,rect.xe,rect.ye,drm,color_scheme,radius);
}

void GUI_Painter::DrawText( unsigned char x,
                           unsigned char y,
                           PGFONT font,
                           char *text,
                           ColorSchemeType color_scheme,
                           DrawMode drawMode)
{

    switch (color_scheme){
        case CST_DEFAULT: gsetcolorb(GENERAL_BACK_COLOR); gsetcolorf(GENERAL_TEXT_COLOR); break;
        case CST_INVERSE: gsetcolorb(GENERAL_TEXT_COLOR); gsetcolorf(GENERAL_BACK_COLOR); break;
    }

    GMODE mode;
    switch (drawMode){
        case DM_NORMAL:      mode = GNORMAL; break;
        case DM_TRANSPARENT: mode = GTRANSPERANT; break;
        case DM_INVERSE:     mode = GINVERSE; break;
    }

    gsetmode(mode);
    gselfont(font);

    std::string str(text);
    convertStrToHackEncoding((uint8_t*)&str[0], str.size(), font);

    gsetvp(0, 0, 159, 127);
    gsetpos(x, y + font->symheight - 2);
    gputs(str.c_str());
}


//void GUI_Painter::DrawText(MoonsGeometry vp,
//                           GPoint coord,
//                           PGFONT font,
//                           char *text,
//                           ColorSchemeType color_scheme,
//                           DrawMode drawMode)
//{
//    DrawText(vp.xs,vp.ys,vp.xe,vp.ye,coord.x,coord.y,font,text,color_scheme,drawMode);
//}

//-------------------------------------------------------

GUI_EL_ScrollArea::GUI_EL_ScrollArea(MoonsGeometry *geom,
                                     Alignment *align,
                                     Margins *margins,
                                     GUI_Obj *parent_obj
                                     ):
                                        GUI_Element(geom,
                                                    align,
                                                    margins,
                                                    parent_obj){
visibleElemsCount = 0;
visElemInd.begin = 0;
visElemInd.end = 0;
focus = 0;
isVScroll = false;
PrepareContent();
}

GUI_EL_ScrollArea::~GUI_EL_ScrollArea()
{
    for (uint16_t i = 0; i < elements.size(); i++)
        delete elements[i];
}

void GUI_EL_ScrollArea::Draw(){


    PrepareContent();
    PrepareViewport();
    for (uint16_t i = visElemInd.begin, c = 0; i < visElemInd.end; i++, c++ )
    {
        MoonsGeometry geom_prev;
        if (i == visElemInd.begin)
        {
           geom_prev = {el_geom.xs,0,el_geom.xe,el_geom.ys};
        }
        else
        {
           GUI_Element *elemPrev = elements[i-1];
           geom_prev = elemPrev->el_geom;
        }
        GUI_Element *elem = elements[i];
        if (elem)
          {
            int32_t h = GEOM_H(elem->el_geom);
            elem->el_geom.xs = geom_prev.xs;
            elem->el_geom.xe = geom_prev.xe;
            elem->el_geom.ys = geom_prev.ye + 1;
            elem->el_geom.ye = elem->el_geom.ys + h;
            elem->Draw();
          }
    }

   // vector<GUI_Element>::iterator i = elements[visElemInd.begin];

    //GUI_Element* i = elements[visElemInd.begin];
 //   while(i != elements[visElemInd.end])
  //  {
        //GUI_Element* el = ((GUI_Element*)*i);
//        i->Draw();
 //       i++;
 //   }
    if (isVScroll)
    {
        MoonsGeometry sliderArea  = { (GXT)el_geom.xe,(GYT)el_geom.ys , (GXT)(el_geom.xe + 10), (GYT)el_geom.ye};
        SliderParams  sliderParams = {elements.size(), 1, focus};
        GUI_EL_Slider slider( &sliderParams, &sliderArea, (GUI_Obj *)this->parent_obj);
        slider.Draw();
    }
}

void GUI_EL_ScrollArea::ClearCanvas()
{
 //ClearCanvasFull();
}

void GUI_EL_ScrollArea::SetInputFocus(bool isFocus)
{

}

void GUI_EL_ScrollArea::CalcContentGeom(){
    content.W = GEOM_W(el_geom) - margins.Left-margins.Right;
    content.H = GEOM_H(el_geom) - margins.Top-margins.Bottom;

    allContent.H = 0;
    allContent.W = 0;

    std::vector<GUI_Element*>::iterator i = elements.begin();
    while(i != elements.end())
    {
        GUI_Element*  el = ((GUI_Element*)*i);

        MoonsGeometry geom = el->el_geom;
        allContent.H += GEOM_H(geom); allContent.H++;
        allContent.W += GEOM_W(geom);

        i++;
    }
    if (allContent.H > GEOM_H(el_geom) && !isVScroll)
    {
        el_geom.xe -= 10;
        isVScroll = true;
    }
    visibleElemsCount = getVisElemCount();
//    if (focus >= visibleElemsCount)
//    {
//        if (focus < elements.size() - visibleElemsCount)
//        {
//            visElemInd.end = focus;
//             visElemInd.begin = visElemInd.end - visibleElemsCount;
//        }
//       else
//        {
//            visElemInd.end = elements.size();
//            visElemInd.begin = visElemInd.end - visibleElemsCount;
//        }
//    }
//    else
//    {
//        visElemInd.begin = 0;
//        visElemInd.end = visibleElemsCount;
//    }

    visElemInd.end = visElemInd.begin + visibleElemsCount;
}

uint32_t GUI_EL_ScrollArea::getVisElemCount(){
    int32_t h = el_geom.ys, visCount = 0;
    for (int32_t i = 0 ; i < elements.size() ; i++)
    {
        GUI_Element *elem = elements[i];

        if (h + GEOM_H(elem->el_geom) < el_geom.ye)
        {
            h += GEOM_H(elem->el_geom);
            visCount++;
        }
        else
            break;
    }
    return visCount;
}

uint32_t GUI_EL_ScrollArea::setFirstVisElem(const int32_t elemIndex)
{

    if (elemIndex >= 0 && elemIndex < elements.size() - visibleElemsCount)
    {
        visElemInd.begin = elemIndex;
        visElemInd.end = visElemInd.begin + visibleElemsCount;
    }
    else
    {
        if (elemIndex < 0 )
            visElemInd.begin = 0;
        if (elemIndex > elements.size() - visibleElemsCount)
            visElemInd.begin = elements.size() - visibleElemsCount;
    }
    return visElemInd.begin;
}

uint32_t GUI_EL_ScrollArea::getFirstVisElem()
{
    return visElemInd.begin;
}

void  GUI_EL_ScrollArea::addGuiElement(GUI_Element *element){
    elements.push_back(element);
    CalcContentGeom();
}

void  GUI_EL_ScrollArea::removeGuiElement(GUI_Element *element){
    //elements.erase(element);
}

int GUI_EL_ScrollArea::incFocus(){
 setFocus(++focus);
 return focus;
}

int GUI_EL_ScrollArea::decFocus(){
  setFocus(--focus);
  return focus;
}

void GUI_EL_ScrollArea::setFocus(const uint32_t elemIndex){
    PrepareContent();
    if (elemIndex >= 0 && elemIndex < elements.size())
    {
            if (elemIndex >= visElemInd.end)
            {
                visElemInd.end = elemIndex;
                visElemInd.begin = visElemInd.end - visibleElemsCount + 1;
            }
            if (elemIndex < visElemInd.begin)
            {
                visElemInd.begin = elemIndex;
                visElemInd.end = visElemInd.begin + visibleElemsCount;
            }

        GUI_Element* elem = (GUI_Element*)elements[focus];
        elem->SetInputFocus(false);
        focus  = elemIndex;
        elem = (GUI_Element*)elements[focus];
        elem->SetInputFocus();

    //Draw();
    }
}

void GUI_EL_ScrollArea::activateElement(const uint32_t elemIndex){

}

void GUI_EL_ScrollArea::activateFocusElement(){

}
