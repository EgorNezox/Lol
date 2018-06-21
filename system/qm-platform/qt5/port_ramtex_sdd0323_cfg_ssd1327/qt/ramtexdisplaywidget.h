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
#include <qdebug.h>
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
        REG_X                       = 0x02, // added to simulation
        REG_Y                       = 0x03, // added to simulation
        REG_DRAM_X                  = 0x04, // added to simulation
        REG_DRAM_Y                  = 0x05, // added to simulation
        REG_VP_X1                   = 0x06, // viewport X LEFT added to simulation
        REG_VP_Y1                   = 0x07, // viewport Y TOP added to simulation
        REG_VP_X2                   = 0x08, // viewport X RIGHT added to simulation
        REG_VP_Y2                   = 0x09, // viewport X BOTTOM added to simulation
        REG_DATA_WR                 = 0x10, // added to simulation
        REG_DATA_RD                 = 0x11, // added to simulation
        REG_COLUMN_ADDRESS          = 0x15,
        REG_RIGHT_HOR_SCROLL        = 0x26,
        REG_RIGHT_VER_SCROLL        = 0x27,
        REG_DEACTIVATE_SCROLL       = 0x2E,
        REG_ACTIVATE_SCROLL         = 0x2F,
        REG_ROW_ADDRESS             = 0x75,
        REG_CONTRAST_CONTROL        = 0x81,
        REG_REMAP                   = 0xA0,
        REG_DISPLAY_STAR_LINE       = 0xA1,
        REG_DISPLAY_OFFSET          = 0xA2,
        REG_DISPLAY_MODE_NORMAL     = 0xA4,
        REG_DISPLAY_MODE_ALL_ON     = 0xA5,
        REG_DISPLAY_MODE_ALL_OFF    = 0xA6,
        REG_DISPLAY_MODE_INVERSE    = 0xA7,
        REG_MUX_RATIO               = 0xA8,
        REG_FUNCTION_SELECTION_A    = 0xAB,
        REG_DISPLAY_OFF             = 0xAE,
        REG_DISPLAY_ON              = 0xAF,
        REG_PHASE_LENGTH            = 0xB1,
        REG_NOP                     = 0xB2,
        REG_FRONT_CLK_DIV_OSC_FREQ  = 0xB3,
        REG_GPIO                    = 0xB5,
        REG_SECOND_PRECHARGE_PERIOD = 0xB6,
        REG_GRAY_SCALE_TABLE        = 0xB8,
        REG_LINEAR_LUT              = 0xB9,
      //REG_NOP                     = 0xBB,
        REG_PRECHARGE_VOLTAGE       = 0xBC,
        REG_V_COMH                  = 0xBE,
        REG_FUNCTION_SELECTION_B    = 0xD5,
        REG_COMMAND_LOCK            = 0xFD
    };
    struct {
        quint8 ir, rdr, wdr;
        quint16 ddram_pixel;
    } mpu_control;
   // QMap<quint8, quint8> registers;
    quint8 registers[256];
    QString regNames[256];
    QVector <QVector<uint8_t>> ram;
    QImage ddram;
};

#endif /* __cplusplus */

#endif // RAMTEXDISPLAYWIDGET_H
