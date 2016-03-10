#ifndef GUI_STACK
#define GUI_STACK

#include <string>
#include <QList>
#include <stack>

enum GuiWindowTypes
{
    mainWindow,\
    messangeWindow,\
    menuWindow
};

class CState
{
    GuiWindowTypes type;
    std::string name;
public:
    CState *nextState;
    CState *prevState;

public:
    CState(){ nextState = nullptr; prevState = nullptr; }
    CState(GuiWindowTypes newType, const char *newName){ type = newType; setName(newName); }
    const char* getName(){ return name.c_str(); }
    void  setName(const char* newName){ name.clear(); name.append(newName); }
    int getType(){ return type; }

    std::list<CState*> statesList;
};

class CGuiTree
{
public:
    static CGuiTree& getInstance()
    {
        static CGuiTree instance;
        return instance;
    }
    ~CGuiTree();

    void append(GuiWindowTypes, char*);
    int  getLastElement(CState&);
    void delLastElement();

private:
    CGuiTree(){}
    CGuiTree( const CGuiTree& );
    CGuiTree& operator=( CGuiTree& );

    QList<const CState*> statesList;

protected:
    void init();
};




#endif // GUI_STACK
