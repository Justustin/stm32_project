#include "EIE3810_GPIO.h"
#include "stm32f10x.h"

// Buzzer is typically on GPIOB Pin 8
void Buzzer_Init(void)
{
	RCC->APB2ENR |= 1<<3; // Enable PORTB clock
	GPIOB->CRH &= 0xFFFFFFF0; // Clear PB8 configuration
	GPIOB->CRH |= 0x00000003; // PB8: Output mode, max speed 50 MHz, General purpose output push-pull
	GPIOB->BRR = 1<<8; // Reset PB8 (Buzzer off)
}

void Buzzer_On(void)
{
	GPIOB->BSRR = 1<<8; // Set PB8 (Buzzer on)
}

void Buzzer_Off(void)
{
	GPIOB->BRR = 1<<8; // Reset PB8 (Buzzer off)
}

// KEY buttons are typically on GPIOE (KEY0: PE4, KEY1: PE3, KEY2: PE2)
// They will be configured for external interrupts
void KEY_Init(void)
{
	// Enable PORTE and AFIO clocks
	RCC->APB2ENR |= 1<<6; // PORTE clock enable
	RCC->APB2ENR |= 1<<0; // AFIO clock enable

	// Configure PE2, PE3, PE4 as inputs with pull-up
	GPIOE->CRL &= 0xFFF000FF; // Clear PE2, PE3, PE4 configuration
	GPIOE->CRL |= 0x000888000; // Input with pull-up / pull-down
	GPIOE->ODR |= 0x1C; // Enable pull-up on PE2, PE3, PE4 (bits 2, 3, 4)

	// Configure EXTI for PE2, PE3, PE4
	AFIO->EXTICR[0] &= 0xFF0F; // Clear EXTI2, EXTI3
	AFIO->EXTICR[0] |= 0x0040; // EXTI2 -> PE2
	AFIO->EXTICR[0] |= 0x0400; // EXTI3 -> PE3

	AFIO->EXTICR[1] &= 0xFFF0; // Clear EXTI4
	AFIO->EXTICR[1] |= 0x0004; // EXTI4 -> PE4

	// Configure EXTI lines for falling edge trigger (button press)
	EXTI->FTSR |= (1<<2) | (1<<3) | (1<<4); // Falling trigger enable for EXTI2, 3, 4
	EXTI->IMR |= (1<<2) | (1<<3) | (1<<4); // Interrupt mask enable for EXTI2, 3, 4

	// Enable NVIC interrupts
	// EXTI2 -> IRQ8, EXTI3 -> IRQ9, EXTI4 -> IRQ10
	NVIC->IP[8] = 0x65; // Priority for EXTI2
	NVIC->ISER[0] |= (1<<8); // Enable EXTI2 interrupt

	NVIC->IP[9] = 0x65; // Priority for EXTI3
	NVIC->ISER[0] |= (1<<9); // Enable EXTI3 interrupt

	NVIC->IP[10] = 0x65; // Priority for EXTI4
	NVIC->ISER[0] |= (1<<10); // Enable EXTI4 interrupt
}
