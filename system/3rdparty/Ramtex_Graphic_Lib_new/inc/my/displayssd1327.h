#ifndef DISPLAYSSD1327_H
#define DISPLAYSSD1327_H

#include "display.h"

#define sgwrby( PADDR, PVAL ) ( PADDR = ( PVAL ))

#define LCD_IO_BASE_ADDRESS	0x60000000

#define GHWWR  (* (uint8_t volatile *) ( LCD_IO_BASE_ADDRESS | 0x1 ))
#define GHWCMD (* (uint8_t volatile *) ( LCD_IO_BASE_ADDRESS ))

#define GCTRL_SET_COLUMN_ADDRESS       	  			0x15
#define GCTRL_SET_ROW_ADDRESS          	  			0x75
#define GCTRL_SET_CONTRAST_CONTROL     	  			0x81
#define GCTRL_SET_REMAP                	  			0xA0
#define GCTRL_SET_DISPLAY_START_LINE   	  			0xA1
#define GCTRL_SET_DISPLAY_OFFSET       	  			0xA2
#define GCTRL_SET_DISPLAY_MODE_NORMAL  	  			0xA4
#define GCTRL_SET_DISPLAY_MODE_ALL_ON  	  			0xA5
#define GCTRL_SET_DISPLAY_MODE_ALL_OFF 	  			0xA6
#define GCTRL_SET_DISPLAY_MODE_INVERSE 	  			0xA7
#define GCTRL_SET_MUX_RATIO            	  			0xA8
#define GCTRL_FUNCTION_SELECTION_A     	  			0xAB
#define GCTRL_SET_DISPLAY_OFF             			0xAE
#define GCTRL_SET_DISPLAY_ON          	  			0xAF
#define GCTRL_SET_PHASE_LENGTH         	  			0xB1
#define GCTRL_NOP                         			0xB2
#define GCTRL_SET_FRONT_CLK_DIV_OSC_FREQ  			0xB3
#define GCTRL_GPIO                        			0xB5
#define GCTRL_SET_SECOND_PRECHARGE_PERIOD 			0xB6
#define GCTRL_SET_FRAY_SCALE_TABLE        			0xB8
#define GCTRL_LINEAR_LUT                  			0xB9
#define GCTRL_NOP_B                    	 			0xBB
#define GCTRL_SET_PRECHARGE_VOLTAGE       			0xBC
#define GCTRL_SET_V_COMH                  			0xBE
#define GCTRL_FUNCTION_SELECTION_B     	  			0xD5
#define GCTRL_SET_COMMAND_LOCK         	  			0xFD
#define GCTRL_SET_RIGHT_HOR_SCROLL        			0x26
#define GCTRL_SET_RIGHT_VER_SCROLL        			0x27
#define GCTRL_DEACTIVATE_SCROLL           			0x2E // 26/27
#define GCTRL_ACTIVATE_SCROLL             			0x2F // 26/27

#define GCTRL_ENABLE_EXTERNAL_VDD_REGULATOR         0x00
#define GCTRL_ENABLE_INTERNAL_VDD_REGULATOR         0x01
#define GCTRL_REPRESENT_GPIO_PIN_HIZ_INPUT_DISABLE  0x00
#define GCTRL_REPRESENT_GPIO_PIN_HIZ_INPUT_ENABLE   0x01
#define GCTRL_REPRESENT_GPIO_PIN_OUTPUT_LOW         0x10
#define GCTRL_REPRESENT_GPIO_PIN_OUTPUT_HIGH        0x11

#define GCTRL_DISABLE_COLUMN_ADDRESS_REMAP 		    0b00000000 // default
#define GCTRL_ENABLE_COLUMN_ADDRESS_REMAP  		    0b00000001
#define GCTRL_DISABLE_NIBBLE_REMAP         		    0b00000000 // default
#define GCTRL_ENABLE_NIBBLE_REMAP          		    0b00000010
#define GCTRL_ENABLE_HORIZONTAL_ADDRESS_INCREMENT   0b00000000 // default
#define GCTRL_ENABLE_VERTICAL_ADDRESS_INCREMENT     0b00000100
#define GCTRL_DISABLE_COM_REMAP        			    0b00000000 // default
#define GCTRL_ENABLE_COM_REMAP       			    0b00010000
#define GCTRL_DISABLE_COM_SPLIT_ODD_EVEN 		    0b00000000 // default
#define GCTRL_ENABLE_COM_SPLIT_ODD_EVEN       	    0b01000000

#define GCTRL_REMAP_MODE                            GCTRL_ENABLE_COM_SPLIT_ODD_EVEN |    \
													GCTRL_ENABLE_COM_REMAP |             \
													GCTRL_ENABLE_COLUMN_ADDRESS_REMAP |  \
													GCTRL_ENABLE_HORIZONTAL_ADDRESS_INCREMENT

#define GCTRL_H_INC_MODE GCTRL_REMAP_MODE
#define GCTRL_V_INC_MODE (GCTRL_REMAP_MODE & 0b00000100)

class DisplaySsd1327 : public Display
{
public:

    DisplaySsd1327();
    bool init();
    void enable(){writeCmd(GCTRL_SET_DISPLAY_ON);}
    void disable(){writeCmd(GCTRL_SET_DISPLAY_OFF);}
    //bool reset();
    GColor read(uint16_t x, uint16_t y){return buffer[x][y];}
    void update();

    void autoWrite(GColor color);

    void writeCmd(uint8_t cmd);
   // void writeData(uint8_t data);
  //  void writeData(uint8_t data1, uint8_t data2);
    void writeCmdData(uint8_t cmd, uint8_t data);
    void writeCmdData(uint8_t cmd, uint8_t data1, uint8_t data2);

    void sendData(uint8_t byte, bool isData);
    void setViewPort(RectArea area);
    uint32_t delay(uint32_t count);

    void setHautoInc(){ writeCmdData(GCTRL_SET_REMAP, GCTRL_H_INC_MODE); } // set horizontal auto increment
    void setVautoInc(){ writeCmdData(GCTRL_SET_REMAP, GCTRL_V_INC_MODE); }

protected:

    GColor buffer[64][128];
};

#endif // DISPLAYSSD1327_H
