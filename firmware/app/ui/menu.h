#ifndef MENU
#define MENU

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <list>
#include "service.h"
#include "elements.h"
#include "keyboard.h"

#include "texts.h"

#define MARGIN			4
#define BUTTON_HEIGHT	13
#define BUTTON_WIDTH	30

extern MoonsGeometry ui_common_dialog_area;
extern MoonsGeometry ui_indicator_area;


class CGuiDialog: public GUI_Obj
{
public:
    CGuiDialog(MoonsGeometry* area):GUI_Obj(area){}
    virtual ~CGuiDialog(){}
    void Draw();

    MoonsGeometry windowArea;
    MoonsGeometry titleArea;
    LabelParams   titleParams;
    std::string   titleStr;
};

class CGuiMenu: public CGuiDialog
{
public:
    CGuiMenu(MoonsGeometry* area, const char *title, const char *text, Alignment align);
    virtual ~CGuiMenu();
    void Draw();

    int focus;
    void initItems(std::list<std::string>, const char*, int);
    void initDialog();
    void setTitle(const char*);

private:
    GUI_Obj obj;
    MoonsGeometry menuArea;
    MoonsGeometry textArea;

    TextAreaParams textAreaParams;
    std::string textStr;

    MenuItemParams itemParams;
    MoonsGeometry  itemArea[7];
    bool draw_mark;

    int numItem;
    GUI_EL_MenuItem *(item[6]);

    GUI_EL_Label *label[2];
    GUI_EL_InputString *inputStr[2];
    char *tx;
};


#endif // MENU

