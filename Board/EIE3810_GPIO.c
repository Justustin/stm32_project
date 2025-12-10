#include "EIE3810_GPIO.h"
#include "stm32f10x.h"

// Buzzer on GPIOB Pin 8 (active-HIGH: HIGH=ON, LOW=OFF)
void Buzzer_Init(void)
{
	RCC->APB2ENR |= 1<<3; // Enable PORTB clock
	GPIOB->CRH &= 0xFFFFFFF0; // Clear PB8 configuration
	GPIOB->CRH |= 0x00000003; // PB8: Output mode, max speed 50 MHz, push-pull
	GPIOB->ODR &= ~(1<<8); // Set PB8 LOW (Buzzer off initially)
}

void Buzzer_On(void)
{
	GPIOB->ODR |= (1<<8); // Set PB8 HIGH (Buzzer on)
}

void Buzzer_Off(void)
{
	GPIOB->ODR &= ~(1<<8); // Set PB8 LOW (Buzzer off)
}

// KEY buttons: KEY0 (PE4), KEY1 (PE3), KEY2 (PE2), KEY_UP (PA0)
// They will be configured for external interrupts
void KEY_Init(void)
{
	// Enable PORTA, PORTE and AFIO clocks
	RCC->APB2ENR |= 1<<2; // PORTA clock enable
	RCC->APB2ENR |= 1<<6; // PORTE clock enable
	RCC->APB2ENR |= 1<<0; // AFIO clock enable

	// Configure PA0 (KEY_UP) as input with pull-down
	// KEY_UP is active high (pressed = high), so use pull-down
	GPIOA->CRL &= 0xFFFFFFF0; // Clear PA0 configuration (bits 0-3)
	GPIOA->CRL |= 0x00000008; // PA0: Input with pull-up/pull-down
	GPIOA->ODR &= ~(1<<0); // Pull-down on PA0

	// Configure PE2, PE3, PE4 as inputs with pull-up
	GPIOE->CRL &= 0xFFF000FF; // Clear PE2, PE3, PE4 configuration
	GPIOE->CRL |= 0x00088800; // Input with pull-up / pull-down for PE2(8-11), PE3(12-15), PE4(16-19)
	GPIOE->ODR |= 0x1C; // Enable pull-up on PE2, PE3, PE4 (bits 2, 3, 4)

	// Configure EXTI0 for PA0 (KEY_UP)
	// EXTICR[0] bits 0-3 = EXTI0 source, Port A = 0x0
	AFIO->EXTICR[0] &= ~0x000F; // Clear EXTI0
	AFIO->EXTICR[0] |= 0x0000; // EXTI0 -> PA0 (port A = 0)

	// Configure EXTI for PE2, PE3, PE4
	// EXTICR[0]: bits 8-11 = EXTI2, bits 12-15 = EXTI3
	// Port E = 0x4
	AFIO->EXTICR[0] &= ~0xFF00; // Clear EXTI2 (bits 8-11) and EXTI3 (bits 12-15)
	AFIO->EXTICR[0] |= 0x0400; // EXTI2 -> PE2 (port E = 4 << 8)
	AFIO->EXTICR[0] |= 0x4000; // EXTI3 -> PE3 (port E = 4 << 12)

	AFIO->EXTICR[1] &= 0xFFF0; // Clear EXTI4
	AFIO->EXTICR[1] |= 0x0004; // EXTI4 -> PE4

	// Configure EXTI0 (KEY_UP) for rising edge trigger (active high button)
	EXTI->RTSR |= (1<<0); // Rising trigger enable for EXTI0
	EXTI->IMR |= (1<<0); // Interrupt mask enable for EXTI0

	// Configure EXTI lines 2,3,4 for falling edge trigger (active low buttons)
	EXTI->FTSR |= (1<<2) | (1<<3) | (1<<4); // Falling trigger enable for EXTI2, 3, 4
	EXTI->IMR |= (1<<2) | (1<<3) | (1<<4); // Interrupt mask enable for EXTI2, 3, 4

	// Enable NVIC interrupts
	// EXTI0 -> IRQ6, EXTI2 -> IRQ8, EXTI3 -> IRQ9, EXTI4 -> IRQ10
	NVIC->IP[6] = 0x65; // Priority for EXTI0 (KEY_UP)
	NVIC->ISER[0] |= (1<<6); // Enable EXTI0 interrupt

	NVIC->IP[8] = 0x65; // Priority for EXTI2
	NVIC->ISER[0] |= (1<<8); // Enable EXTI2 interrupt

	NVIC->IP[9] = 0x65; // Priority for EXTI3
	NVIC->ISER[0] |= (1<<9); // Enable EXTI3 interrupt

	NVIC->IP[10] = 0x65; // Priority for EXTI4
	NVIC->ISER[0] |= (1<<10); // Enable EXTI4 interrupt
}
