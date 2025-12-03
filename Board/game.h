/****************************************
 * File: game.h
 * Description: Game logic declarations
 ****************************************/

#ifndef __GAME_H
#define __GAME_H

#include "stm32f10x.h"

// Game states
typedef enum {
    STATE_WELCOME,
    STATE_DIFFICULTY_SELECT,
    STATE_WAIT_USART,
    STATE_COUNTDOWN,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;

// Display functions
void showWelcomeScreen(void);
void showDifficultyScreen(void);
void updateDifficultyScreen(void);
void showWaitUSARTScreen(void);
void startCountdown(void);
void updateGameDisplay(void);
void showPauseScreen(void);
void showGameOverScreen(void);
void showString(u16 x, u16 y, char* str, u16 color, u16 bgcolor);
void showNumber(u16 x, u16 y, u32 num, u8 len, u16 color, u16 bgcolor);

// Game logic functions
void initGame(u8 seed, u8 diff);
void updateBallPosition(void);
void movePlayerAPad(s8 direction);
void movePlayerBPad(s8 direction);
void handleJoypadInput(u8 data);

// Helper functions
u32 powInt(u32 base, u32 exp);

#endif