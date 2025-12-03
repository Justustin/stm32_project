#ifndef __EIE3810_GPIO_H
#define __EIE3810_GPIO_H

#include "stm32f10x.h"

// Buzzer control functions
void Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);

// KEY button functions
void KEY_Init(void);

#endif
