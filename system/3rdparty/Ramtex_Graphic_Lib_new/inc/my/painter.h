#ifndef PAINTER_H
#define PAINTER_H

#include "string"
#include "display.h"
#include "gtypes.h"

//    Порядок работы:
//    1) Создаем где-то экземпляр класса Display.
//    2) Создаем GPainter, передавая указатель на конкретный дисплей Display.
//    3) GPainter запускает инициализацию Display:
//        1. core pins or bus(fsmc) init
//        2. init display (отправка команд настроек и влючения дисплея)

//    Порядок отрирсовки:
//    1) Устанавливаем параметры рисования (viewport, mode, color, font...)
//       через верхний уровень (GuiElement or GuiPaiter)
//    2) Вызываем функции отрисовки (pixel, line, rect, sym, text...)
//    3) Они в свою очередь:
//       1. Производят некоторые преобразования, расчеты.
//       2. Вызывают функцию Display.setViewPort(Rect) установки области рисования (для пикселя, линии или прямоугольника).
//       3. Вызывают функцию Display.autoWrite(GColor) для автозаполнения выбранной ранее области заданным цветом

class GPainter
{
public:
    GPainter(Display* aDisplay);

    GFont* getFont()         { return curViewPort->font;                         }
    GMode getMode()          { return curViewPort->mode;                         }
    GColor getBgColor()      { return curViewPort->bgColor;                      }
    GColor getFrColor()      { return curViewPort->frColor;                      }
    GColor getGroundColor()  { return isInverse() ? getBgColor() : getFrColor(); }

    void drawPixel(GPoint point, GColor color);
    void drawLineTo(GPoint b, GPoint e) { moveTo(b); lineTo(e);  }
    void drawLineTo(GPoint to)          {            lineTo(to); }
    void drawRoundRect( uint16_t ltx, uint16_t lty, uint16_t rbx, uint16_t rby, uint16_t r, uint8_t skipflags);
    void drawText( const char* text );

    void moveTo(GPoint point);
    void lineTo(GPoint point);

    void setMode(GMode mode)       { curViewPort->mode = mode;     }
    void setFont(GFont* font)      { curViewPort->font = font;     }
    void setBgColor(GColor color)  { curViewPort->bgColor = color; }
    void setFrColor(GColor color)  { curViewPort->frColor = color; }

    void setColor(GColor bgColor, GColor fgColor);
    void setViewPort(); // max VP
    void setViewPort(GPoint begin, GPoint end);
    void setViewPort(GPointF begin, GPointF end);

    GArea getMaxVP(){return {{0,0},{display->getWidth(),display->getHeight()}};}

    void setCoordMode(CoordMode aCoordMode){coordMode = aCoordMode;};
    CoordMode getCoordMode(){return coordMode;};

    GPoint percentToPixel(GPointF gPointF);

    uint16_t getDisplayWidth()  {return display->getWidth(); }
    uint16_t getDisplayHeight() {return display->getHeight();}

    void setSymPos(GPoint val)        { curViewPort->symbolPos.x = val.x;
    								    curViewPort->symbolPos.y = val.y; }

private:

    uint8_t     viewPortCount = 1;
    Display     *display;
    uint16_t    tabSize;  // px
    GViewPort   *curViewPort;
    uint8_t     viewPortNum = 0; // current number of viewPort
    GViewPort   *viewports;
    bool        isFullHeightLine;
    CoordMode   coordMode = CM_PIXEL;

    //GColor** buffer;
    /* ghw_tmpbuf */ GColor* tmpbuf;

    /* cposx */ uint16_t getSymPosX() { return curViewPort->symbolPos.x;}
    /* cposx */ uint16_t getSymPosY() { return curViewPort->symbolPos.y;}
    /* pposx */ uint16_t getPixPosX() { return curViewPort->pixelPos.x; }
    /* pposy */ uint16_t getPixPosY() { return curViewPort->pixelPos.y; }
    /* ltx */   uint16_t getBx()      { return curViewPort->begin.x;    } // left top x
    /* lty */   uint16_t getBy()      { return curViewPort->begin.y;    } //          y
    /* rbx */   uint16_t getEx()      { return curViewPort->end.x;      } // right bottom x
    /* rby */   uint16_t getEy()      { return curViewPort->end.y;      } //              y
    void setSymPosX(uint16_t val)     { curViewPort->symbolPos.x = val; }
    void setSymPosY(uint16_t val)     { curViewPort->symbolPos.y = val; }


    void setPixPosX(uint16_t val)     { curViewPort->pixelPos.x  = val; }
    void setPixPosY(uint16_t val)     { curViewPort->pixelPos.y  = val; }
    void setBx(uint16_t val)      	  { curViewPort->begin.x     = val; }
    void setBy(uint16_t val)          { curViewPort->begin.y     = val; }
    void setEx(uint16_t val)          { curViewPort->end.x       = val; }
    void setEy(uint16_t val)          { curViewPort->end.y       = val; }

    bool isInverse()      {return (uint8_t)getMode() & (uint8_t)GINVERSE       != 0;}
    bool isNoScroll()     {return (uint8_t)getMode() & (uint8_t)GNOSCROLL      != 0;}
    bool isAlignH()       {return (uint8_t)getMode() & (uint8_t)GALIGN_HCENTER != 0;}
    bool isAlignHR()      {return (uint8_t)getMode() & (uint8_t)GALIGN_RIGHT   != 0;}
    bool isNoWrap()       {return (uint8_t)getMode() & (uint8_t)GNO_WRAP       != 0;}
    bool isAlignTop()     {return (uint8_t)getMode() & (uint8_t)GALIGN_VCENTER          == (uint8_t)GALIGN_TOP;    }
    bool isAlignBottom()  {return (uint8_t)getMode() & (uint8_t)GALIGN_VCENTER          == (uint8_t)GALIGN_BOTTOM; }
    bool isAlignCenterV() {return (uint8_t)getMode() & (uint8_t)GALIGN_VCENTER          == (uint8_t)GALIGN_VCENTER;}
    bool isAlignLeft()    {return (uint8_t)getMode() & (uint8_t)GALIGN_HCENTER          == (uint8_t)GALIGN_LEFT;   }
    bool isAlignRight()   {return (uint8_t)getMode() & (uint8_t)GALIGN_HCENTER          == (uint8_t)GALIGN_RIGHT;  }
    bool isAlignCenterH() {return (uint8_t)getMode() & (uint8_t)GALIGN_HCENTER          == (uint8_t)GALIGN_HCENTER;}
    bool isWordWrap()     {return (uint8_t)getMode() & (uint8_t)(GWORD_WRAP | GNO_WRAP) == (uint8_t)GWORD_WRAP;    }

    bool isVpClrUp()      {return getMode() & GVPCLR_UP    != 0;}
    bool isVpClrDown()    {return getMode() & GVPCLR_DOWN  != 0;}
    bool isVpClrLeft()    {return getMode() & GVPCLR_LEFT  != 0;}
    bool isVpClrRight()   {return getMode() & GVPCLR_RIGHT != 0;}
    bool isTransparent()  {return getMode() & GTRANSPARENT != 0;}

    bool isPartialLine()  {return getMode() & GPARTIAL_LINE != 0;}
    bool isPartialChar()  {return getMode() & GPARTIAL_CHAR != 0;}
    bool isLineCut()      {return (getMode() & (GNO_WRAP | GPARTIAL_CHAR)) == (GNO_WRAP | GPARTIAL_CHAR);}

    /* gi_fsymh           */ uint16_t   getSymH(GFont* pfont)     {return pfont->symHeight;}
    /* gi_fsymw           */ uint16_t   getSymW(GFont* pfont)     {return pfont->symWidth;}
    /* gi_fpcodepage      */ GCodePage* getCodePage(GFont* pfont) {return pfont->codePage;}
    /* gi_fsymsize        */ uint16_t   getSymSize(GFont* pfont)  {return pfont->symSize;}
    /* gi_fnumsym         */ uint16_t   getNumSym(GFont* pfont)   {return pfont->symCount;}
    /* gvfdevice          */ //uint16_t getDevice(Font* pfont)    {return (GFONTDEVICE)0;}

    /* gsymw              */ uint8_t gsymW(GSymbol* psymbol) {giscolor((psymbol)) ? ((GColorSymbol*)psymbol)->head.w : ((GBWSymbol*)psymbol)->head.w;}
    /* gsymh              */ uint8_t gsymH(GSymbol* psymbol) {giscolor((psymbol)) ? ((GColorSymbol*)psymbol)->head.h : ((GBWSymbol*)psymbol)->head.h;}

    /* gfgetfh   		  */ uint16_t gfGetFontH( GFont* fp );
    /* ggetfh    		  */ uint16_t gGetFontH();
    /* ggetfh_vp 		  */ uint16_t gGetFontHvp(uint8_t viewPortNum);

    /* setCodePage        */ void setCodePage(GCodePage* codePage) {curViewPort->font->codePage = codePage;}

    /* gi_resetvp         */ void resetVP(uint8_t resettype);
    /* limitToVP          */ void limitToVP(uint16_t* xs, uint16_t* ys, uint16_t* xe, uint16_t* ye);
    /* limitToViewPort    */ void limitToViewPort(uint16_t* xs, uint16_t* ys,uint16_t* xe,uint16_t* ye);

    /* setPos             */ void setPos( GPoint pos );

    /* gi_process_newline */ uint8_t processNewline(uint16_t lnsp);
    /* ghw_gscroll        */ void scroll(uint16_t ltx, uint16_t lty, uint16_t rbx, uint16_t rby, uint16_t lines, uint16_t pattern);
    /* gi_calcdatacheck   */ void calcDataCheck();

    /* gi_put_prepare     */ void putPrepare(void);
    /* gi_put_complete    */ void putComplete();
    /* gi_putch           */ char putChar( uint8_t val );
    /* gputchw            */ void putCharW( uint8_t val );
    /* gi_putsymbol       */ void putSymbol(uint16_t xs, uint16_t ys, uint16_t xemax, uint16_t yemax, GSymbol *psymbol, uint16_t yoffset, uint16_t symsize);

    /* sizeStringSeg      */ uint8_t sizeStringSeg(StringMarks *stringMarks );

    /* tabStep            */ uint16_t tabStep(uint16_t x);
    /* gi_getSymW         */ uint16_t getSymWidth( uint8_t c );
    /* getSymbol          */ GSymbol* getSymbol(uint8_t c, GFont* pfont, GCodePage* codepagep);

    /* fill               */ void fill(uint16_t ltx, uint16_t lty, uint16_t rbx, uint16_t rby, uint16_t pattern);
    /* ghw_updatehw       */ void updatehw(){return;} // no buffer

    /* drawLine           */ void drawLine(GPoint begin, GPoint end);
    /* drawText           */ void drawText( StringMarks* stringMarks );
    /* ghw_rectangle      */ void drawRectangle(uint16_t ltx, uint16_t lty, uint16_t rbx, uint16_t rby, GColor color);
    /* ghw_lineh          */ void drawLineH(uint16_t xb, uint16_t yb, uint16_t ye, GColor color);
    /* ghw_linev          */ void drawLineV(uint16_t xb, uint16_t yb, uint16_t ye, GColor color);
    /* gi_carc            */ void drawCornerArc(uint16_t xc, uint16_t yc, uint16_t r, uint8_t arctype); // угловая дуга


};

#endif // PAINTER_H
