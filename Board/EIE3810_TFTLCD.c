#include "stm32f10x.h"
#include "EIE3810_TFTLCD.h"
#include "Font.H"

extern void Delay(u32 count);

/*
 * =========================================================================
 * ALIENTEK DNF103 / WARSHIP / ELITE BOARD - 2.8" LCD CONFIGURATION
 * =========================================================================
 * Orientation:    PORTRAIT (240x320)
 * Solution:       Added 16x8 Font String function to fit more text.
 * =========================================================================
 */

#define LCD_WIDTH   240
#define LCD_HEIGHT  320

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
	EIE3810_TFTLCD_WrCmd(0x2A); // Column Address Set
	EIE3810_TFTLCD_WrData(xStar >> 8);
	EIE3810_TFTLCD_WrData(xStar & 0xFF);
	EIE3810_TFTLCD_WrData(xEnd >> 8);
	EIE3810_TFTLCD_WrData(xEnd & 0xFF);

	EIE3810_TFTLCD_WrCmd(0x2B); // Page Address Set
	EIE3810_TFTLCD_WrData(yStar >> 8);
	EIE3810_TFTLCD_WrData(yStar & 0xFF);
	EIE3810_TFTLCD_WrData(yEnd >> 8);
	EIE3810_TFTLCD_WrData(yEnd & 0xFF);

	EIE3810_TFTLCD_WrCmd(0x2C); // Write GRAM Start
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
	EIE3810_TFTLCD_WrCmd(0x01); // Software Reset
	Delay(100000);

	// Power Control
	EIE3810_TFTLCD_WrCmd(0xCB);
	EIE3810_TFTLCD_WrData(0x39); EIE3810_TFTLCD_WrData(0x2C); EIE3810_TFTLCD_WrData(0x00);
	EIE3810_TFTLCD_WrData(0x34); EIE3810_TFTLCD_WrData(0x02);

	EIE3810_TFTLCD_WrCmd(0xCF);
	EIE3810_TFTLCD_WrData(0x00); EIE3810_TFTLCD_WrData(0xC1); EIE3810_TFTLCD_WrData(0x30);

	EIE3810_TFTLCD_WrCmd(0xE8);
	EIE3810_TFTLCD_WrData(0x85); EIE3810_TFTLCD_WrData(0x00); EIE3810_TFTLCD_WrData(0x78);

	EIE3810_TFTLCD_WrCmd(0xEA);
	EIE3810_TFTLCD_WrData(0x00); EIE3810_TFTLCD_WrData(0x00);

	EIE3810_TFTLCD_WrCmd(0xED);
	EIE3810_TFTLCD_WrData(0x64); EIE3810_TFTLCD_WrData(0x03); EIE3810_TFTLCD_WrData(0x12); EIE3810_TFTLCD_WrData(0x81);

	EIE3810_TFTLCD_WrCmd(0xF7);
	EIE3810_TFTLCD_WrData(0x20);

	EIE3810_TFTLCD_WrCmd(0xC0); EIE3810_TFTLCD_WrData(0x23);
	EIE3810_TFTLCD_WrCmd(0xC1); EIE3810_TFTLCD_WrData(0x10);
	EIE3810_TFTLCD_WrCmd(0xC5); EIE3810_TFTLCD_WrData(0x3E); EIE3810_TFTLCD_WrData(0x28);
	EIE3810_TFTLCD_WrCmd(0xC7); EIE3810_TFTLCD_WrData(0x86);

	// ------------------------------------------------------------------
	// MEMORY ACCESS CONTROL (ORIENTATION SETTING)
	// 0x08 = Portrait (240x320)
	// ------------------------------------------------------------------
	EIE3810_TFTLCD_WrCmd(0x36);
	EIE3810_TFTLCD_WrData(0x08); // Portrait

	EIE3810_TFTLCD_WrCmd(0x3A); EIE3810_TFTLCD_WrData(0x55); // 16-bit
	EIE3810_TFTLCD_WrCmd(0xB1); EIE3810_TFTLCD_WrData(0x00); EIE3810_TFTLCD_WrData(0x18);
	EIE3810_TFTLCD_WrCmd(0xB6); EIE3810_TFTLCD_WrData(0x08); EIE3810_TFTLCD_WrData(0x82); EIE3810_TFTLCD_WrData(0x27);
	EIE3810_TFTLCD_WrCmd(0xF2); EIE3810_TFTLCD_WrData(0x00);
	EIE3810_TFTLCD_WrCmd(0x26); EIE3810_TFTLCD_WrData(0x01);

	// Gamma
	EIE3810_TFTLCD_WrCmd(0xE0);
	EIE3810_TFTLCD_WrData(0x0F); EIE3810_TFTLCD_WrData(0x31); EIE3810_TFTLCD_WrData(0x2B);
	EIE3810_TFTLCD_WrData(0x0C); EIE3810_TFTLCD_WrData(0x0E); EIE3810_TFTLCD_WrData(0x08);
	EIE3810_TFTLCD_WrData(0x4E); EIE3810_TFTLCD_WrData(0xF1); EIE3810_TFTLCD_WrData(0x37);
	EIE3810_TFTLCD_WrData(0x07); EIE3810_TFTLCD_WrData(0x10); EIE3810_TFTLCD_WrData(0x03);
	EIE3810_TFTLCD_WrData(0x0E); EIE3810_TFTLCD_WrData(0x09); EIE3810_TFTLCD_WrData(0x00);

	EIE3810_TFTLCD_WrCmd(0xE1);
	EIE3810_TFTLCD_WrData(0x00); EIE3810_TFTLCD_WrData(0x0E); EIE3810_TFTLCD_WrData(0x14);
	EIE3810_TFTLCD_WrData(0x03); EIE3810_TFTLCD_WrData(0x11); EIE3810_TFTLCD_WrData(0x07);
	EIE3810_TFTLCD_WrData(0x31); EIE3810_TFTLCD_WrData(0xC1); EIE3810_TFTLCD_WrData(0x48);
	EIE3810_TFTLCD_WrData(0x08); EIE3810_TFTLCD_WrData(0x0F); EIE3810_TFTLCD_WrData(0x0C);
	EIE3810_TFTLCD_WrData(0x31); EIE3810_TFTLCD_WrData(0x36); EIE3810_TFTLCD_WrData(0x0F);

	EIE3810_TFTLCD_WrCmd(0x11); // Sleep Out
	Delay(120000);

	EIE3810_TFTLCD_WrCmd(0x29); // Display On
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
