#include "EIE3810_LED.h"
#include "stm32f10x.h"

// LED initialization (simple stub for compatibility)
void EIE3810_LED_Init(void)
{
	// Enable GPIOE clock for LED (PE5)
	RCC->APB2ENR |= 1<<6; // PORTE clock enable

	// Configure PE5 as output
	GPIOE->CRL &= 0xFF0FFFFF;
	GPIOE->CRL |= 0x00300000; // PE5: Output mode, max speed 50 MHz
}
