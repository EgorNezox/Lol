#include "displayssd1327.h"
#include "stdlib.h"
#include "system_hw_io.h"

DisplaySsd1327::DisplaySsd1327()
{
    width = 128;
    height = 128;
    bitPerPixel = 4;

    for (uint16_t j = 0; j < height; j++)
    for (uint16_t i = 0; i < width/2; i++)
    {
		 //buffer[i][j] = 0xAAAA;
		 //buffer[i][j] = 0xFFFF;
		 buffer[i][j] = 0x0000;
    }

    viewPort = {0, 0, width-1, height-1};
}

uint32_t DisplaySsd1327::delay(uint32_t count)
{
	volatile uint32_t w;
	for(uint32_t i = 0; i < count; i++)
	{
		w++;
	}
	return w;
	//for (uint8_t c = 0; c < count; c++)
	// writeCmd(GCTRL_NOP);
}

bool DisplaySsd1327::init()
{
     isError = false;

    stm32f2_LCD_init();

    if (isError)
          return 1;

    writeCmdData(GCTRL_FUNCTION_SELECTION_A,        GCTRL_ENABLE_INTERNAL_VDD_REGULATOR ); /* enable vdd regulator */
    writeCmdData(GCTRL_SET_FRONT_CLK_DIV_OSC_FREQ,  0xF1 ); /* set front clock divider */
    writeCmdData(GCTRL_GPIO,                        GCTRL_REPRESENT_GPIO_PIN_HIZ_INPUT_DISABLE); /* gpio pin represents */
    writeCmdData(GCTRL_SET_SECOND_PRECHARGE_PERIOD, 0x04 );

    for(int i = 0; i<100; i++) {}

    writeCmdData(GCTRL_SET_REMAP,                   GCTRL_REMAP_MODE);    /* remap ghraph display data */ //51
    writeCmdData(GCTRL_SET_MUX_RATIO,               0x7F ); /* set multiplex ratio  */
    writeCmdData(GCTRL_SET_PHASE_LENGTH,            0x4A );
    writeCmdData(GCTRL_SET_PRECHARGE_VOLTAGE,       0x08 );   /* vcc */
    writeCmd(GCTRL_SET_DISPLAY_MODE_NORMAL);        // normal mode

    enable();

    //ghw_updatehw();  /* Flush to display hdw or simulator */

    return isError;
}

// GColor - передавать значение в младшем nibble
void DisplaySsd1327::autoWrite(GColor color)
{
    uint8_t nibble[2] = {0xF0, 0x0F};

    for (uint16_t j = viewPort.ly; j <= viewPort.ry; j++)
    for (uint16_t i = viewPort.lx; i <= viewPort.rx; i++)
    {
        uint8_t nibbleNum = div(i, 2).rem;
        uint8_t tmp = buffer[div(i, 2).quot][j] | (nibble[!nibbleNum]); // сохраняем пиксель, который не должны трогать
        buffer[div(i, 2).quot][j] = tmp | (color << (!nibbleNum * 4) ) ; // присваиваем паре пикселей : сохраненный пиксель с новым пикселем
    }
}

void DisplaySsd1327::update()
{
    uint16_t lx = div(viewPort.lx, 2).quot;
    uint16_t rx = div(viewPort.rx, 2).quot;

    for (uint16_t j = viewPort.ly; j <= viewPort.ry; j++)
    for (uint16_t i = lx; i <= rx; i++)
    {
    	sendData(buffer[i][j], 1);
    }
}

void DisplaySsd1327::writeCmd(uint8_t cmd)
{
    sendData(cmd, 0);
}

void DisplaySsd1327::writeCmdData(uint8_t cmd, uint8_t data)
{
    sendData(cmd, 0);
    sendData(data, 0);
}

void DisplaySsd1327::writeCmdData(uint8_t cmd, uint8_t data1, uint8_t data2)
{
    sendData(cmd, 0);
    sendData(data1, 0);
    sendData(data2, 0);
}

void DisplaySsd1327::setViewPort(RectArea area)
{ 
    viewPort = area;

    // struct div_t
    //int quot; /* частное */
    //int rem; /* остаток */

    // column range = 0..63  (0x00 - 0x3F)
    // row range    = 0..127 (0x00 - 0x7F)
    // div x ->    0,1 -> 0  2,3 -> 1, 4,5 -> 2 ...

    uint16_t lx = div(viewPort.lx, 2).quot;
    uint16_t rx = div(viewPort.rx, 2).quot ;
    uint16_t ly = viewPort.ly;
    uint16_t ry = viewPort.ry;

    writeCmdData(GCTRL_SET_COLUMN_ADDRESS, lx, rx);
    writeCmdData(GCTRL_SET_ROW_ADDRESS,    ly, ry );
}

void DisplaySsd1327::sendData(uint8_t data, bool isData)
{
#ifdef DISPLAY_BUS

	if (isData)
		sgwrby(GHWWR, data);
	else
		sgwrby(GHWCMD, data);

	delay(60);

#else

	sendPinDataToLcd(data, isData);

#endif
}



