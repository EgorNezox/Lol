/**
  ******************************************************************************
  * @file    ramtexdisplaywidget.cpp
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  * @brief   Реализация класса виджета Qt, симулирующего дисплей LCD с контроллером SEPS525
  *
  ******************************************************************************
  */

#include "gdispcfg.h"
#include <qpainter.h>

#include "ramtexdisplaywidget.h"

#ifndef GHW_SEPS525
#error "Ramtex display driver should be configured for SEPS525 controller"
#endif
#if !(GDISPPIXW == 16)
#error "Ramtex dsiplay configuration: only 16-bit pixel mode supported"
#endif

RamtexDisplayWidget* RamtexDisplayWidget::self = 0;

void sgfnc sgwrby( enum access_type paddr, SGUCHAR pval ) {
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

SGUCHAR sgfnc sgrdby( enum access_type paddr ) {
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
    for (int x = 0; x < GDISPW; x++)
        for (int y = 0; y < GDISPH; y++)
            ddram.setPixel(x, y, qRgb(qrand()%255, qrand()%255, qrand()%255));
    reset();
    resize(GDISPW, GDISPH);
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
    if (registers[REG_DISP_ON_OFF] & 0x01) {
        painter.drawImage(0, 0, ddram);
    } else {
        painter.fillRect(0, 0, ddram.width(), ddram.height(), Qt::black);
    }
}

quint8 RamtexDisplayWidget::emulateMPUDataRead()
{
    execReadCommand();
    return mpu_control.rdr;
}

void RamtexDisplayWidget::emulateMPUIndexWrite(quint8 value)
{
    mpu_control.ir = value;
}

void RamtexDisplayWidget::emulateMPUDataWrite(quint8 value)
{
    mpu_control.wdr = value;
    execWriteCommand();
}

void RamtexDisplayWidget::execReadCommand()
{
    switch (mpu_control.ir) {
    case REG_DDRAM_DATA_ACCESS_PORT:
        Q_ASSERT(registers[REG_MEMORY_WRITE_MODE] == 0x66);
        mpu_control.ddram_pixel = *(getDDRAMPixelPointer());
        mpu_control.rdr = 0; // dummy/invalid
        mpu_control.ir++;
        break;
    case REG_DDRAM_DATA_ACCESS_PORT+1:
        mpu_control.rdr = (mpu_control.ddram_pixel >> 8) & 0xFF;
        mpu_control.ir++;
        break;
    case REG_DDRAM_DATA_ACCESS_PORT+2:
        mpu_control.rdr = mpu_control.ddram_pixel & 0xFF;
        incrementDDRAMPosition();
        mpu_control.ir = REG_DDRAM_DATA_ACCESS_PORT;
        break;
    default:
        Q_ASSERT(registers.contains(mpu_control.ir));
        mpu_control.rdr = registers.value(mpu_control.ir, 0);
        break;
    }
}

void RamtexDisplayWidget::execWriteCommand()
{
    switch (mpu_control.ir) {
    case REG_INDEX:
    case REG_STATUS_RD:
        Q_ASSERT(0); // read-only
        break;
    case REG_SOFT_RST:
        if (mpu_control.wdr & 0x01) {
            reset();
            update();
        }
        break;
    case REG_DISP_ON_OFF:
        saveMPUWriteRegister();
        update();
        break;
    case REG_DISPLAY_MODE_SET:
        Q_ASSERT(mpu_control.wdr == 0x00); // only specific setting supported
        saveMPUWriteRegister();
        break;
    case REG_RGB_IF:
        Q_ASSERT(mpu_control.wdr == 0x11); // only specific setting supported
        saveMPUWriteRegister();
        break;
    case REG_MEMORY_WRITE_MODE:
        Q_ASSERT(mpu_control.wdr == 0x66); // only specific setting supported
        saveMPUWriteRegister();
        break;
    case REG_MX1_ADDR:
    case REG_MX2_ADDR:
        if (mpu_control.wdr > (GDISPW-1))
            mpu_control.wdr = (GDISPW-1);
        saveMPUWriteRegister();
        checkAndAlignDDRAMPositionX();
        break;
    case REG_MY1_ADDR:
    case REG_MY2_ADDR:
        if (mpu_control.wdr > (GDISPH-1))
            mpu_control.wdr = (GDISPH-1);
        saveMPUWriteRegister();
        checkAndAlignDDRAMPositionY();
        break;
    case REG_MEMORY_ACCESS_POINTER_X:
        saveMPUWriteRegister();
        checkAndAlignDDRAMPositionX();
        break;
    case REG_MEMORY_ACCESS_POINTER_Y:
        saveMPUWriteRegister();
        checkAndAlignDDRAMPositionY();
        break;
    case REG_DDRAM_DATA_ACCESS_PORT:
        Q_ASSERT(registers[REG_MEMORY_WRITE_MODE] == 0x66);
        mpu_control.ddram_pixel = (mpu_control.wdr << 8);
        mpu_control.ir++;
        break;
    case REG_DDRAM_DATA_ACCESS_PORT+1:
        mpu_control.ddram_pixel |= (mpu_control.wdr);
        *(getDDRAMPixelPointer()) = mpu_control.ddram_pixel;
        incrementDDRAMPosition();
        mpu_control.ir = REG_DDRAM_DATA_ACCESS_PORT;
        if (registers[REG_DISP_ON_OFF] & 0x01)
            update();
        break;
    case REG_DUTY:
        Q_ASSERT(mpu_control.wdr == 0x7F); // only specific setting supported
        saveMPUWriteRegister();
        break;
    case REG_DSL:
        Q_ASSERT(mpu_control.wdr == 0x00); // only specific setting supported
        saveMPUWriteRegister();
        break;
    case REG_D1_DDRAM_FAC:
    case REG_D1_DDRAM_FAR:
    case REG_D2_DDRAM_SAC:
    case REG_D2_DDRAM_SAR:
    case REG_SCR1_FX1:
    case REG_SCR1_FY1:
    case REG_SCR2_SX1:
    case REG_SCR2_SY1:
        Q_ASSERT(mpu_control.wdr == 0x00); // only specific setting supported
        saveMPUWriteRegister();
        break;
    case REG_SCR1_FX2:
    case REG_SCR2_SX2:
        Q_ASSERT(mpu_control.wdr == 0x9F); // only specific setting supported
        saveMPUWriteRegister();
        break;
    case REG_SCR1_FY2:
    case REG_SCR2_SY2:
        Q_ASSERT(mpu_control.wdr == 0x7F); // only specific setting supported
        saveMPUWriteRegister();
        break;
    default:
        Q_ASSERT(registers.contains(mpu_control.ir));
        saveMPUWriteRegister();
        break;
    }
}

quint16* RamtexDisplayWidget::getDDRAMPixelPointer()
{
    return (quint16 *)(ddram.bits() + 2*(registers[REG_MEMORY_ACCESS_POINTER_Y]*GDISPW + registers[REG_MEMORY_ACCESS_POINTER_X]));
}

void RamtexDisplayWidget::incrementDDRAMPosition()
{
    if (!(registers[REG_MX1_ADDR] <= registers[REG_MX2_ADDR]))
        registers[REG_MX2_ADDR] = registers[REG_MX1_ADDR];
    if (!(registers[REG_MY1_ADDR] <= registers[REG_MY2_ADDR]))
        registers[REG_MY2_ADDR] = registers[REG_MY1_ADDR];
    registers[REG_MEMORY_ACCESS_POINTER_X]++;
    if (registers[REG_MEMORY_ACCESS_POINTER_X] > registers[REG_MX2_ADDR]) {
        registers[REG_MEMORY_ACCESS_POINTER_X] = registers[REG_MX1_ADDR];
        registers[REG_MEMORY_ACCESS_POINTER_Y]++;
    }
    if (registers[REG_MEMORY_ACCESS_POINTER_Y] > registers[REG_MY2_ADDR]) {
        registers[REG_MEMORY_ACCESS_POINTER_Y] = registers[REG_MY1_ADDR];
    }
}

void RamtexDisplayWidget::checkAndAlignDDRAMPositionX()
{
    if (!((registers[REG_MX1_ADDR] <= registers[REG_MEMORY_ACCESS_POINTER_X])
          && (registers[REG_MEMORY_ACCESS_POINTER_X] <= registers[REG_MX2_ADDR])))
        registers[REG_MEMORY_ACCESS_POINTER_X] = registers[REG_MX1_ADDR];
}

void RamtexDisplayWidget::checkAndAlignDDRAMPositionY()
{
    if (!((registers[REG_MY1_ADDR] <= registers[REG_MEMORY_ACCESS_POINTER_Y])
          && (registers[REG_MEMORY_ACCESS_POINTER_Y] <= registers[REG_MY2_ADDR])))
        registers[REG_MEMORY_ACCESS_POINTER_Y] = registers[REG_MY1_ADDR];
}

void RamtexDisplayWidget::saveMPUWriteRegister()
{
    registers[mpu_control.ir] = mpu_control.wdr;
}

void RamtexDisplayWidget::reset()
{
    mpu_control.ir = 0;
    registers[REG_INDEX] = 0x00;
    registers[REG_STATUS_RD] = 0xC0;
    registers[REG_OSC_CTL] = 0xC0;
    registers[REG_CLOCK_DIV] = 0x30;
    registers[REG_REDUCE_CURRENT] = 0x00;
    registers[REG_SOFT_RST] = 0x00;
    registers[REG_DISP_ON_OFF] = 0x00;
    registers[REG_PRECHARGE_TIME_R] = 0x00;
    registers[REG_PRECHARGE_TIME_G] = 0x00;
    registers[REG_PRECHARGE_TIME_B] = 0x00;
    registers[REG_PRECHARGE_CURRENT_R] = 0x00;
    registers[REG_PRECHARGE_CURRENT_G] = 0x00;
    registers[REG_PRECHARGE_CURRENT_B] = 0x00;
    registers[REG_DRIVING_CURRENT_R] = 0x00;
    registers[REG_DRIVING_CURRENT_G] = 0x00;
    registers[REG_DRIVING_CURRENT_B] = 0x00;
    registers[REG_DISPLAY_MODE_SET] = 0x00;
    registers[REG_RGB_IF] = 0x11;
    registers[REG_RGB_POL] = 0x00;
    registers[REG_MEMORY_WRITE_MODE] = 0x06;
    registers[REG_MX1_ADDR] = 0x00;
    registers[REG_MX2_ADDR] = 0x9F;
    registers[REG_MY1_ADDR] = 0x00;
    registers[REG_MY2_ADDR] = 0x7F;
    registers[REG_MEMORY_ACCESS_POINTER_X] = 0x00;
    registers[REG_MEMORY_ACCESS_POINTER_Y] = 0x00;
    registers[REG_GRAY_SCALE_TABLE_INDEX] = 0x00;
    registers[REG_DUTY] = 0x7F;
    registers[REG_DSL] = 0x00;
    registers[REG_D1_DDRAM_FAC] = 0x00;
    registers[REG_D1_DDRAM_FAR] = 0x00;
    registers[REG_D2_DDRAM_SAC] = 0x00;
    registers[REG_D2_DDRAM_SAR] = 0x00;
    registers[REG_SCR1_FX1] = 0x00;
    registers[REG_SCR1_FX2] = 0x9F;
    registers[REG_SCR1_FY1] = 0x00;
    registers[REG_SCR1_FY2] = 0x7F;
    registers[REG_SCR2_SX1] = 0x00;
    registers[REG_SCR2_SX2] = 0x9F;
    registers[REG_SCR2_SY1] = 0x00;
    registers[REG_SCR2_SY2] = 0x7F;
    registers[REG_SCREEN_SAVER_CONTROL] = 0x00;
    registers[REG_SS_SLEEP_TIMER] = 0x00;
    registers[REG_SCREEN_SAVER_MODE] = 0x00;
    registers[REG_SS_SCR1_FU] = 0x00;
    registers[REG_SS_SCR1_MXY] = 0x00;
    registers[REG_SS_SCR2_FU] = 0x00;
    registers[REG_SS_SCR2_MXY] = 0x00;
    registers[REG_MOVING_DIRECTION] = 0x00;
    registers[REG_SS_SCR2_SX1] = 0x00;
    registers[REG_SS_SCR2_SX2] = 0x00;
    registers[REG_SS_SCR2_SY1] = 0x00;
    registers[REG_SS_SCR2_SY2] = 0x00;
    registers[REG_IREF] = 0x00; // expected action on undocumented register
}
