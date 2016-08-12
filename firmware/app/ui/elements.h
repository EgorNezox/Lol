/**
  ******************************************************************************
  * @file     elements.h
  * @author  Egor Dudyak, PMR dept. software team, ONIIP, PJSC
  * @date    08 окт. 2013 г.
  * @brief  Описание абстрактного класса Element, и производных от него классов
  *	представляющих собой отдельные графические элементы с минимальной логикой,
  *	используются в диалогах.
  ******************************************************************************
  */

#ifndef ELEMENTS_H_
#define ELEMENTS_H_

#include <stdint.h>
#include "gdisp.h"
#include "gui_elements_common.h"
#include "gui_obj.h"
#include <string>

//-----------------------------
#define MAX_LABEL_LENGTH 85
#define MAX_TEXT_AREA_LENGTH 180

struct ElementParams{
    Margins margins;
    Alignment align;
};


/*!Базовый абстрактный класс элемента*/
class GUI_Element{
    public:
        GUI_Element(MoonsGeometry *geom, Alignment *align, Margins *margins, GUI_Obj *parent_obj);
        virtual ~GUI_Element();
        virtual void Draw() =0;
        void PrepareContent(); //Должна вызываться в конце конструктора конкретного элемента!
        void PrepareViewport();	//Выбирает текущим вьпорт элемента и настраивает его размеры в соответствии с el_geom, рекомендуется к использованию в отрисовке.
        /*! Возвращает координаты контента относительно координат диалога
         * Внимание! Вызывать только если content уже вычислен
         */
        MoonsGeometry GetContentGeomOnElem();
        ContentSize content;	//Размер и относительные координаты содержимого относительно области элемента. Вычисляются.
        MoonsGeometry el_geom;	//Область элемента. Абсолютные координаты
        MoonsGeometry geom;	//Координаты элемента относительно диалога
        Margins margins;
        Alignment align;
    private:
        void AlignContent();
    protected:
        virtual void CalcContentGeom() =0;
        GUI_Obj *parent_obj;
};

//-----------------------------

struct LabelParams{
    ElementParams element;
    PGFONT font;
    ColorScheme color_sch;
    bool transparent;
};

/*!Класс элемента надпись*/
class GUI_EL_Label: public GUI_Element{
	public:
		void Draw();
		GUI_EL_Label(LabelParams *params, MoonsGeometry *geom, char *text, GUI_Obj *parent_obj);
		void SetText(char *text);
        bool transparent;
	private:

		PGFONT font;
		ColorScheme color_sch;
	protected:
		void CalcContentGeom();
        std::string text;
};

struct IconParams{
    ElementParams element;
    PGSYMBOL icon;
};

//-----------------------------

typedef LabelParams TextAreaParams;

/*!Класс элемента текстовое поле*/
class GUI_EL_TextArea: public GUI_Element{
    public:
        GUI_EL_TextArea(TextAreaParams *params, MoonsGeometry *geom, char *text, GUI_Obj *parent_obj);
        void Draw();
        void SetText(char *text);
        void ScrollUp();
        void ScrollDown();
        int32_t GetScrollIndex();
        int32_t GetMaxScrollIndex();
        int32_t SetScrollIndex(int32_t index);
    private:
        int32_t lines_count;
        GYT line_height;
        TextAreaParams params;
        bool isScroll = false;
        int32_t visLineBegin = 0;
        int32_t visLinesCount = 0;
    protected:
        void CalcContentGeom();
        char text[300];
};

//-----------------------------

/*!Класс элемента иконка*/
class GUI_EL_Icon: public GUI_Element{
    public:
        void Draw();
        GUI_EL_Icon(ElementParams *params, MoonsGeometry *geom, PGSYMBOL icon, GUI_Obj *parent_obj);
        PGSYMBOL icon;
    protected:
        void CalcContentGeom();
};

//-----------------------------

/*!Класс элемента индикатор баттареи*/
class GUI_EL_Battery: public GUI_Element{
    public:
        int charge;	//заряд в процентах
        void Draw();
        GUI_EL_Battery(ElementParams *params, int charge, MoonsGeometry *geom, GUI_Obj *parent_obj);
    protected:
        void CalcContentGeom();
};

//-----------------------------

struct VolumeTunerParams{
    ElementParams el_params;
    uint8_t bar_count;
    GXT bar_interval;
};

/*!Класс элемента регулятор громкости*/
class GUI_EL_VolumeTuner: public GUI_Element{
    public:
        GUI_EL_VolumeTuner(VolumeTunerParams* params, MoonsGeometry* geom, GUI_Obj* parent_obj, uint8_t level = 0);
        void setLevel(uint8_t level);
        void Draw();
    protected:
        void CalcContentGeom();
    private:
        uint8_t bar_count;
        GXT bar_interval;
        uint8_t level;
};

//-----------------------------

struct SpBoxSettings{
    int32_t value;
    int32_t min;
    int32_t max;
    int32_t step;
    int32_t spbox_len;
    bool cyclic;
    /*!Указатель колбек, переводящий цифровое значение в строку для вывода.
     * Предполагается что str достаточной длины
     */
    void (*ValueToStr)(int32_t value, char *str);
};

struct SpBoxParams{
    PGSYMBOL up_arrow;
    PGSYMBOL down_arrow;
};

/*!Класс элемента выбора числового значения*/
class GUI_EL_SpinBox: public GUI_Element{
    public:
        GUI_EL_SpinBox(MoonsGeometry* geom, SpBoxParams *spbox_params, SpBoxSettings *spbox_settings, GUI_Obj* parent_obj);
        void Draw();
        void SetActiveness(bool active);
        void Inc();
        void Dec();
        int32_t GetValue();
        void SetValue(int32_t value);
    protected:
        void CalcContentGeom();
    private:
        void (*ValueToStr)(int32_t value, char *str);	//Указатель колбек, переводящий цифровое значение в строку для вывода.
        bool active;
        bool cyclic;
        int32_t value;
        int32_t min;
        int32_t max;
        int32_t step;
        int32_t spbox_len;
        LabelParams *lab_params;
        PGSYMBOL up_arrow;
        PGSYMBOL down_arrow;
        GYT label_h;
        char str[10];
};

//-----------------------------

struct WindowParams{
        ColorScheme color_sch;
        uint8_t frame_thick;
        bool round_corners;		// если 1, то углы будут скруглены с радиусов 5
};

/*!Класс элемента окно*/
class GUI_EL_Window: public GUI_Element{
    public:
        void Draw();
        GUI_EL_Window(WindowParams *params, MoonsGeometry *geom, GUI_Obj *parrent_obj);
    protected:
        void CalcContentGeom();
    private:
        ColorScheme color_sch;
        uint8_t frame_thick;
        bool round_corners;
};

//-----------------------------

struct MenuItemParams{
    LabelParams label_params;
    IconParams icon_params;
};

/*!Класс элемента меню*/
class GUI_EL_MenuItem: public GUI_EL_Label{
    public:
        void Draw();
        GUI_EL_MenuItem(MenuItemParams *params, MoonsGeometry *geom, char *text, bool draw_mark, bool rec, GUI_Obj *parrent_obj );
        char *text;
    private:
        GUI_EL_Icon mark;
        bool draw_mark;
        bool rec_flag;
    protected:
        void CalcContentGeom();
};

//-----------------------------

struct SliderParams{
    int32_t list_size;
    int32_t window_size;
    int32_t window_offset;
};

/*!Класс элемента бегунок*/
class GUI_EL_Slider:public GUI_Element{
    public:
        GUI_EL_Slider(SliderParams *params, MoonsGeometry *geom, GUI_Obj *parrent_obj);
        void Draw();
        void SetWindowOffset(int32_t offset);
    private:
        int32_t list_size;
        int32_t window_size;
        int32_t window_offset;
        GUI_EL_Icon up_arrow;
        GUI_EL_Icon down_arrow;
    protected:
        void CalcContentGeom();
};

//-----------------------------

/*!Класс элемента вводимая строка*/
class GUI_EL_InputString: public GUI_EL_Label{
    public:
        GUI_EL_InputString(LabelParams *params, MoonsGeometry *geom, char *text, int length,int position, GUI_Obj *parrent_obj);
        void Draw();
        char *GetText();
        int position;
    protected:
        void CalcContentGeom();
    private:
        int length;
};

//-----------------------------

enum tButtonStatus{bs_selected, bs_unselected};

/*!Класс элемента кнопка*/
class GUI_EL_Button: public GUI_EL_Label{
    public:
        GUI_EL_Button(LabelParams *params, MoonsGeometry *geom, char *text, tButtonStatus status, GUI_Obj *parrent_obj);
        void Draw();
        tButtonStatus status;
    protected:
        void CalcContentGeom();
    private:

};

//-----------------------------

struct BarParams{
    int32_t min_val;
    int32_t max_val;
    int32_t init_val;
};

/*!Класс элемента прямоугольная шкала*/
class GUI_EL_Bar: public GUI_Element{
    public:
        GUI_EL_Bar(BarParams *params, MoonsGeometry *geom, GUI_Obj *parrent_obj);
        void Draw();
        void UpdateVal(int32_t val);
    protected:
        int32_t min_val;
        int32_t max_val;
        int32_t cur_val;
        void CalcContentGeom();
};

class GUI_EL_MarkLvlBar: public GUI_EL_Bar{
    public:
        GUI_EL_MarkLvlBar(BarParams *params, MoonsGeometry *geom,int32_t mark_lvl, GUI_Obj *parrent_obj);
        void Draw();
        void CalcContentGeom();
    private:
        #define MARK_HEIGHT	10
        int32_t mark_lvl;


};

#include "element_templates.h"

#endif /* ELEMENTS_H_ */
