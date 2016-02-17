/**
  ******************************************************************************
  * @file    ramtexdisplaywidget.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    26.10.2015
  * @brief   Интерфейс класса виджета Qt, симулирующего дисплей LCD с контроллером SEPS525
  *
  ******************************************************************************
  */

#ifndef RAMTEXDISPLAYWIDGET_H
#define RAMTEXDISPLAYWIDGET_H

#ifdef __cplusplus
#include <qwidget.h>
#include <qmap.h>
#include <qimage.h>
#include "sgio.h"
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif
void ramtexdisplaywidget_init(void);
void ramtexdisplaywidget_deinit(void);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

class RamtexDisplayWidget : public QWidget
{
    Q_OBJECT

public:
    RamtexDisplayWidget(QWidget *parent = 0);
    ~RamtexDisplayWidget();

private:
    friend void ramtexdisplaywidget_init(void);
    friend void ramtexdisplaywidget_deinit(void);
    friend void sgfnc sgwrby( enum access_type paddr, SGUCHAR pval );
    friend SGUCHAR sgfnc sgrdby( enum access_type paddr );

    static RamtexDisplayWidget *getInstance();
    void paintEvent(QPaintEvent *);
    quint8 emulateMPUDataRead();
    void emulateMPUIndexWrite(quint8 value);
    void emulateMPUDataWrite(quint8 value);
    void execReadCommand();
    void execWriteCommand();
    quint16 *getDDRAMPixelPointer();
    void incrementDDRAMPosition();
    void checkAndAlignDDRAMPositionX();
    void checkAndAlignDDRAMPositionY();
    void saveMPUWriteRegister();
    void reset();

    static RamtexDisplayWidget *self;
    enum mpu_reg_addr {
        REG_INDEX = 0x00,
        REG_STATUS_RD = 0x01,
        REG_OSC_CTL = 0x02,
        REG_CLOCK_DIV = 0x03,
        REG_REDUCE_CURRENT = 0x04,
        REG_SOFT_RST = 0x05,
        REG_DISP_ON_OFF = 0x06,
        REG_PRECHARGE_TIME_R = 0x08,
        REG_PRECHARGE_TIME_G = 0x09,
        REG_PRECHARGE_TIME_B = 0x0A,
        REG_PRECHARGE_CURRENT_R = 0x0B,
        REG_PRECHARGE_CURRENT_G = 0x0C,
        REG_PRECHARGE_CURRENT_B = 0x0D,
        REG_DRIVING_CURRENT_R = 0x10,
        REG_DRIVING_CURRENT_G = 0x11,
        REG_DRIVING_CURRENT_B = 0x12,
        REG_DISPLAY_MODE_SET = 0x13,
        REG_RGB_IF = 0x14,
        REG_RGB_POL = 0x15,
        REG_MEMORY_WRITE_MODE = 0x16,
        REG_MX1_ADDR = 0x17,
        REG_MX2_ADDR = 0x18,
        REG_MY1_ADDR = 0x19,
        REG_MY2_ADDR = 0x1A,
        REG_MEMORY_ACCESS_POINTER_X = 0x20,
        REG_MEMORY_ACCESS_POINTER_Y = 0x21,
        REG_DDRAM_DATA_ACCESS_PORT = 0x22,
        REG_GRAY_SCALE_TABLE_INDEX = 0x50,
        REG_GRAY_SCALE_TABLE_DATA = 0x51,
        REG_DUTY = 0x28,
        REG_DSL = 0x29,
        REG_D1_DDRAM_FAC = 0x2E,
        REG_D1_DDRAM_FAR = 0x2F,
        REG_D2_DDRAM_SAC = 0x31,
        REG_D2_DDRAM_SAR = 0x32,
        REG_SCR1_FX1 = 0x33,
        REG_SCR1_FX2 = 0x34,
        REG_SCR1_FY1 = 0x35,
        REG_SCR1_FY2 = 0x36,
        REG_SCR2_SX1 = 0x37,
        REG_SCR2_SX2 = 0x38,
        REG_SCR2_SY1 = 0x39,
        REG_SCR2_SY2 = 0x3A,
        REG_SCREEN_SAVER_CONTROL = 0x3B,
        REG_SS_SLEEP_TIMER = 0x3C,
        REG_SCREEN_SAVER_MODE = 0x3D,
        REG_SS_SCR1_FU = 0x3E,
        REG_SS_SCR1_MXY = 0x3F,
        REG_SS_SCR2_FU = 0x40,
        REG_SS_SCR2_MXY = 0x41,
        REG_MOVING_DIRECTION = 0x42,
        REG_SS_SCR2_SX1 = 0x47,
        REG_SS_SCR2_SX2 = 0x48,
        REG_SS_SCR2_SY1 = 0x49,
        REG_SS_SCR2_SY2 = 0x4A,
        REG_IREF = 0x80 // undocummented register (PDAC, DDAC, Reference Volt.control)
    };
    struct {
        quint8 ir, rdr, wdr;
        quint16 ddram_pixel;
    } mpu_control;
    QMap<quint8, quint8> registers;
    QImage ddram;
};

#endif /* __cplusplus */

#endif // RAMTEXDISPLAYWIDGET_H
