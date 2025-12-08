#include "game.h"
#include "EIE3810_TFTLCD.h"
#include "EIE3810_GPIO.h"

// External functions
extern void Delay(u32 count);

// External variables
extern GameState currentState;
extern u8 difficulty;
extern u8 playerA_ready;
extern u8 playerB_ready;

// Game constants
#define BALL_RADIUS 8
#define PAD_WIDTH 60
#define PAD_HEIGHT 10
#define SCREEN_WIDTH 240    // 2.8" TFT LCD width
#define SCREEN_HEIGHT 320   // 2.8" TFT LCD height

// Ball state
u16 ballX = 120;
u16 ballY = 160;
s8 ballVx = 2;
s8 ballVy = 2;
u16 oldBallX = 120;
u16 oldBallY = 160;

// Pad positions
u16 padA_x = 90;   // Player A (bottom) - centered
u16 padB_x = 90;   // Player B (top) - centered
u16 padA_y = 300;  // Near bottom
u16 padB_y = 10;   // Near top

// Game stats
u32 gameTime = 0;
u16 bounceCount = 0;
u8 gameStarted = 0;
u8 winner = 0;
u8 speedMultiplier = 1;

/****************************************
 * Helper Functions
 ****************************************/

u32 powInt(u32 base, u32 exp) {
    u32 result = 1;
    while(exp--) {
        result *= base;
    }
    return result;
}

void showString(u16 x, u16 y, char* str, u16 color, u16 bgcolor) {
    EIE3810_TFTLCD_ShowString2412(x, y, str, color, bgcolor);
}

void showNumber(u16 x, u16 y, u32 num, u8 len, u16 color, u16 bgcolor) {
    u8 i;
    u8 temp;
    u8 enshow = 0;
    
    for(i = 0; i < len; i++) {
        temp = (num / powInt(10, len - i - 1)) % 10;
        if(enshow == 0 && i < (len - 1)) {
            if(temp == 0) {
                EIE3810_TFTLCD_ShowChar2412(x + 12 * i, y, ' ', color, bgcolor);
                continue;
            } else {
                enshow = 1;
            }
        }
        EIE3810_TFTLCD_ShowChar2412(x + 12 * i, y, temp + '0', color, bgcolor);
    }
}

void clearBall(void) {
    EIE3810_TFTLCD_DrawCircle(oldBallX, oldBallY, BALL_RADIUS + 1, 1, WHITE);
}

void drawBall(void) {
    EIE3810_TFTLCD_DrawCircle(ballX, ballY, BALL_RADIUS, 1, RED);
}

void drawPads(void) {
    // Player A pad (bottom) - Yellow
    // YOUR function signature: (start_x, length_x, start_y, length_y, color)
    EIE3810_TFTLCD_FillRectangle(padA_x, PAD_WIDTH, padA_y, PAD_HEIGHT, YELLOW);
    
    // Player B pad (top) - Blue
    EIE3810_TFTLCD_FillRectangle(padB_x, PAD_WIDTH, padB_y, PAD_HEIGHT, BLUE);
}

/****************************************
 * Display Functions
 ****************************************/

void showWelcomeScreen(void) {
    EIE3810_TFTLCD_FillScreen(WHITE);
    showString(80, 200, "Welcome to mini Project!", BLUE, WHITE);
    showString(80, 250, "This is the Final Lab", RED, WHITE);
    showString(80, 300, "Are you ready?", RED, WHITE);
    showString(80, 350, "3...2...1...Let's start", RED, WHITE);
}

void showDifficultyScreen(void) {
    EIE3810_TFTLCD_FillScreen(WHITE);
    showString(50, 150, "Please select difficulty:", RED, WHITE);
    
    if(difficulty == 0) {
        showString(50, 250, "Easy", BLUE, WHITE);
        showString(50, 300, "Hard", BLACK, WHITE);
    } else {
        showString(50, 250, "Easy", BLACK, WHITE);
        showString(50, 300, "Hard", BLUE, WHITE);
    }
    
    showString(50, 400, "Press KEY0 to enter.", RED, WHITE);
    
    if(playerA_ready) {
        showString(50, 500, "Player A: Ready", GREEN, WHITE);
    }
    if(playerB_ready) {
        showString(50, 530, "Player B: Ready", GREEN, WHITE);
    }
}

void updateDifficultyScreen(void) {
    showDifficultyScreen();
}

void showWaitUSARTScreen(void) {
    EIE3810_TFTLCD_FillScreen(WHITE);
    showString(50, 350, "Use USART for a random", RED, WHITE);
    showString(50, 380, "direction.", RED, WHITE);
}

void startCountdown(void) {
    EIE3810_TFTLCD_FillScreen(WHITE);
    
    drawPads();
    EIE3810_TFTLCD_DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    
    showString(220, 380, "3", RED, WHITE);
    Delay(1000000);
    
    EIE3810_TFTLCD_FillRectangle(220, 24, 380, 24, WHITE);  // Clear
    showString(220, 380, "2", RED, WHITE);
    Delay(1000000);
    
    EIE3810_TFTLCD_FillRectangle(220, 24, 380, 24, WHITE);
    showString(220, 380, "1", RED, WHITE);
    Delay(1000000);
    
    EIE3810_TFTLCD_FillRectangle(220, 24, 380, 24, WHITE);
    showString(200, 380, "GO!", GREEN, WHITE);
    Delay(500000);
    
    EIE3810_TFTLCD_FillRectangle(200, 60, 380, 24, WHITE);
}

void showPauseScreen(void) {
    EIE3810_TFTLCD_FillRectangle(150, 180, 380, 40, BLACK);
    showString(180, 390, "PAUSED", YELLOW, BLACK);
}

void showGameOverScreen(void) {
    EIE3810_TFTLCD_FillScreen(BLACK);
    
    if(winner == 1) {
        showString(120, 300, "Player A Wins!", GREEN, BLACK);
    } else if(winner == 2) {
        showString(120, 300, "Player B Wins!", GREEN, BLACK);
    }
    
    showString(100, 400, "Restarting...", WHITE, BLACK);
}

/****************************************
 * Game Logic
 ****************************************/

void initGame(u8 seed, u8 diff) {
    ballX = SCREEN_WIDTH / 2;
    ballY = SCREEN_HEIGHT / 2;
    oldBallX = ballX;
    oldBallY = ballY;
    
    padA_x = (SCREEN_WIDTH - PAD_WIDTH) / 2;
    padB_x = (SCREEN_WIDTH - PAD_WIDTH) / 2;
    
    gameTime = 0;
    bounceCount = 0;
    gameStarted = 1;
    winner = 0;
    
    if(diff == 0) {
        speedMultiplier = 1;
    } else {
        speedMultiplier = 2;
    }
    
    // Set velocity based on seed (0-7)
    switch(seed % 8) {
        case 0: ballVx = 2 * speedMultiplier; ballVy = 2 * speedMultiplier; break;
        case 1: ballVx = 0; ballVy = 3 * speedMultiplier; break;
        case 2: ballVx = -2 * speedMultiplier; ballVy = 2 * speedMultiplier; break;
        case 3: ballVx = -3 * speedMultiplier; ballVy = 0; break;
        case 4: ballVx = -2 * speedMultiplier; ballVy = -2 * speedMultiplier; break;
        case 5: ballVx = 0; ballVy = -3 * speedMultiplier; break;
        case 6: ballVx = 2 * speedMultiplier; ballVy = -2 * speedMultiplier; break;
        case 7: ballVx = 3 * speedMultiplier; ballVy = 0; break;
    }
    
    EIE3810_TFTLCD_FillScreen(WHITE);
    drawPads();
    drawBall();
    
    showString(10, 10, "Time:", BLACK, WHITE);
    showString(300, 10, "Bounces:", BLACK, WHITE);
}

void updateBallPosition(void) {
    if(!gameStarted || currentState != STATE_PLAYING) return;
    
    gameTime++;
    
    oldBallX = ballX;
    oldBallY = ballY;
    
    ballX += ballVx;
    ballY += ballVy;
    
    // Left/right walls
    if(ballX <= BALL_RADIUS || ballX >= SCREEN_WIDTH - BALL_RADIUS) {
        ballVx = -ballVx;
        if(ballX <= BALL_RADIUS) ballX = BALL_RADIUS;
        if(ballX >= SCREEN_WIDTH - BALL_RADIUS) ballX = SCREEN_WIDTH - BALL_RADIUS;
        bounceCount++;
        Buzzer_On();
        Delay(5000);
        Buzzer_Off();
    }
    
    // Player A's pad (bottom)
    if(ballY + BALL_RADIUS >= padA_y && 
       ballY + BALL_RADIUS <= padA_y + PAD_HEIGHT &&
       ballX >= padA_x && 
       ballX <= padA_x + PAD_WIDTH) {
        ballVy = -ballVy;
        ballY = padA_y - BALL_RADIUS;
        bounceCount++;
        Buzzer_On();
        Delay(5000);
        Buzzer_Off();
    }
    
    // Player B's pad (top)
    if(ballY - BALL_RADIUS <= padB_y + PAD_HEIGHT && 
       ballY - BALL_RADIUS >= padB_y &&
       ballX >= padB_x && 
       ballX <= padB_x + PAD_WIDTH) {
        ballVy = -ballVy;
        ballY = padB_y + PAD_HEIGHT + BALL_RADIUS;
        bounceCount++;
        Buzzer_On();
        Delay(5000);
        Buzzer_Off();
    }
    
    // Check game over
    if(ballY >= SCREEN_HEIGHT) {
        winner = 2;
        currentState = STATE_GAME_OVER;
        gameStarted = 0;
        return;
    }
    
    if(ballY <= 0) {
        winner = 1;
        currentState = STATE_GAME_OVER;
        gameStarted = 0;
        return;
    }
    
    clearBall();
    drawBall();
}

void movePlayerAPad(s8 direction) {
    if(currentState != STATE_PLAYING) return;
    
    // Clear old - YOUR function: (start_x, length_x, start_y, length_y, color)
    EIE3810_TFTLCD_FillRectangle(padA_x, PAD_WIDTH, padA_y, PAD_HEIGHT, WHITE);
    
    padA_x += direction * 10;
    
    if(padA_x < 0) padA_x = 0;
    if(padA_x > SCREEN_WIDTH - PAD_WIDTH) padA_x = SCREEN_WIDTH - PAD_WIDTH;
    
    EIE3810_TFTLCD_FillRectangle(padA_x, PAD_WIDTH, padA_y, PAD_HEIGHT, YELLOW);
}

void movePlayerBPad(s8 direction) {
    if(currentState != STATE_PLAYING) return;
    
    EIE3810_TFTLCD_FillRectangle(padB_x, PAD_WIDTH, padB_y, PAD_HEIGHT, WHITE);
    
    padB_x += direction * 10;
    
    if(padB_x < 0) padB_x = 0;
    if(padB_x > SCREEN_WIDTH - PAD_WIDTH) padB_x = SCREEN_WIDTH - PAD_WIDTH;
    
    EIE3810_TFTLCD_FillRectangle(padB_x, PAD_WIDTH, padB_y, PAD_HEIGHT, BLUE);
}

void handleJoypadInput(u8 data) {
    if(currentState == STATE_DIFFICULTY_SELECT) {
        if(data == 'U' || data == 'D') {
            difficulty = 1 - difficulty;
            updateDifficultyScreen();
        }
        else if(data == 'S') {
            playerB_ready = 1;
            updateDifficultyScreen();
            if(playerA_ready) {
                currentState = STATE_WAIT_USART;
            }
        }
    }
    else if(currentState == STATE_PLAYING) {
        if(data == 'L') {
            movePlayerBPad(-1);
        }
        else if(data == 'R') {
            movePlayerBPad(1);
        }
        else if(data == 'T') {
            currentState = STATE_PAUSED;
        }
    }
    else if(currentState == STATE_PAUSED) {
        if(data == 'T') {
            currentState = STATE_PLAYING;
            EIE3810_TFTLCD_FillRectangle(150, 180, 380, 40, WHITE);
        }
    }
}

void updateGameDisplay(void) {
    if(currentState != STATE_PLAYING) return;
    
    static u32 lastDisplayTime = 0;
    if(gameTime - lastDisplayTime >= 100) {
        lastDisplayTime = gameTime;
        
        EIE3810_TFTLCD_FillRectangle(80, 100, 10, 24, WHITE);
        showNumber(80, 10, gameTime / 100, 3, BLACK, WHITE);
        
        EIE3810_TFTLCD_FillRectangle(420, 50, 10, 24, WHITE);
        showNumber(420, 10, bounceCount, 3, BLACK, WHITE);
    }
}