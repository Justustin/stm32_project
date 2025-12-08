#include "stm32f10x.h"
#include "EIE3810_LED.h"
#include "EIE3810_Clock.h"
#include "EIE3810_TFTLCD.h"
#include "EIE3810_USART.h"
#include "EIE3810_GPIO.h"
#include "game.h"

// Function prototypes
void EIE3810_TIM3_PWMInit(u16 arr, u16 psc);
void EIE3810_TIM3_Init(u16 arr, u16 psc);
void EIE3810_TIM4_Init(u16 arr, u16 psc);
void EIE3810_NVIC_SetPriorityGroup(u8);
void Delay(u32);
void JOYPAD_Init(void);
void JOYPAD_Delay(u16 t);
u8 JOYPAD_Read(void);

// Game state variables
GameState currentState = STATE_WELCOME;
u8 difficulty = 0; // 0 = Easy, 1 = Hard
u8 playerA_ready = 0;
u8 playerB_ready = 0;
u8 usartReceived = 0;
u8 randomSeed = 0;

int main(void)
{
	// Initialize clock
	EIE3810_clock_tree_init();

	// Initialize peripherals
	EIE3810_LED_Init();
	EIE3810_TFTLCD_Init();

	JOYPAD_Init();
	Buzzer_Init();
	KEY_Init();

	// Initialize USART1 with proper baud rate
	// USART1 is on APB2 bus which runs at 72MHz
	// Baud rate for student ID 122040026: ~9600
	EIE3810_USART1_init(72, 9600);
	EIE3810_USART1_EXTIInit();

	// Initialize timers
	EIE3810_NVIC_SetPriorityGroup(2);
	EIE3810_TIM3_Init(499, 719); // Timer for game updates and joypad reading (10ms period)

	// Show welcome screen
	showWelcomeScreen();
	Delay(3000000); // Wait 3 seconds

	// Start difficulty selection
	currentState = STATE_DIFFICULTY_SELECT;
	showDifficultyScreen();

	// Main game loop
	while(1)
	{
		switch(currentState)
		{
			case STATE_DIFFICULTY_SELECT:
				// Wait for both players to be ready
				if(playerA_ready && playerB_ready)
				{
					currentState = STATE_WAIT_USART;
					showWaitUSARTScreen();
				}
				// Update display if difficulty changed
				break;

			case STATE_WAIT_USART:
				// Wait for USART random seed
				if(usartReceived)
				{
					// Display received seed (fixed for 2.8" LCD)
					showString(10, 200, "Received: ", RED, WHITE);
					showNumber(90, 200, randomSeed, 1, RED, WHITE);
					Delay(1000000);

					// Start countdown
					currentState = STATE_COUNTDOWN;
					startCountdown();

					// Initialize game
					initGame(randomSeed, difficulty);

					// Start playing
					currentState = STATE_PLAYING;
					usartReceived = 0;
				}
				break;

			case STATE_PLAYING:
				// Game runs in TIM3 interrupt
				// Just wait here
				break;

			case STATE_PAUSED:
				// Game is paused, wait for KEY1 or START to resume
				break;

			case STATE_GAME_OVER:
				showGameOverScreen();
				Delay(3000000); // Wait 3 seconds

				// Reset game state
				currentState = STATE_DIFFICULTY_SELECT;
				difficulty = 0;
				playerA_ready = 0;
				playerB_ready = 0;
				usartReceived = 0;
				showDifficultyScreen();
				break;

			default:
				break;
		}
	}
}

void JOYPAD_Init(void)
{
	RCC->APB2ENR |= 1<<3; // IO port B clock enable
	RCC->APB2ENR |= 1<<5; // IO port D clock enable
	GPIOB->CRH &= 0xFFFF00FF;
	GPIOB->CRH |= 0x00003800; // GPIOB10: input pull-up/down, GPIOB11: output, general purpose push-pull
	GPIOB->ODR |= 3<<10; // port output data, 10 and 11
	GPIOD->CRL &= 0xFFFF0FFF;
	GPIOD->CRL |= 0x00003000; // GPIOD3 output mode, general purpose push-pull
	GPIOD->ODR |= 1<<3; // port output data, 3
}

void JOYPAD_Delay(u16 t)
{
	while(t--);
}

u8 JOYPAD_Read(void)
{
	vu8 temp = 0;
	u8 t;
	GPIOB->BSRR |= 1<<11; // set ODR11 bit of PB11
	Delay(80);
	GPIOB->BSRR |= 1<<27; // reset ODR11 bit of PB11
	for(t = 0; t < 8; t++)
	{
		temp >>= 1;
		if((((GPIOB->IDR)>>10)&0x01) == 0) temp |= 0x80;
		// read input of PB10, if it is 0, set temp to 0x80
		GPIOD->BSRR |= (1<<3); // set ODR3 of PD3
		Delay(80);
		GPIOD->BSRR |= (1<<19); // reset ODR3 of PD3
		Delay(80);
	}
	return temp;
}

void EIE3810_TIM3_PWMInit(u16 arr, u16 psc)
{
	RCC->APB2ENR |= 1<<3; // IO port B enable
	GPIOB->CRL &= 0xFF0FFFFF;
	GPIOB->CRL |= 0X00B00000; // PB5 output mode, alternative push-pull
	RCC->APB2ENR |= 1<<0; // AFIOEN: Alternative function IO clock enable
	AFIO->MAPR &= 0xFFFFF3FF;
	AFIO->MAPR |= 1<<11; // TIM3_REMAP: partial remap, remap TIM3_CH2 to the PB5
	RCC->APB1ENR |= 1<<1; // TIM3 enable
	TIM3->ARR = arr;
	TIM3->PSC = psc; // set prescaler and reload value
	TIM3->CCMR1 |= 7<<12; // set TIM3 output compare 2 mode
	TIM3->CCMR1 |= 1<<11; // output compare 2 preload enable
	TIM3->CCER |= 1<<4; // Capture/compare 2 output enable
	TIM3->CR1 = 0x0080; // TXE interrupt enable
	TIM3->CR1 |= 1<<0; // Counter enable
}

void EIE3810_NVIC_SetPriorityGroup(u8 prigroup)
{
	u32 temp1, temp2;
	temp2 = prigroup & 0x00000007;
	temp2 <<= 8;
	temp1 = SCB->AIRCR;
	temp1 &= 0x0000F8FF;
	temp1 |= 0x05FA0000;
	temp1 |= temp2;
	SCB->AIRCR = temp1;
}

void EIE3810_TIM3_Init(u16 arr, u16 psc)
{
	// TIM3
	RCC->APB1ENR |= 1<<1; // TIM3EN: enable TIM3
	TIM3->ARR = arr;
	TIM3->PSC = psc;
	TIM3->DIER |= 1<<0; // update interrupt enable
	TIM3->CR1 |= 0x01; // counter enable
	NVIC->IP[29] = 0X45; // set interrupt priority of TIM3
	NVIC->ISER[0] = (1<<29); // enable interrupt TIM3
}

void EIE3810_TIM4_Init(u16 arr, u16 psc)
{
	// TIM4
	RCC->APB1ENR |= 1<<2; // TIM4EN: enable TIM4
	TIM4->ARR = arr;
	TIM4->PSC = psc;
	TIM4->DIER |= 1<<0; // update interrupt enable
	TIM4->CR1 |= 0x01; // counter enable
	NVIC->IP[30] = 0X45; // set interrupt priority of TIM4
	NVIC->ISER[0] = (1<<30); // enable interrupt TIM4
}

void Delay(u32 count)
{
	u32 i;
	for(i = 0; i < count; i++);
}
