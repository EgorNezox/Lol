#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>

struct RectArea
{
    uint16_t lx;
    uint16_t ly;
    uint16_t rx;
    uint16_t ry;
};

typedef uint16_t GColor;

class Display
{
public:

    Display(){};

    uint16_t getWidth(){return width;}
    uint16_t getHeight(){return height;}

    virtual bool init();
    virtual void enable();
    virtual void disable();
    virtual GColor read(uint16_t x, uint16_t y);
    virtual void setViewPort(RectArea area);
    RectArea getViewPort() {return viewPort;}

    virtual void autoWrite(GColor color);
    virtual void update();

    virtual void setHautoInc();
    virtual void setVautoInc();

protected:

    virtual void writeCmd(uint8_t cmd);
   // virtual void writeData(uint8_t data);
  //  virtual void writeData(uint8_t data1, uint8_t data2);
    //virtual void writeData(uint8_t* data, uint8_t dataSize);
    virtual void writeCmdData(uint8_t cmd, uint8_t data);
    virtual void writeCmdData(uint8_t cmd, uint8_t data1, uint8_t data2);
    //virtual void writeCmdData(uint8_t cmd, uint8_t* data, uint8_t dataSize);

    uint16_t width;  // px
    uint16_t height; // px

    uint16_t xOffset = 0; // px
    uint16_t yOffset = 0; // px

    uint16_t busWidth;
    uint16_t bitPerPixel;

    RectArea viewPort;

    bool isError;


};

#endif // DISPLAY_H
