#include "stm32f10x.h"
#include "EIE3810_Clock.h"

void EIE3810_USART1_EXTIInit(void)
{
	NVIC->IP[37] = 0x65; // set interrupt priority 0x65
	NVIC->ISER[1] |= 1<<5; // enable interrupt USART1
}

void EIE3810_USART1_init(u32 pclk2, u32 baudrate) // pclk2: APB2 clock in MHz (72 for STM32F103)
{
	//USART1 is on APB2 bus
	float temp;
	u16 mantissa;
	u16 fraction;
	temp=(float) (pclk2*1000000)/(baudrate*16); // Tx/Rx baud
	mantissa=temp;
	fraction=(temp-mantissa)*16;
	mantissa<<=4;
	mantissa+=fraction;
	RCC->APB2ENR |= 1<<14; // Enable USART1 clock (CRITICAL - was missing!)
	RCC->APB2ENR |= 1<<2; //enable GPIOA
	GPIOA->CRH &= 0xFFFFF00F; //
	GPIOA->CRH |= 0x000008B0; //PA9(TX):output, push-pull; PA10(RX):input, pull-up/pull down
	GPIOA->ODR |= 1<<10; // Enable pull-up on PA10 (RX)
	RCC->APB2RSTR |= 1<<14; //Reset USART1
	RCC->APB2RSTR &= ~(1<<14); //Release reset
	USART1->BRR=mantissa;//set the baud rate
	USART1->CR1=0x200C; // UE=1 (enable), TE=1 (transmit), RE=1 (receive), M=0 (8-bit), PCE=0 (no parity)
	USART1->CR1|=0x20; // RXNEIE: RXNE interrupt enable
}


void EIE3810_USART2_init(u32 pclk1, u32 baudrate) // pclk: input clock to the peripheral
{
	//USART2
	float temp;
	u16 mantissa;
	u16 fraction;
	temp=(float) (pclk1*1000000)/(baudrate*16); // Tx/Rx baud
	mantissa=temp;
	fraction=(temp-mantissa)*16;
	mantissa<<=4;
	mantissa+=fraction;
	RCC->APB2ENR |= 1<<2; //enable GPIOA
	GPIOA->CRL &= 0xFFFF00FF; //
	GPIOA->CRL |= 0x00008B00; //PA2:output, push-pill; PA3:input, pull-up/pull down
	RCC->APB1RSTR |= 1<<17; //Reset USART2
	RCC->APB1RSTR &= ~(1<<17); //no effect, set the value back to 0
	USART2->BRR=mantissa;//set the baud rate
	USART2->CR1=0x2008; //transmitter is enabled + word length is 9 bit(1 start, 8 data) + disable parity
}

void USART_print(u8 USARTport, char *st){
    u8 i = 0;
    while(st[i] != 0x00)
    {
        if (USARTport == 1){
            USART1->DR = st[i];
            while (!((USART1->SR & 0x80) >>7));
        }
        if (USARTport == 2){
            USART2->DR = st[i];
            while (!((USART2->SR & 0x80) >>7));
        }
        //Delay(50000);
        if (i == 255) break;
        i++;
    }
}
