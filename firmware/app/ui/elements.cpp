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
#include "elements.h"
#include "all_sym_indicators.h"


//----------DEFINES------------

#define VP_COMMON	0

//----------TYPES--------------

//----------GLOBAL_VARS--------

static Margins zero_margins={0,0,0,0};
static Alignment no_align={alignLeft, alignTop};
static Alignment bottom_align={alignLeft, alignBottom};

//----------PROTOTYPES---------

//----------CODE---------------


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
	font=params->font;
	transparent=params->transparent;
	color_sch=params->color_sch;
	SetText(text);
	if(text!=0)PrepareContent();
}

//-----------------------------

void GUI_EL_Label::SetText(char *text){
	if(text!=0){
		strcpy(this->text, text);
	}
}

//-----------------------------

void GUI_EL_Label::Draw(){
	if(text!=0){
		PrepareContent();
		PrepareViewport();
		gselfont(font);
		gsetcolorb(color_sch.background);
		gsetcolorf(color_sch.foreground);
		if(!transparent){
			gsetmode(COMMON_ELEMENT_VP_MODE);
            groundrect(0,0,GEOM_W(el_geom)-1,GEOM_H(el_geom)-1,0,GLINE);
		}
		else {
			gsetmode(COMMON_ELEMENT_VP_MODE | GTRANSPERANT);
		}
		gsetpos(content.x, CONTENT_YE(content));
		gputs(text);
		if(transparent){
			gsetmode(COMMON_ELEMENT_VP_MODE);
		}
	}
}

//-----------------------------

void GUI_EL_Label::CalcContentGeom(){
	int32_t i = 0, size=strlen(text);
	content.W=0;
	content.H=0;
	gselfont(font);
	for (i = 0; i < size; i++)		//Вычисляем ширину и высоту
	{
		content.W += ggetsymw(text[i]);
	}
	content.H=ggetfh();

}



//+++++++++++++TextArea+++++++++++++++++++++

GUI_EL_TextArea::GUI_EL_TextArea(TextAreaParams *params, MoonsGeometry *geom, char *text, GUI_Obj *parent_obj):GUI_Element(geom, &params->element.align, &params->element.margins, parent_obj){
 this->params=*params;
 lines_count=0;
 SetText(text);
}

//-----------------------------

void GUI_EL_TextArea::SetText(char *text){
	if(text!=NULL){
		strcpy(this->text, text);
	}
}

//-----------------------------

void GUI_EL_TextArea::Draw(){
	if(text!=0){
		int32_t i=0, j=0,k=0,str_width=0, last_space=0, sym_to_cp=0;
		MoonsGeometry local_content, line_geom;
		char line_str[MAX_LABEL_LENGTH];
		LabelParams label_params=params;
		label_params.element.align.align_v=alignTop;
		label_params.element.margins={0,0,0,0};
		PrepareContent();
		PrepareViewport();
		local_content=GetContentGeomOnElem();
		line_geom=local_content;
		line_geom.ye=line_geom.ys+line_height-1;
		for(i=0;i<lines_count;++i, line_geom.ys+=line_height, line_geom.ye+=line_height){
			for(j=0, str_width=0, last_space=0;;++j,++k){
				if(text[k]=='\n' || text[k]==0){
					strncpy(line_str,&text[k-j],j);
					line_str[j]=0;
					++k;
					break;
				}
				else{
					if(text[k]==' '){
						last_space=k;
					}
					str_width+=ggetsymw(text[k]);
					if(str_width>GEOM_W(el_geom)){
						if(last_space==0 || text[k]==' '){
							sym_to_cp=j;
							strncpy(line_str,&text[k-j],sym_to_cp);
							line_str[sym_to_cp]=0;
							++k;
							break;
						}
						else{
							sym_to_cp=j-(k-last_space);
							strncpy(line_str,&text[k-j],sym_to_cp);
							line_str[sym_to_cp]=0;
							k=last_space+1;
							break;
						}

					}
				}
			}
			GUI_EL_Label text_line(&label_params,&line_geom,line_str,parent_obj);
			text_line.Draw();
		}

	}
}

//-----------------------------

void GUI_EL_TextArea::CalcContentGeom(){
	int32_t i = 0, lf_count=0, max_str_width=0, str_width=0, size=strlen(text), last_space=0, last_str_with=0;
	content.W=0;
	content.H=0;
	gselfont(params.font);
	for(i=0;i<=size;++i){ //	подсчет количества строк
		if(text[i]==' '){
			last_space=i;
			last_str_with=str_width;
		}
		if(text[i]=='\n' || text[i]==0){
			++lf_count;
			if(str_width>max_str_width){
				max_str_width=str_width;
			}
			str_width=0;
		}
		else {
			str_width+=ggetsymw(text[i]);
			if(str_width>GEOM_W(el_geom)){		//если строка длиннее чем область элемента
				++lf_count;						//cчитаем перенос строки
				if(last_space==0 || text[i]==' '){
					str_width-=ggetsymw(text[i]);	//и отменяем сложение длины этого символа
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
	lines_count=lf_count;
	line_height=ggetfh();
	content.H=ggetfh()*lines_count;
	content.W=max_str_width;
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
	(*ValueToStr)(value,str);
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




//+++++++++++++Menu Item+++++++++++++++++++++

GUI_EL_MenuItem::GUI_EL_MenuItem(MenuItemParams *params,
                                 MoonsGeometry  *geom,
                                 char           *text,
                                 bool            draw_mark,
                                 bool            rec_flag,
                                 GUI_Obj        *parrent_obj
                                ):
                                 GUI_EL_Label(&params->label_params, geom, text, parrent_obj),
                                 mark(&params->icon_params.element, geom, params->icon_params.icon, parrent_obj)
{
    this->draw_mark = draw_mark;
    this->rec_flag = rec_flag;
}

void GUI_EL_MenuItem::Draw()
{
	GUI_EL_Label::Draw();
    if (draw_mark) mark.Draw();
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
