#include "stm32f10x.h"
#include "EIE3810_TFTLCD.h"
#include "Font.H"

extern void Delay(u32 count);
extern void EIE3810_TFTLCD_DrawDot(u16 x, u16 y, u16 color);
extern void EIE3810_TFTLCD_WrCmd(u16 cmd);
extern void EIE3810_TFTLCD_WrData(u16 data);
extern void EIE3810_TFTLCD_FillRectangle(u16 start_x, u16 length_x, u16 start_y, u16 length_y, u16 color);

void EIE3810_TFTLCD_DrawCircle(u16 x0,u16 y0, u8 r, u8 full,u16 color) 
	/* x0, y0 : center of the circle; r: radius; full: whether the circle is hollow or solid; color: color of the circle.*/
{
	int a,b;
	int di,yi;
	a=0;b=r;	  
	di=3-(r<<1);             
  if(!full)        //Hollow circle        
  {
			while(a<=b) 
			{
					EIE3810_TFTLCD_DrawDot(x0+a,y0-b,color);             
					EIE3810_TFTLCD_DrawDot(x0+b,y0-a,color);             
					EIE3810_TFTLCD_DrawDot(x0+b,y0+a,color);             
					EIE3810_TFTLCD_DrawDot(x0+a,y0+b,color);             
					EIE3810_TFTLCD_DrawDot(x0-a,y0+b,color);             
					EIE3810_TFTLCD_DrawDot(x0-b,y0+a,color);             
					EIE3810_TFTLCD_DrawDot(x0-a,y0-b,color);             
					EIE3810_TFTLCD_DrawDot(x0-b,y0-a,color);             
					a++;					
					//Use Bresenham algorithm 
					if(di<0)di +=4*a+6;	  
					else
					{
						di+=10+4*(a-b);   
						b--;
					} 						    
			}
	}
  else                   //  Solid circle
	{
			while(a<=b)
			{
					for(yi=a;yi<=b;yi++)
					{
							EIE3810_TFTLCD_DrawDot(x0+a,y0-yi,color);             
							EIE3810_TFTLCD_DrawDot(x0+yi,y0-a,color);             
							EIE3810_TFTLCD_DrawDot(x0+yi,y0+a,color);             
							EIE3810_TFTLCD_DrawDot(x0+a,y0+yi,color);             
							EIE3810_TFTLCD_DrawDot(x0-a,y0+yi,color);             
							EIE3810_TFTLCD_DrawDot(x0-yi,y0+a,color);             
							EIE3810_TFTLCD_DrawDot(x0-a,y0-yi,color);             
							EIE3810_TFTLCD_DrawDot(x0-yi,y0-a,color);             
					}
					a++;
					//Use Bresenham algorithm 
					if(di<0)di +=4*a+6;	  
					else
					{
						di+=10+4*(a-b);   
						b--;
					} 						    
			}
	}
}	

/*****    24 *12 fonts        *********/

void EIE3810_TFTLCD_ShowChar2412(u16 x, u16 y, u8 ascii, u16 color, u16 bgcolor)
{
	u8 i,j;
	u8 index;
	u8 height = 24, length = 12;
	if(ascii < 32 || ascii > 127) return;
	ascii -= 32;

	EIE3810_TFTLCD_WrCmd(0x2A00); EIE3810_TFTLCD_WrData(x >> 8);
	EIE3810_TFTLCD_WrCmd(0x2A01); EIE3810_TFTLCD_WrData(x & 0xFF);
	EIE3810_TFTLCD_WrCmd(0x2A02); EIE3810_TFTLCD_WrData((length + x - 1) >> 8);
	EIE3810_TFTLCD_WrCmd(0x2A03); EIE3810_TFTLCD_WrData((length + x - 1) & 0xFF);

	EIE3810_TFTLCD_WrCmd(0x2B00);	EIE3810_TFTLCD_WrData(y >> 8);
	EIE3810_TFTLCD_WrCmd(0x2B01);	EIE3810_TFTLCD_WrData(y & 0xFF);
	EIE3810_TFTLCD_WrCmd(0x2B02);	EIE3810_TFTLCD_WrData((height + y - 1)>> 8);
	EIE3810_TFTLCD_WrCmd(0x2B03);	EIE3810_TFTLCD_WrData((height + y - 1) & 0xFF);

	EIE3810_TFTLCD_WrCmd(0x2C00);

	for(j=0; j<height/8;j++)
	{
		for(i=0; i<height/3;i++)
		{
			for(index=0; index<length; index++)
			{
				if((asc2_2412[ascii][index*3+j]<<i) & 0x80)
					EIE3810_TFTLCD_WrData(color);
				else EIE3810_TFTLCD_WrData(bgcolor);
			}
		}
	}
}

void EIE3810_TFTLCD_ShowString2412(u16 x, u16 y, char* str, u16 color, u16 bgcolor)
{
	u16 i = 0;
	while(str[i] != '\0')
	{
		EIE3810_TFTLCD_ShowChar2412(x + i*12, y, str[i], color, bgcolor);
		i++;
	}
}

void EIE3810_TFTLCD_DrawRectangle(u16 start_x, u16 start_y, u16 width, u16 height, u16 color)
{
	u16 i;
	// Top horizontal line
	for(i = 0; i < width; i++)
	{
		EIE3810_TFTLCD_DrawDot(start_x + i, start_y, color);
	}
	// Bottom horizontal line
	for(i = 0; i < width; i++)
	{
		EIE3810_TFTLCD_DrawDot(start_x + i, start_y + height - 1, color);
	}
	// Left vertical line
	for(i = 0; i < height; i++)
	{
		EIE3810_TFTLCD_DrawDot(start_x, start_y + i, color);
	}
	// Right vertical line
	for(i = 0; i < height; i++)
	{
		EIE3810_TFTLCD_DrawDot(start_x + width - 1, start_y + i, color);
	}
}

void EIE3810_TFTLCD_FillScreen(u16 color)
{
	EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, color);
}




