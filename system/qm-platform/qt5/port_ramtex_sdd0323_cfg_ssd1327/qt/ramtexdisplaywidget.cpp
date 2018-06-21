/**
  ******************************************************************************
  * @file    ramtexdisplaywidget.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  * @brief   Реализация класса виджета Qt, симулирующего дисплей LCD с контроллером GHW_SSD1329
  *
  ******************************************************************************
  */

#include "gdispcfg.h"
#include <qpainter.h>

#include "ramtexdisplaywidget.h"

#ifndef GHW_SSD1329
#error "Ramtex display driver should be configured for GHW_SSD1329 controller"
#endif
#if !(GDISPPIXW == 4)
#error "Ramtex dsiplay configuration: only 4-bit pixel mode supported"
#endif

#define GCTRL_ENABLE_EXTERNAL_VDD_REGULATOR        0x00
#define GCTRL_ENABLE_INTERNAL_VDD_REGULATOR        0x01
#define GCTRL_REPRESENT_GPIO_PIN_HIZ_INPUT_DISABLE 0x00
#define GCTRL_REPRESENT_GPIO_PIN_HIZ_INPUT_ENABLE  0x01
#define GCTRL_REPRESENT_GPIO_PIN_OUTPUT_LOW        0x10
#define GCTRL_REPRESENT_GPIO_PIN_OUTPUT_HIGH       0x11

#define GCTRL_DISABLE_COLUMN_ADDRESS_REMAP 		   0x00
#define GCTRL_ENABLE_COLUMN_ADDRESS_REMAP  		   0x01
#define GCTRL_DISABLE_NIBBLE_REMAP         		   0x00
#define GCTRL_ENABLE_NIBBLE_REMAP          		   0x02
#define GCTRL_ENABLE_HORIZONTAL_ADDRESS_INCREMENT  0x00
#define GCTRL_ENABLE_VERTICAL_ADDRESS_INCREMENT    0x04
#define GCTRL_DISABLE_COM_REMAP        			   0x00
#define GCTRL_ENABLE_COM_REMAP       			   0x10
#define GCTRL_DISABLE_COM_SPLIT_ODD_EVEN 		   0x00
#define GCTRL_ENABLE_COM_SPLIT_ODD_EVEN       	   0x40

#define GCTRL_REMAP_MODE                           GCTRL_ENABLE_COM_SPLIT_ODD_EVEN | GCTRL_ENABLE_COM_REMAP | GCTRL_ENABLE_COLUMN_ADDRESS_REMAP

RamtexDisplayWidget* RamtexDisplayWidget::self = 0;

void sgfnc sgwrby(enum access_type paddr, SGUCHAR pval ) {
    RamtexDisplayWidget *instance = RamtexDisplayWidget::getInstance();
    switch (paddr) {
    case GHWWR:
        instance->emulateMPUDataWrite(pval);
        break;
    case GHWCMD:
        instance->emulateMPUIndexWrite(pval);
        break;
    default:
        Q_ASSERT(0); // unsupported operation
    }
}

SGUCHAR sgfnc sgrdby(enum access_type paddr ) {
    RamtexDisplayWidget *instance = RamtexDisplayWidget::getInstance();
    switch (paddr) {
    case GHWRD:
        return instance->emulateMPUDataRead();
    default:
        Q_ASSERT(0); // unsupported operation
    }
    return 0;
}

void ramtexdisplaywidget_init(void) {
    RamtexDisplayWidget::getInstance()->reset();
}

void ramtexdisplaywidget_deinit(void) {
    RamtexDisplayWidget::getInstance();
}

RamtexDisplayWidget::RamtexDisplayWidget(QWidget *parent) :
    QWidget(parent), ddram(GDISPW, GDISPH, QImage::Format_RGB16)
{
    Q_ASSERT(self == 0);
    self = this;

    ram.resize(GDISPW);
    for (int x = 0; x < GDISPW; x++)
        ram[x].resize(GDISPH);

    for (int x = 0; x < GDISPW; x++)
        for (int y = 0; y < GDISPH; y++)
        {
            ram[x][y] = 0;
            ddram.setPixel(x, y, qRgb(qrand()%255, qrand()%255, qrand()%255));
            //ddram.setPixel(x, y, 0xff);
        }
    reset();
    resize(GDISPW, GDISPH);



    regNames[REG_DISPLAY_OFF] = "REG_DISPLAY_OFF";
    regNames[REG_DISPLAY_ON] = "REG_DISPLAY_ON";
    regNames[REG_X] = "REG_X";
    regNames[REG_Y] = "REG_Y";
    regNames[REG_VP_X1] = "REG_VP_X1";
    regNames[REG_VP_X2] = "REG_VP_X2";
    regNames[REG_VP_Y1] = "REG_VP_Y1";
    regNames[REG_VP_Y2] = "REG_VP_Y2";

}

RamtexDisplayWidget::~RamtexDisplayWidget()
{
    Q_ASSERT(self != 0);
    self = 0;
}

RamtexDisplayWidget *RamtexDisplayWidget::getInstance()
{
    Q_ASSERT(self != 0);
    return self;
}

void RamtexDisplayWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    if (registers[REG_DISPLAY_ON] & 0x01) {
        painter.drawImage(0, 0, ddram);
    } else if (registers[REG_DISPLAY_OFF] & 0x01){
        painter.fillRect(0, 0, ddram.width(), ddram.height(), Qt::black);
    }
}

quint8 RamtexDisplayWidget::emulateMPUDataRead()
{
    mpu_control.ir = REG_DATA_RD;
    execReadCommand();
    return mpu_control.rdr;
}

void RamtexDisplayWidget::emulateMPUIndexWrite(quint8 value)
{
    mpu_control.wdr = value;
    mpu_control.ir = value;
    execWriteCommand();
}

void RamtexDisplayWidget::emulateMPUDataWrite(quint8 value)
{
    mpu_control.wdr = value;
    mpu_control.ir = REG_DATA_WR;
    execWriteCommand();
}

void RamtexDisplayWidget::execReadCommand()
{
    static uint8_t counter = 0;
    switch (mpu_control.ir) {
    case REG_DATA_RD:
        switch (counter)
        {
        case 0:
            mpu_control.rdr = 0; // dummy/invalid
            counter++;
            break;
        case 1:

         //   QRgb rgb = ddram.pixel(registers[REG_X],registers[REG_Y]);
         //      uint8_t leftNibble = QColor(rgb).black() ? Qt::black : Qt::yellow;
           //    rgb = ddram.pixel(registers[REG_X],registers[REG_Y]);
           //   uint8_t rightNibble = QColor(rgb).black() ? Qt::black : Qt::yellow;
           // uint8_t color;

            //color = *(getDDRAMPixelPointer());
                       //uint8_t leftNibble = color ? 0xF0 : 0;
             //  color = *(getDDRAMPixelPointer());
            //uint8_t rightNibble = color ? 0x0F : 0;
           // incrementDDRAMPosition();

            uint8_t leftNibble = ram[registers[REG_X]][registers[REG_Y]];
            incrementDDRAMPosition();
            uint8_t rightNibble = ram[registers[REG_X]][registers[REG_Y]];
            incrementDDRAMPosition();

            //registers[REG_X]-=2;
           // incrementDDRAMPosition();

            mpu_control.ddram_pixel = (leftNibble << 4) + rightNibble;
            mpu_control.rdr = mpu_control.ddram_pixel;

            break;
        }
        break;
//    default:
//        Q_ASSERT(registers.contains(mpu_control.ir));
//        mpu_control.rdr = registers.value(mpu_control.ir, 0);
//        break;
    }
}

void RamtexDisplayWidget::execWriteCommand()
{
    static bool isBigCmd = false;
    static uint8_t nextCmd = 0;
    static uint8_t counter = 0;

    if (isBigCmd)
        mpu_control.ir = nextCmd;

    switch (mpu_control.ir)
    {
    case REG_DATA_WR:
    {
        QColor blackColor = Qt::black;
        QColor yellowColor = Qt::yellow;
        QRgb rgbBlackCol = blackColor.rgb();
        QRgb rgbYellowCol = yellowColor.rgb();

         mpu_control.ddram_pixel = (mpu_control.wdr); // 2 pixels

        uint8_t leftNibble = mpu_control.ddram_pixel & 0xF0;
        leftNibble = leftNibble >> 4;
        uint8_t rightNibble = mpu_control.ddram_pixel & 0x0F;

        QRgb left = leftNibble ? rgbYellowCol : rgbBlackCol;
        QRgb right = rightNibble ? rgbYellowCol : rgbBlackCol;
           //ddram.setPixel(registers[REG_X], registers[REG_Y], left);
           // ddram.setPixel(registers[REG_X], registers[REG_Y], right);



//        *(getDDRAMPixelPointer()) = leftNibble ? 0xFF : 0x00;
//        incrementDDRAMPosition();

//        *(getDDRAMPixelPointer()) = rightNibble ? 0xFF : 0x00;
//        incrementDDRAMPosition();

        ram[registers[REG_X]][registers[REG_Y]] = leftNibble;
        ddram.setPixel(registers[REG_X], registers[REG_Y], left);
        incrementDDRAMPosition();

        ram[registers[REG_X]][registers[REG_Y]] = rightNibble;
        ddram.setPixel(registers[REG_X], registers[REG_Y], right);
        incrementDDRAMPosition();

        if (registers[REG_DISPLAY_ON] == 1)
            update();
        break;
    }
    case REG_DISPLAY_ON:
        qDebug() << "REG_DISPLAY_ON";
        registers[REG_DISPLAY_OFF] = 0;
        registers[REG_DISPLAY_ON] = 1;
        update();
        break;
    case REG_DISPLAY_OFF:
        qDebug() << "REG_DISPLAY_OFF";
        registers[REG_DISPLAY_ON] = 0;
        registers[REG_DISPLAY_OFF] = 1;
        update();
        break;
    case REG_COLUMN_ADDRESS:
         isBigCmd = true;
         nextCmd = REG_VP_X1;
         break;
    case REG_ROW_ADDRESS:
        isBigCmd = true;
        nextCmd = REG_VP_Y1;
         break;
    case REG_VP_X1:
    case REG_VP_X2:
        if (mpu_control.wdr > (GDISPW-1))
            mpu_control.wdr = (GDISPW-1);   

        saveMPUWriteRegister();
        checkAndAlignDDRAMPositionX();

        if (mpu_control.ir == REG_VP_X1)
            nextCmd = REG_VP_X2;
        if (mpu_control.ir == REG_VP_X2)
            isBigCmd = false;

        break;
    case REG_VP_Y1:
    case REG_VP_Y2:
        if (mpu_control.wdr > (GDISPH-1))
            mpu_control.wdr = (GDISPH-1);

        saveMPUWriteRegister();
        checkAndAlignDDRAMPositionY();

        if (mpu_control.ir == REG_VP_Y1)
            nextCmd = REG_VP_Y2;
        if (mpu_control.ir == REG_VP_Y2)
            isBigCmd = false;

        break;
//    case REG_X:
//        saveMPUWriteRegister();
//        checkAndAlignDDRAMPositionX();
//        break;
//    case REG_Y:
//        saveMPUWriteRegister();
//        checkAndAlignDDRAMPositionY();
//        break;
    case REG_GRAY_SCALE_TABLE:
        nextCmd = REG_GRAY_SCALE_TABLE;
        isBigCmd = true;
        if (counter == 0)
            counter = 1;
        if (counter > 0)
            counter++;
        if (counter == 17)
        {
            counter = 0;
            isBigCmd = false;
        }
        break;
    case REG_COMMAND_LOCK:
        nextCmd = REG_COMMAND_LOCK;
        isBigCmd = true;
        if (counter == 1)
        {
            counter = 0;
            isBigCmd = false;
            break;
        }
        if (counter == 0)
            counter = 1;
        break;
    case REG_DISPLAY_STAR_LINE:
        nextCmd = REG_DISPLAY_STAR_LINE;
        isBigCmd = true;
        if (counter == 1)
        {
            counter = 0;
            isBigCmd = false;
            break;
        }
        if (counter == 0)
            counter = 1;
        break;
    case REG_DISPLAY_OFFSET:
        nextCmd = REG_DISPLAY_OFFSET;
        isBigCmd = true;
        if (counter == 1)
        {
            counter = 0;
            isBigCmd = false;
            break;
        }
        if (counter == 0)
            counter = 1;
        break;
    case REG_DISPLAY_MODE_NORMAL:
        break;
    case REG_CONTRAST_CONTROL:
        nextCmd = REG_CONTRAST_CONTROL;
        isBigCmd = true;
        if (counter == 1)
        {
            counter = 0;
            isBigCmd = false;
            break;
        }
        if (counter == 0)
            counter = 1;
        break;
    case REG_REMAP:
        nextCmd = REG_REMAP;
        isBigCmd = true;
        if (counter == 1)
        {
            counter = 0;
            isBigCmd = false;
            break;
        }
        if (counter == 0)
            counter = 1;
        break;

//    case REG_DDRAM_DATA_ACCESS_PORT:
//        Q_ASSERT(registers[REG_MEMORY_WRITE_MODE] == 0x66);
//        mpu_control.ddram_pixel = (mpu_control.wdr << 8);
//        mpu_control.ir++;
//        break;
//    case REG_DDRAM_DATA_ACCESS_PORT+1:
//        mpu_control.ddram_pixel |= (mpu_control.wdr);
//        *(getDDRAMPixelPointer()) = mpu_control.ddram_pixel;
//        incrementDDRAMPosition();
//        mpu_control.ir = REG_DDRAM_DATA_ACCESS_PORT;
//        if (registers[REG_DISPLAY_ON] & 0x01)
//            update();
//        break;
    default:
        Q_ASSERT(registers[mpu_control.ir]);
        saveMPUWriteRegister();
        break;
    }
}

quint16* RamtexDisplayWidget::getDDRAMPixelPointer()
{
    return (quint16 *)(ddram.bits() + 2*(registers[REG_Y]*GDISPW + registers[REG_X]));
}

void RamtexDisplayWidget::incrementDDRAMPosition()
{
    if (registers[REG_VP_X1] > registers[REG_VP_X2])
        registers[REG_VP_X2] = registers[REG_VP_X1];

    if (registers[REG_VP_Y1] > registers[REG_VP_Y2])
        registers[REG_VP_Y2] = registers[REG_VP_Y1];

    registers[REG_X]++;
    registers[REG_DRAM_X] = registers[REG_X] / 2;

    if (registers[REG_DRAM_X] > registers[REG_VP_X2])
    {
        registers[REG_DRAM_X] = registers[REG_VP_X1];
        registers[REG_X] = registers[REG_DRAM_X];
        registers[REG_Y]++;
    }



//    if (registers[REG_DRAM_X] > registers[REG_VP_X2])
//    {
//        registers[REG_DRAM_X] = registers[REG_VP_X1];
//        registers[REG_Y]++;
//    }

    if (registers[REG_Y] > registers[REG_VP_Y2])
    {
        registers[REG_Y] = registers[REG_VP_Y1];
    }
    registers[REG_DRAM_Y] = registers[REG_Y];
}

void RamtexDisplayWidget::checkAndAlignDDRAMPositionX()
{
    if (!((registers[REG_VP_X1] <= registers[REG_DRAM_X]) &&
          (registers[REG_DRAM_X] <= registers[REG_VP_X2])))
    {
        registers[REG_DRAM_X] = registers[REG_VP_X1];
        //registers[REG_X] = registers[REG_DRAM_X] * 2;
    }
}

void RamtexDisplayWidget::checkAndAlignDDRAMPositionY()
{
    if (!((registers[REG_VP_Y1] <= registers[REG_DRAM_Y])&&
          (registers[REG_DRAM_Y] <= registers[REG_VP_Y2])))
    {
        registers[REG_DRAM_Y] = registers[REG_VP_Y1];
        registers[REG_Y] = registers[REG_DRAM_Y];
    }
}

void RamtexDisplayWidget::saveMPUWriteRegister()
{
    registers[mpu_control.ir] = mpu_control.wdr;
   // qDebug() << regNames[mpu_control.ir] << " = " << mpu_control.wdr;
}

void RamtexDisplayWidget::reset()
{
    mpu_control.ir = 0;

    registers[REG_DISPLAY_OFF] = 0x01;
    registers[REG_DISPLAY_ON] = 0x00;
    registers[REG_VP_X1] = 0x00;
    registers[REG_VP_Y1] = 0x00;
    registers[REG_VP_X2] = 0x3F;
    registers[REG_VP_Y2] = 0x7F;
    registers[REG_X] = 0x00;
    registers[REG_Y] = 0x00;
    registers[REG_DRAM_X] = 0x00;
    registers[REG_DRAM_Y] = 0x00;
}
