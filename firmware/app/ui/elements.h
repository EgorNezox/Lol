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
#include <vector>

//-----------------------------
#define MAX_LABEL_LENGTH 60
#define MAX_TEXT_AREA_LENGTH 180

struct ElementParams{
    Margins margins;
    Alignment align;
};

struct FullContentSize{
    uint32_t H = 0;
    uint32_t W = 0;
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
        virtual void SetInputFocus(bool isFocus = true) = 0;
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
		void setSkipTextBackgronundFilling(bool enabled);
		void SetText(char *text);
        bool transparent;
        void SetParams(LabelParams *params);
        virtual void SetInputFocus(bool isFocus);
	private:

		PGFONT font;
		ColorScheme color_sch;
        bool skip_text_bg_filling;
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
        GUI_EL_TextArea(TextAreaParams *params, MoonsGeometry *geom, std::vector<uint8_t> *data, GUI_Obj *parent_obj);
        ~GUI_EL_TextArea();
        void Draw();
        void SetText(char *text);
        void SetText(std::vector<uint8_t> *data);
        void ScrollUp();
        void ScrollDown();
        uint32_t GetScrollIndex();
        uint32_t GetMaxScrollIndex();
        uint32_t SetScrollIndex(uint32_t index);
        virtual void SetInputFocus(bool isFocus);
        void setVisibleScroll(bool isVisible);
    private:
        void copyStrFromData(char *dest, uint32_t index, uint32_t count);
        char getChar(uint32_t index);
        uint32_t getDataSize();
        uint32_t lines_count;
        GYT line_height;
        TextAreaParams params;
        bool isScroll = false;
        bool isVisibleScroll = false;
        uint32_t visLineBegin = 0;
       // int32_t visLineBeginMax = 0;
        uint32_t visLinesCount = 0;
        bool isData = false;
        FullContentSize allContent;
    protected:
        void CalcContentGeom();
        char* text = 0;
        std::vector<uint8_t>* data = 0;
        //char* text[MAX_TEXT_AREA_LENGTH];
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

        // GUI_Element interface
public:
        virtual void SetInputFocus(bool isFocus);
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

        // GUI_Element interface
public:
        virtual void SetInputFocus(bool isFocus);
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

        // GUI_Element interface
public:
        virtual void SetInputFocus(bool isFocus);
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
        LabelParams *lab_params;
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

        PGSYMBOL up_arrow;
        PGSYMBOL down_arrow;
        GYT label_h;
        char str[10];

        // GUI_Element interface
public:
        virtual void SetInputFocus(bool isFocus);
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

        // GUI_Element interface
public:
        virtual void SetInputFocus(bool isFocus);
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
        GUI_EL_MenuItem(MenuItemParams *params, MoonsGeometry *geom, char *text, bool rec, GUI_Obj *parrent_obj );
        char *text;
    private:
        GUI_EL_Icon mark;
        bool rec_flag;
        MenuItemParams params;
    protected:
        void CalcContentGeom();

        // GUI_Element interface
public:
        virtual void SetInputFocus(bool isFocus);
};

//-----------------------------

struct SliderParams{
    uint32_t list_size;
    uint32_t window_size;
    uint32_t window_offset;
};

/*!Класс элемента бегунок*/
class GUI_EL_Slider: public GUI_Element{
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

        // GUI_Element interface
public:
        virtual void SetInputFocus(bool isFocus);
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

        // GUI_Element interface
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

        // GUI_Element interface
public:
        virtual void SetInputFocus(bool isFocus);
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

//-----------------------------

enum ColorSchemeType {CST_DEFAULT = 0, CST_INVERSE = 1};
enum RectDrawMode {RDM_LINE = 0, RDM_FRAME = 1, RDM_FILL = 2};
enum DrawMode {DM_NORMAL = 0, DM_TRANSPARENT = 1, DM_INVERSE = 2};

struct GPoint{
    GXT x;
    GYT y;
};

class GUI_Painter{
    public:
        GUI_Painter();
//               inline void setFont(PGFONT font);
//               inline void setColorSheme(ColorSchemeType color_scheme);
//               inline void setDrawMode(DrawMode drawMode);
//               inline void setRectDrawMode(RectDrawMode rectDrawMode);
//               inline void drawLine(GXT xs,
//                                    GYT ys,
//                                    GXT xe,
//                                    GYT ye);
//               inline void drawRect(GXT xs,
//                                    GYT ys,
//                                    GXT xe,
//                                    GYT ye,
//                                    RectDrawMode drm,
//                                    ColorSchemeType color_scheme = CST_DEFAULT,
//                                    GXYT radius = 0);

// ----------  static -------------------------------------------------------

                static void SetMode(DrawMode drawMode);

          static void ClearViewPort();

         static void SetColorScheme(ColorSchemeType colorScheme);

         static void SelectViewPort(SGUCHAR vp);

             static void SelectFont(PGFONT font);

            static void SetViewPort(GXT xs,
                                    GYT ys,
                                    GXT xe,
                                    GYT ye);

        inline static void DrawLine(GXT xs,
                                    GYT ys,
                                    GXT xe,
                                    GYT ye,
                                    ColorSchemeType color_scheme = CST_DEFAULT);

               static void DrawLine(GPoint start,
                                    GPoint end,
                                    ColorSchemeType color_scheme = CST_DEFAULT);

               static void DrawLine(MoonsGeometry coord,
                                    ColorSchemeType color_scheme = CST_DEFAULT);

        inline static void DrawRect(GXT xs,
                                    GYT ys,
                                    GXT xe,
                                    GYT ye,
                                    RectDrawMode drm,
                                    ColorSchemeType color_scheme = CST_DEFAULT,
                                    GXYT radius = 0);

               static void DrawRect(MoonsGeometry rect,
                                    RectDrawMode drm,
                                    ColorSchemeType color_scheme = CST_DEFAULT,
                                    GXYT radius = 0);

               static void DrawText(GXT x,
                                    GYT y,
                                    PGFONT font,
                                    char* text,
                                    ColorSchemeType color_scheme = CST_DEFAULT,
                                    DrawMode drawMode = DM_TRANSPARENT);

//        inline static void DrawText(GXT x,
//                                    GYT y,
//                                    PGFONT font,
//                                    char* text,
//                                    ColorSchemeType color_scheme = CST_DEFAULT,
//                                    DrawMode drawMode = DM_TRANSPARENT);

//               static void DrawText(MoonsGeometry vp,
//                                    GPoint coord,
//                                    PGFONT font,
//                                    char *text,
//                                    ColorSchemeType color_scheme = CST_DEFAULT,
//                                    DrawMode drawMode = DM_TRANSPARENT);
};

//----------------------------------------------------


/*!Класс элемента прокручиваемая зона*/
class GUI_EL_ScrollArea: public GUI_Element{
    public:
        struct visibleElemIndex{
            uint32_t begin;
            uint32_t end;
        };
        GUI_EL_ScrollArea(MoonsGeometry *geom, Alignment *align, Margins *margins, GUI_Obj *parent_obj);
        ~GUI_EL_ScrollArea();
        void Draw();
        void ClearCanvas();
        void SetInputFocus(bool isFocus = true);
        void addGuiElement(GUI_Element* element);
        void removeGuiElement(GUI_Element* element);
        int incFocus();
        int decFocus();
        void setFocus(const uint32_t elemIndex);
        void activateElement(const uint32_t elemIndex);
        void activateFocusElement();
        GUI_Element* getFocusElement();
        uint32_t getVisElemCount();
        uint32_t setFirstVisElem(const int32_t elemIndex);
        uint32_t getFirstVisElem();
    protected:
        void CalcContentGeom();
        std::vector<GUI_Element*> elements;
        FullContentSize allContent;
        int visibleElemsCount; // количество отображаемых элементов из всех в данный момент
        visibleElemIndex visElemInd; // индекс отображаемого элемента
        bool isVScroll; // есть ли вертикальный скроллбар
        uint16_t focus; // номер элемента с фокусом
        //GUI_EL_Slider hSlider;
};

#include "element_templates.h"

#endif /* ELEMENTS_H_ */
