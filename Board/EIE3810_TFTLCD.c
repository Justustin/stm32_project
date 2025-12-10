#include "stm32f10x.h"
#include "EIE3810_TFTLCD.h"
#include "Font.H"

extern void Delay(u32 count);

/*
 * =========================================================================
 * ALIENTEK 4.3" TFT LCD CONFIGURATION
 * =========================================================================
 * Orientation:    PORTRAIT (480x800)
 * Solution:       Updated for larger 4.3" display.
 * =========================================================================
 */

#define LCD_WIDTH   480
#define LCD_HEIGHT  800

// FSMC Base Address (Bank1 Sector4, A10 as Register Select)
#define LCD_BASE        ((u32)(0x6C000000 | 0x000007FE))
#define LCD             ((LCD_TypeDef *) LCD_BASE)

typedef struct
{
	volatile u16 LCD_REG;
	volatile u16 LCD_RAM;
} LCD_TypeDef;

/* =========================================================================
 * LOW LEVEL FUNCTIONS
 * ========================================================================= */

void EIE3810_TFTLCD_WrCmd(u16 cmd)
{
	LCD->LCD_REG = cmd;
}

void EIE3810_TFTLCD_WrData(u16 data)
{
	LCD->LCD_RAM = data;
}

void EIE3810_TFTLCD_WrCmdData(u16 cmd, u16 data)
{
	LCD->LCD_REG = cmd;
	LCD->LCD_RAM = data;
}

void EIE3810_TFTLCD_SetWindow(u16 xStar, u16 yStar, u16 xEnd, u16 yEnd)
{
	// 4.3" LCD uses split command format
	EIE3810_TFTLCD_WrCmd(0x2A00); EIE3810_TFTLCD_WrData(xStar >> 8);
	EIE3810_TFTLCD_WrCmd(0x2A01); EIE3810_TFTLCD_WrData(xStar & 0xFF);
	EIE3810_TFTLCD_WrCmd(0x2A02); EIE3810_TFTLCD_WrData(xEnd >> 8);
	EIE3810_TFTLCD_WrCmd(0x2A03); EIE3810_TFTLCD_WrData(xEnd & 0xFF);

	EIE3810_TFTLCD_WrCmd(0x2B00); EIE3810_TFTLCD_WrData(yStar >> 8);
	EIE3810_TFTLCD_WrCmd(0x2B01); EIE3810_TFTLCD_WrData(yStar & 0xFF);
	EIE3810_TFTLCD_WrCmd(0x2B02); EIE3810_TFTLCD_WrData(yEnd >> 8);
	EIE3810_TFTLCD_WrCmd(0x2B03); EIE3810_TFTLCD_WrData(yEnd & 0xFF);

	EIE3810_TFTLCD_WrCmd(0x2C00); // Write GRAM Start
}

/* =========================================================================
 * DRAWING FUNCTIONS
 * ========================================================================= */

void EIE3810_TFTLCD_DrawDot(u16 x, u16 y, u16 color)
{
    if(x >= LCD_WIDTH || y >= LCD_HEIGHT) return;
	EIE3810_TFTLCD_SetWindow(x, y, x, y);
	EIE3810_TFTLCD_WrData(color);
}

void EIE3810_TFTLCD_FillRectangle(u16 start_x, u16 length_x, u16 start_y, u16 length_y, u16 color)
{
	u32 index = 0;
	u32 total_pixels;
    u16 end_x, end_y;

    if(start_x >= LCD_WIDTH || start_y >= LCD_HEIGHT) return;

    end_x = start_x + length_x - 1;
    end_y = start_y + length_y - 1;

    if(end_x >= LCD_WIDTH)  end_x = LCD_WIDTH - 1;
    if(end_y >= LCD_HEIGHT) end_y = LCD_HEIGHT - 1;

    if(end_x < start_x || end_y < start_y) return;

    total_pixels = (end_x - start_x + 1) * (end_y - start_y + 1);

	EIE3810_TFTLCD_SetWindow(start_x, start_y, end_x, end_y);

	for(index = 0; index < total_pixels; index++){
		LCD->LCD_RAM = color;
	}
}

void EIE3810_TFTLCD_Clear(u16 color)
{
	EIE3810_TFTLCD_FillRectangle(0, LCD_WIDTH, 0, LCD_HEIGHT, color);
}

void EIE3810_TFTLCD_DrawCircle(u16 x0, u16 y0, u8 r, u8 full, u16 color)
{
	int a, b;
	int di, yi;
	a = 0;
	b = r;
	di = 3 - (r << 1);

	if(!full) // Hollow
	{
		while(a <= b)
		{
			EIE3810_TFTLCD_DrawDot(x0 + a, y0 - b, color);
			EIE3810_TFTLCD_DrawDot(x0 + b, y0 - a, color);
			EIE3810_TFTLCD_DrawDot(x0 + b, y0 + a, color);
			EIE3810_TFTLCD_DrawDot(x0 + a, y0 + b, color);
			EIE3810_TFTLCD_DrawDot(x0 - a, y0 + b, color);
			EIE3810_TFTLCD_DrawDot(x0 - b, y0 + a, color);
			EIE3810_TFTLCD_DrawDot(x0 - a, y0 - b, color);
			EIE3810_TFTLCD_DrawDot(x0 - b, y0 - a, color);
			a++;
			if(di < 0) di += 4 * a + 6;
			else { di += 10 + 4 * (a - b); b--; }
		}
	}
	else // Filled
	{
		while(a <= b)
		{
			for(yi = a; yi <= b; yi++)
			{
				EIE3810_TFTLCD_DrawDot(x0 + a, y0 - yi, color);
				EIE3810_TFTLCD_DrawDot(x0 + yi, y0 - a, color);
				EIE3810_TFTLCD_DrawDot(x0 + yi, y0 + a, color);
				EIE3810_TFTLCD_DrawDot(x0 + a, y0 + yi, color);
				EIE3810_TFTLCD_DrawDot(x0 - a, y0 + yi, color);
				EIE3810_TFTLCD_DrawDot(x0 - yi, y0 + a, color);
				EIE3810_TFTLCD_DrawDot(x0 - a, y0 - yi, color);
				EIE3810_TFTLCD_DrawDot(x0 - yi, y0 - a, color);
			}
			a++;
			if(di < 0) di += 4 * a + 6;
			else { di += 10 + 4 * (a - b); b--; }
		}
	}
}

void EIE3810_TFTLCD_DrawRectangle(u16 start_x, u16 start_y, u16 width, u16 height, u16 color)
{
	EIE3810_TFTLCD_FillRectangle(start_x, width, start_y, 1, color);
	EIE3810_TFTLCD_FillRectangle(start_x, width, start_y + height - 1, 1, color);
	EIE3810_TFTLCD_FillRectangle(start_x, 1, start_y, height, color);
	EIE3810_TFTLCD_FillRectangle(start_x + width - 1, 1, start_y, height, color);
}

void EIE3810_TFTLCD_FillScreen(u16 color)
{
	EIE3810_TFTLCD_Clear(color);
}

/* =========================================================================
 * FONT FUNCTIONS - 1608 (Smaller) and 2412 (Larger)
 * ========================================================================= */

// Show Single Char 16x8 (Small)
void EIE3810_TFTLCD_ShowChar(u16 x, u16 y, u8 ASCII, u16 color, u16 bgcolor)
{
    u8 i,j,k;
    if(ASCII <32 || ASCII == 127) return;

    for(i=0;i<8;i++){
        for(j=0;j<8;j++){
            if((asc2_1608[ASCII-32][2*i]>>(7-j)) & 0x1)
                EIE3810_TFTLCD_DrawDot(x+i, y+j, color);
            else
                EIE3810_TFTLCD_DrawDot(x+i, y+j, bgcolor);
        }
        for(k=8;k<16;k++){
            if((asc2_1608[ASCII-32][2*i+1]>>(15-k)) & 0x1)
                EIE3810_TFTLCD_DrawDot(x+i, y+k, color);
            else
                EIE3810_TFTLCD_DrawDot(x+i, y+k, bgcolor);
        }
    }
}

// Wrapper for compatibility
void EIE3810_ShowChar(u16 x, u16 y, u8 ASCII, u16 color, u16 bgcolor){
    EIE3810_TFTLCD_ShowChar(x,y,ASCII,color,bgcolor);
}

// Show String 16x8 (Small) - USE THIS FOR SMALLER TEXT
void EIE3810_TFTLCD_ShowString1608(u16 x, u16 y, char* str, u16 color, u16 bgcolor)
{
	u16 i = 0;
	while(str[i] != '\0')
	{
		// Wrap text if it goes off screen
		if(x > LCD_WIDTH - 8) { x = 0; y += 16; }
		if(y > LCD_HEIGHT - 16) break;

		EIE3810_TFTLCD_ShowChar(x, y, str[i], color, bgcolor);
		x += 8;
		i++;
	}
}

// Show Single Char 24x12 (Large)
void EIE3810_TFTLCD_ShowChar2412(u16 x, u16 y, u8 ascii, u16 color, u16 bgcolor)
{
	u8 i, j, index;
    if(ascii < 32 || ascii > 127) return;
    ascii -= 32;

    for(j = 0; j < 3; j++)
    {
    	for(i = 0; i < 8; i++)
    	{
    		for(index = 0; index < 12; index++)
    		{
    			if((asc2_2412[ascii][index * 3 + j] >> (7-i)) & 0x01)
    				EIE3810_TFTLCD_DrawDot(x + index, y + j*8 + i, color);
    			else
    				EIE3810_TFTLCD_DrawDot(x + index, y + j*8 + i, bgcolor);
    		}
    	}
    }
}

// Show String 24x12 (Large)
void EIE3810_TFTLCD_ShowString2412(u16 x, u16 y, char* str, u16 color, u16 bgcolor)
{
	u16 i = 0;
	while(str[i] != '\0')
	{
		if(x > LCD_WIDTH - 12) { x = 0; y += 24; }
		if(y > LCD_HEIGHT - 24) break;

		EIE3810_TFTLCD_ShowChar2412(x, y, str[i], color, bgcolor);
		x += 12;
		i++;
	}
}

/* =========================================================================
 * INITIALIZATION
 * ========================================================================= */

void EIE3810_TFTLCD_SetParameter(void)
{
	// 4.3" LCD initialization (NT35510 or similar controller)
	EIE3810_TFTLCD_WrCmd(0x0100); // Software Reset
	Delay(100000);

	// Display settings for 4.3" LCD (480x800)
	EIE3810_TFTLCD_WrCmd(0xF000); EIE3810_TFTLCD_WrData(0x55);
	EIE3810_TFTLCD_WrCmd(0xF001); EIE3810_TFTLCD_WrData(0xAA);
	EIE3810_TFTLCD_WrCmd(0xF002); EIE3810_TFTLCD_WrData(0x52);
	EIE3810_TFTLCD_WrCmd(0xF003); EIE3810_TFTLCD_WrData(0x08);
	EIE3810_TFTLCD_WrCmd(0xF004); EIE3810_TFTLCD_WrData(0x01);

	// AVDD Set AVDD 5.2V
	EIE3810_TFTLCD_WrCmd(0xB000); EIE3810_TFTLCD_WrData(0x0D);
	EIE3810_TFTLCD_WrCmd(0xB001); EIE3810_TFTLCD_WrData(0x0D);
	EIE3810_TFTLCD_WrCmd(0xB002); EIE3810_TFTLCD_WrData(0x0D);

	// AVDD ratio
	EIE3810_TFTLCD_WrCmd(0xB600); EIE3810_TFTLCD_WrData(0x34);
	EIE3810_TFTLCD_WrCmd(0xB601); EIE3810_TFTLCD_WrData(0x34);
	EIE3810_TFTLCD_WrCmd(0xB602); EIE3810_TFTLCD_WrData(0x34);

	// AVEE -5.2V
	EIE3810_TFTLCD_WrCmd(0xB100); EIE3810_TFTLCD_WrData(0x0D);
	EIE3810_TFTLCD_WrCmd(0xB101); EIE3810_TFTLCD_WrData(0x0D);
	EIE3810_TFTLCD_WrCmd(0xB102); EIE3810_TFTLCD_WrData(0x0D);

	// AVEE ratio
	EIE3810_TFTLCD_WrCmd(0xB700); EIE3810_TFTLCD_WrData(0x34);
	EIE3810_TFTLCD_WrCmd(0xB701); EIE3810_TFTLCD_WrData(0x34);
	EIE3810_TFTLCD_WrCmd(0xB702); EIE3810_TFTLCD_WrData(0x34);

	// VCL -2.5V
	EIE3810_TFTLCD_WrCmd(0xB200); EIE3810_TFTLCD_WrData(0x00);
	EIE3810_TFTLCD_WrCmd(0xB201); EIE3810_TFTLCD_WrData(0x00);
	EIE3810_TFTLCD_WrCmd(0xB202); EIE3810_TFTLCD_WrData(0x00);

	// VCL ratio
	EIE3810_TFTLCD_WrCmd(0xB800); EIE3810_TFTLCD_WrData(0x24);
	EIE3810_TFTLCD_WrCmd(0xB801); EIE3810_TFTLCD_WrData(0x24);
	EIE3810_TFTLCD_WrCmd(0xB802); EIE3810_TFTLCD_WrData(0x24);

	// VGH 15V
	EIE3810_TFTLCD_WrCmd(0xBF00); EIE3810_TFTLCD_WrData(0x01);
	EIE3810_TFTLCD_WrCmd(0xB300); EIE3810_TFTLCD_WrData(0x0F);
	EIE3810_TFTLCD_WrCmd(0xB301); EIE3810_TFTLCD_WrData(0x0F);
	EIE3810_TFTLCD_WrCmd(0xB302); EIE3810_TFTLCD_WrData(0x0F);

	// VGH ratio
	EIE3810_TFTLCD_WrCmd(0xB900); EIE3810_TFTLCD_WrData(0x34);
	EIE3810_TFTLCD_WrCmd(0xB901); EIE3810_TFTLCD_WrData(0x34);
	EIE3810_TFTLCD_WrCmd(0xB902); EIE3810_TFTLCD_WrData(0x34);

	// VGL_REG -10V
	EIE3810_TFTLCD_WrCmd(0xB500); EIE3810_TFTLCD_WrData(0x08);
	EIE3810_TFTLCD_WrCmd(0xB501); EIE3810_TFTLCD_WrData(0x08);
	EIE3810_TFTLCD_WrCmd(0xB502); EIE3810_TFTLCD_WrData(0x08);
	EIE3810_TFTLCD_WrCmd(0xC200); EIE3810_TFTLCD_WrData(0x03);

	// VGLX ratio
	EIE3810_TFTLCD_WrCmd(0xBA00); EIE3810_TFTLCD_WrData(0x24);
	EIE3810_TFTLCD_WrCmd(0xBA01); EIE3810_TFTLCD_WrData(0x24);
	EIE3810_TFTLCD_WrCmd(0xBA02); EIE3810_TFTLCD_WrData(0x24);

	// VGMP/VGSP 4.5V/0V
	EIE3810_TFTLCD_WrCmd(0xBC00); EIE3810_TFTLCD_WrData(0x00);
	EIE3810_TFTLCD_WrCmd(0xBC01); EIE3810_TFTLCD_WrData(0x78);
	EIE3810_TFTLCD_WrCmd(0xBC02); EIE3810_TFTLCD_WrData(0x00);

	// VGMN/VGSN -4.5V/0V
	EIE3810_TFTLCD_WrCmd(0xBD00); EIE3810_TFTLCD_WrData(0x00);
	EIE3810_TFTLCD_WrCmd(0xBD01); EIE3810_TFTLCD_WrData(0x78);
	EIE3810_TFTLCD_WrCmd(0xBD02); EIE3810_TFTLCD_WrData(0x00);

	// VCOM
	EIE3810_TFTLCD_WrCmd(0xBE00); EIE3810_TFTLCD_WrData(0x00);
	EIE3810_TFTLCD_WrCmd(0xBE01); EIE3810_TFTLCD_WrData(0x64);

	// Memory Access Control - Portrait mode (480x800)
	EIE3810_TFTLCD_WrCmd(0x3600); EIE3810_TFTLCD_WrData(0x00);

	// Interface Pixel Format: 16-bit
	EIE3810_TFTLCD_WrCmd(0x3A00); EIE3810_TFTLCD_WrData(0x55);

	// Sleep Out
	EIE3810_TFTLCD_WrCmd(0x1100);
	Delay(120000);

	// Display On
	EIE3810_TFTLCD_WrCmd(0x2900);
}

void EIE3810_TFTLCD_Init(void)
{
	RCC->AHBENR  |= 1 << 8;
	RCC->APB2ENR |= 1 << 3;
	RCC->APB2ENR |= 1 << 5;
	RCC->APB2ENR |= 1 << 6;
	RCC->APB2ENR |= 1 << 8;

	GPIOB->CRL &= 0xFFFFFFF0; GPIOB->CRL |= 0x00000003;
	GPIOD->CRH &= 0x00FFF000; GPIOD->CRH |= 0xBB000BBB;
	GPIOD->CRL &= 0xFF00FF00; GPIOD->CRL |= 0x00BB00BB;
	GPIOE->CRH &= 0x00000000; GPIOE->CRH |= 0xBBBBBBBB;
	GPIOE->CRL &= 0x0FFFFFFF; GPIOE->CRL |= 0xB0000000;
	GPIOG->CRH &= 0xFFF0FFFF; GPIOG->CRH |= 0x000B0000;
	GPIOG->CRL &= 0xFFFFFFF0; GPIOG->CRL |= 0x0000000B;

	FSMC_Bank1->BTCR[6] = 0x00000000;
	FSMC_Bank1->BTCR[7] = 0x00000000;
	FSMC_Bank1E->BWTR[6] = 0x00000000;

	FSMC_Bank1->BTCR[6] |= 1 << 12;
	FSMC_Bank1->BTCR[6] |= 1 << 14;
	FSMC_Bank1->BTCR[6] |= 1 << 4;
	FSMC_Bank1->BTCR[6] |= 1 << 0;

	FSMC_Bank1->BTCR[7] |= 0 << 28;
	FSMC_Bank1->BTCR[7] |= 0x0F << 8;
	FSMC_Bank1->BTCR[7] |= 1 << 0;

	FSMC_Bank1E->BWTR[6] |= 0 << 28;
	FSMC_Bank1E->BWTR[6] |= 3 << 8;
	FSMC_Bank1E->BWTR[6] |= 0 << 0;

	Delay(50000);

	EIE3810_TFTLCD_SetParameter();

	GPIOB->ODR |= 1<<0;
	EIE3810_TFTLCD_Clear(0xFFFF);
}
