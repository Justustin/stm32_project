#include "stm32f10x.h"
#include "EIE3810_LED.h"
#include "EIE3810_Clock.h"
#include "EIE3810_TFTLCD.h"
#include "EIE3810_USART.h"
#define LED0_PWM_VAL TIM3->CCR2

void EIE3810_TIM3_PWMInit(u16 arr, u16 psc);
void EIE3810_TIM3_Init(u16 arr, u16 psc);
void EIE3810_TIM4_Init(u16 arr, u16 psc);
void EIE3810_NVIC_SetPriorityGroup(u8);
void TIM3_IRQHandler(void);
void TIM4_IRQHandler(void);
void Delay(u32);
void EIE3810_SYSTICK_Init(void);
u8 task1HeartBeat =0;
u8 task2HeartBeat=0;
void JOYPAD_Init(void);
void JOYPAD_Delay(u16 t);
u8 JOYPAD_Read(void);

int main(void)
{
	EIE3810_clock_tree_init();
	EIE3810_LED_Init();
	EIE3810_TFTLCD_Init();
	JOYPAD_Init();
	EIE3810_TIM3_PWMInit(9999, 71);   
	EIE3810_TIM3_Init(499, 719); 
	EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, RED);
	while(1)
	{
		;
	}
}

void JOYPAD_Init(void){
	RCC->APB2ENR |= 1<<3; // IO port B clock enable
	RCC->APB2ENR |= 1<<5; // IO port D clock enable
	GPIOB->CRH &= 0xFFFF00FF;
	GPIOB->CRH |= 0x00003800; // GPIOB10: input pull -up /down, GPIOx11: ouput, general purpose push pull
	GPIOB->ODR |= 3<<10; // port output data, 10 and 11
	GPIOD->CRL &= 0xFFFF0FFF; 
	GPIOD->CRL |= 0x00003000; // GPIOD3 ouput mode, general purpose push pull
	GPIOD->ODR |= 1<<3; // port output data, 3
}


void JOYPAD_Delay(u16 t)
{
	while (t--);
}

u8 JOYPAD_Read(void){
	vu8 temp=0;
	u8 t;
	GPIOB->BSRR |= 1<<11; // set ODR11 bit of PB11
	Delay(80);
	GPIOB->BSRR |= 1<<27; // reset ODR11 bit of PB11
	for (t=0; t<8; t++)
	{
		temp>>=1;
		if((((GPIOB->IDR)>>10)&0x01)==0) temp |= 0x80;
		// read input of PB10, if it is 1, set temo to 0x80
		GPIOD->BSRR |= (1<<3); // set ODR3 of PD3
		Delay(80);
		GPIOD->BSRR |= (1<<19); // set ODR3 of PD3 
		Delay(80);
	}
	return temp;
}

void EIE3810_TIM3_PWMInit(u16 arr, u16 psc)
{
	RCC->APB2ENR |= 1<<3; // IO port B enable
	GPIOB->CRL &= 0xFF0FFFFF;
	GPIOB->CRL |= 0X00B00000; // PB5 output mode, alternative push-pull
	RCC->APB2ENR |= 1<<0; // AFIOEN: Alternative funtion IO clock enable
	AFIO->MAPR &= 0xFFFFF3FF;
	AFIO->MAPR |= 1<<11; // TIM3_REMAP: partial remap, remap TIM3_CH2 to the PB5
	RCC->APB1ENR |= 1<<1; // TIM3 enable
	TIM3->ARR = arr;
	TIM3->PSC = psc; // set prescaler and reload value
	TIM3->CCMR1 |= 7<<12; // set TIM3 output compare 2 mode
	TIM3->CCMR1 |= 1<<11; // output compare 2 preload enable
	TIM3->CCER |= 1<<4; // Capture/compare 2 output enable
	TIM3->CR1 = 0x0080; // TXE interrupt enable
	TIM3->CR1 |= 1<<0; // break character will be transimitted
}


void EIE3810_SYSTICK_Init(void) // if we need to change the frequency, remember to reset the load register
{
	// SYSTICK
	SysTick->CTRL = 0; // Clear SysTick->CTRL setting
	SysTick->LOAD = 90000; // reload value 9*e4, now that the frquency = 9MHx/90000 = 100 Hz =>10ms period clock
	SysTick->CTRL = 0x3; // control abd status register, use clock source 0
	// CLKSOURCE=0: FCLK/8
	// CLKSOURCE=1: FCLK
	// CLKSOURCE=0 is synchronized and better than CLKSOURCE=1
	// P92 of RM0008
}


void EIE3810_NVIC_SetPriorityGroup(u8 prigroup){
    u32 temp1, temp2;
    temp2= prigroup&0x00000007;
    temp2 <<=8; // Left shift temp2 by 8 bits. Preparing the priority group bits to be inserted into the correct position of the AIRCR register.
    temp1 = SCB->AIRCR; // Read the current value of the Application Interrupt and Reset Control Register (AIRCR) into temp1.
    temp1 &= 0x0000F8FF; // Clear the bits where the priority group will be set (bits 10:8) in the AIRCR register, without affecting other bits.
    temp1 |= 0x05FA0000; // Set the VECTKEY bits (16:31) to 0x05FA, which is required to write to AIRCR register (this is a write protection mechanism).
    temp1 |=temp2;
    SCB->AIRCR =temp1;
}

void EIE3810_TIM3_Init(u16 arr, u16 psc)
{
	// TIM3
	RCC->APB1ENR |= 1<<1; // TIM3EN: enable TIM3
	TIM3->ARR = arr; 
	// set prescaler value, The counter clock frequency CK_CNT is equal to fCK_PSC / (PSC[15:0] + 1).
	TIM3->PSC = psc; 
	// set prescaler value, ARR is the value to be loaded in the actual auto-reload register.
	TIM3->DIER |= 1<<0; // update interrupt enable
	TIM3->CR1 |= 0x01; // counter enable + UEV disable
	/* the bug occurs here, the TIM3 is interrupt 29 */
	NVIC->IP[29] = 0X45; // set interrupt priority of TIM3
	NVIC->ISER[0] = (1<<29); // enable interrupt TIM3
}

void EIE3810_TIM4_Init(u16 arr, u16 psc)
{
	// TIM4
	RCC->APB1ENR |= 1<<2; // TIM4EN: enable TIM4
	TIM4->ARR = arr; 
	// set prescaler value, The counter clock frequency CK_CNT is equal to fCK_PSC / (PSC[15:0] + 1).
	TIM4->PSC = psc; 
	// set prescaler value, ARR is the value to be loaded in the actual auto-reload register.
	TIM4->DIER |= 1<<0; // update interrupt enable
	TIM4->CR1 |= 0x01; // counter enable + UEV disable
	// the TIM4 is interrupt 30 
	NVIC->IP[30] = 0X45; // set interrupt priority of TIM4
	NVIC->ISER[0] = (1<<30); // enable interrupt TIM4
}

void TIM3_IRQHandler(void)
{
	int i, idx;
	if (TIM3->SR & (1<<0)) //Updating the interrupt
	{
		u8 Key_Data = JOYPAD_Read();
		if (Key_Data)
		{
			for (i=0; i<8; i++)
			{
				if ((Key_Data >> i) == 1)
				{
					idx = i+1;
				}
			}
		}
	}
	
	switch(idx)
	{
		case 1:  //A
			EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, RED);
           EIE3810_TFTLCD_ShowChar(250, 400, 'A', BLUE, RED);
			break;
		case 2:  //B
			EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, RED);	
			EIE3810_TFTLCD_ShowChar(250, 400, 'B', BLUE, RED);
		  break;
		case 3:  //SELECT
			EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, RED);
			EIE3810_TFTLCD_ShowChar(250, 400, 'S', BLUE, RED);
			EIE3810_TFTLCD_ShowChar(258, 400, 'E', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(266, 400, 'L', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(274, 400, 'E', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(282, 400, 'C', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(290, 400, 'T', BLUE, RED);
		  break;
		case 4:  //START
			EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, RED);
			EIE3810_TFTLCD_ShowChar(250, 400, 'S', BLUE, RED);
			EIE3810_TFTLCD_ShowChar(258, 400, 'T', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(266, 400, 'A', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(274, 400, 'R', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(282, 400, 'T', BLUE, RED);
            break;
		case 5:  //UP
			EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, RED);
			EIE3810_TFTLCD_ShowChar(250, 400, 'U', BLUE, RED);
			EIE3810_TFTLCD_ShowChar(258, 400, 'P', BLUE, RED);
		  break;
		case 6:  //DOWN
			EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, RED);
			EIE3810_TFTLCD_ShowChar(250, 400, 'D', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(258, 400, 'O', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(266, 400, 'W', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(274, 400, 'N', BLUE, RED);
            break;
		case 7:  //LEFT
			EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, RED);
			EIE3810_TFTLCD_ShowChar(250, 400, 'L', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(258, 400, 'E', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(266, 400, 'F', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(274, 400, 'T', BLUE, RED);
            break;
		case 8:  //RIGHT
			EIE3810_TFTLCD_FillRectangle(0, 480, 0, 800, RED);
			EIE3810_TFTLCD_ShowChar(250, 400, 'R', BLUE, RED);
			EIE3810_TFTLCD_ShowChar(258, 400, 'I', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(266, 400, 'G', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(274, 400, 'H', BLUE, RED);
            EIE3810_TFTLCD_ShowChar(282, 400, 'T', BLUE, RED);
            break;
		default: //WRONG
            break;
	}
	TIM3->SR &= ~(1<<0); 
}

void TIM4_IRQHandler(void)
{
    if (TIM4->SR &1<<0) //Update interrupt flag = 1
    {
        GPIOE-> ODR ^=1<<5;// //GPIOE pin 5 output
    }
    TIM4->SR &=~(1<<0); //Update interrupt flag = 0
}

void Delay(u32 count) //Delay subroutine generate some time delay by looping
{
    u32 i;
    for(i=0;i<count;i++);
}
