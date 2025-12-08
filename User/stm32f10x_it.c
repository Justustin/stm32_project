/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"
#include "game.h"
#include "EIE3810_TFTLCD.h"

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

// External variables from main.c
extern u8 difficulty;
extern u8 playerA_ready;
extern u8 playerB_ready;
extern u8 usartReceived;
extern u8 randomSeed;
extern GameState currentState;

// External functions from game.c
extern void movePlayerAPad(s8 direction);
extern void handleJoypadInput(u8 data);
extern void updateBallPosition(void);
extern void updateGameDisplay(void);
extern void showPauseScreen(void);
extern void updateDifficultyScreen(void);

// External functions from main.c
extern u8 JOYPAD_Read(void);
extern void Delay(u32 count);

/**
  * @brief  USART1 interrupt handler (receives random seed)
  */
void USART1_IRQHandler(void)
{
	u8 temp;
	if(USART1->SR & (1<<5)) // RXNE: Read data register not empty
	{
		temp = USART1->DR; // Read received data
		if(temp >= '0' && temp <= '7') // Validate random seed (0-7)
		{
			randomSeed = temp - '0';
			usartReceived = 1;
		}
	}
}

/**
  * @brief  EXTI2 interrupt handler (KEY2 - Not available on this board)
  */
void EXTI2_IRQHandler(void)
{
	if(EXTI->PR & (1<<2)) // Check EXTI2 pending bit
	{
		// KEY2 not available on this board - no action
		EXTI->PR = 1<<2; // Clear pending bit
	}
}

/**
  * @brief  EXTI3 interrupt handler (KEY1 - Toggle difficulty or Pause)
  */
void EXTI3_IRQHandler(void)
{
	if(EXTI->PR & (1<<3)) // Check EXTI3 pending bit
	{
		Delay(10000); // Debounce delay

		if(currentState == STATE_DIFFICULTY_SELECT)
		{
			difficulty = 1 - difficulty; // Toggle difficulty Easy <-> Hard
			extern void updateDifficultyScreen(void);
			updateDifficultyScreen();
		}
		else if(currentState == STATE_PLAYING)
		{
			currentState = STATE_PAUSED;
			extern void showPauseScreen(void);
			showPauseScreen();
		}
		else if(currentState == STATE_PAUSED)
		{
			currentState = STATE_PLAYING;
			// Clear pause text (fixed coordinates for 2.8" LCD)
			EIE3810_TFTLCD_FillRectangle(60, 120, 140, 30, WHITE);
		}

		EXTI->PR = 1<<3; // Clear pending bit
	}
}

/**
  * @brief  EXTI4 interrupt handler (KEY0 - Start game or move pad)
  */
void EXTI4_IRQHandler(void)
{
	if(EXTI->PR & (1<<4)) // Check EXTI4 pending bit
	{
		Delay(10000); // Debounce delay

		if(currentState == STATE_DIFFICULTY_SELECT)
		{
			// KEY0 starts the game immediately (skip player ready confirmation)
			playerA_ready = 1;
			playerB_ready = 1; // Auto-confirm both players for solo development
			currentState = STATE_WAIT_USART;
			extern void showWaitUSARTScreen(void);
			showWaitUSARTScreen();
		}
		else if(currentState == STATE_PLAYING)
		{
			movePlayerAPad(-1); // Move left
		}

		EXTI->PR = 1<<4; // Clear pending bit
	}
}

/**
  * @brief  TIM3 interrupt handler (Game update and joypad reading)
  */
void TIM3_IRQHandler(void)
{
	if(TIM3->SR & (1<<0)) // Update interrupt flag
	{
		// Read joypad input
		u8 joypadData = JOYPAD_Read();
		if(joypadData != 0)
		{
			// Decode joypad button
			u8 button = 0;
			u8 i;
			for(i = 0; i < 8; i++)
			{
				if((joypadData >> i) == 1)
				{
					button = i + 1;
					break;
				}
			}

			// Handle joypad input based on button index
			if(button == 1) handleJoypadInput('A'); // A button
			else if(button == 2) handleJoypadInput('B'); // B button
			else if(button == 3) handleJoypadInput('S'); // SELECT
			else if(button == 4) handleJoypadInput('T'); // START (pause)
			else if(button == 5) handleJoypadInput('U'); // UP
			else if(button == 6) handleJoypadInput('D'); // DOWN
			else if(button == 7) handleJoypadInput('L'); // LEFT
			else if(button == 8) handleJoypadInput('R'); // RIGHT
		}

		// Update ball position during gameplay
		if(currentState == STATE_PLAYING)
		{
			updateBallPosition();
			updateGameDisplay();
		}
	}
	TIM3->SR &= ~(1<<0); // Clear update interrupt flag
}

/**
  * @brief  TIM4 interrupt handler
  */
void TIM4_IRQHandler(void)
{
	if(TIM4->SR & 1<<0) // Update interrupt flag
	{
		GPIOE->ODR ^= 1<<5; // Toggle GPIOE pin 5
	}
	TIM4->SR &= ~(1<<0); // Clear update interrupt flag
}

/**
  * @}
  */


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
