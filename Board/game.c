#include "game.h"
#include "EIE3810_TFTLCD.h"
#include "EIE3810_GPIO.h"

// External functions
extern void Delay(u32 count);

// External variables (volatile - modified in interrupts)
extern volatile GameState currentState;
extern volatile u8 difficulty;
extern volatile u8 playerA_ready;
extern volatile u8 playerB_ready;

// Game constants
#define BALL_RADIUS 6
#define PAD_WIDTH 50      // Pong-style paddle width
#define PAD_HEIGHT 8      // Pong-style paddle height
#define SCREEN_WIDTH 240    // 2.8" TFT LCD width
#define SCREEN_HEIGHT 320   // 2.8" TFT LCD height
#define BALL_SPEED_DIVIDER 3  // Ball moves every N frames (higher = slower)

// Ball state
u16 ballX = 120;
u16 ballY = 160;
s8 ballVx = 1;
s8 ballVy = 1;
u16 oldBallX = 120;
u16 oldBallY = 160;
u8 frameCounter = 0;  // For slowing down ball

// Pad positions
u16 padA_x = 95;   // Player A (bottom) - centered
u16 padB_x = 95;   // Player B (top) - centered
u16 padA_y = 305;  // Near bottom
u16 padB_y = 8;    // Near top

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
    EIE3810_TFTLCD_ShowString1608(x, y, str, color, bgcolor);  // Use smaller 8x16 font
}

void showNumber(u16 x, u16 y, u32 num, u8 len, u16 color, u16 bgcolor) {
    u8 i;
    u8 temp;
    u8 enshow = 0;

    for(i = 0; i < len; i++) {
        temp = (num / powInt(10, len - i - 1)) % 10;
        if(enshow == 0 && i < (len - 1)) {
            if(temp == 0) {
                EIE3810_TFTLCD_ShowChar(x + 8 * i, y, ' ', color, bgcolor);
                continue;
            } else {
                enshow = 1;
            }
        }
        EIE3810_TFTLCD_ShowChar(x + 8 * i, y, temp + '0', color, bgcolor);
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
    EIE3810_TFTLCD_FillScreen(BLUE);

    // Match PDF design (Fig. 3)
    showString(20, 80, "Welcome to mini", WHITE, BLUE);
    showString(20, 100, "Project!", WHITE, BLUE);

    showString(20, 140, "This is the Final Lab.", YELLOW, BLUE);

    showString(20, 180, "Are you ready?", YELLOW, BLUE);

    showString(20, 220, "OK! Let's start.", GREEN, BLUE);

    showString(20, 280, "Press KEY0...", WHITE, BLUE);
}

void showDifficultyScreen(void) {
    EIE3810_TFTLCD_FillScreen(WHITE);
    showString(10, 60, "Select difficulty:", RED, WHITE);

    if(difficulty == 0) {
        showString(10, 100, "> Easy", BLUE, WHITE);
        showString(10, 120, "  Hard", BLACK, WHITE);
    } else {
        showString(10, 100, "  Easy", BLACK, WHITE);
        showString(10, 120, "> Hard", BLUE, WHITE);
    }

    showString(10, 160, "KEY1: Toggle", RED, WHITE);
    showString(10, 180, "KEY0: Player A Ready", RED, WHITE);
    showString(10, 200, "KEY_UP: Player B Ready", RED, WHITE);

    if(playerA_ready) {
        showString(10, 240, "Player A: Ready", GREEN, WHITE);
    } else {
        showString(10, 240, "Player A: Not Ready", GRAY, WHITE);
    }
    if(playerB_ready) {
        showString(10, 260, "Player B: Ready", GREEN, WHITE);
    } else {
        showString(10, 260, "Player B: Not Ready", GRAY, WHITE);
    }
}

void updateDifficultyScreen(void) {
    showDifficultyScreen();
}

void showWaitUSARTScreen(void) {
    EIE3810_TFTLCD_FillScreen(BLUE);
    showString(10, 120, "Use USART for a", YELLOW, BLUE);
    showString(10, 140, "random direction.", YELLOW, BLUE);
    showString(10, 180, "Send value 0-7", WHITE, BLUE);
}

void showSeedReceivedScreen(u8 seed) {
    EIE3810_TFTLCD_FillScreen(BLUE);
    showString(30, 100, "Random seed:", WHITE, BLUE);
    showNumber(180, 100, seed, 1, YELLOW, BLUE);
    showString(30, 150, "Direction set!", GREEN, BLUE);
    showString(30, 200, "Get ready...", WHITE, BLUE);
}

void startCountdown(void) {
    EIE3810_TFTLCD_FillScreen(WHITE);
    EIE3810_TFTLCD_DrawRectangle(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BLACK);

    // Show "3"
    showString(100, 140, "  3  ", RED, WHITE);
    Delay(15000000);  // ~1.5 seconds

    // Show "2"
    showString(100, 140, "  2  ", RED, WHITE);
    Delay(15000000);  // ~1.5 seconds

    // Show "1"
    showString(100, 140, "  1  ", RED, WHITE);
    Delay(15000000);  // ~1.5 seconds

    // Show "GO!"
    showString(90, 140, " GO! ", GREEN, WHITE);
    Delay(10000000);  // ~1 second
}

void showPauseScreen(void) {
    EIE3810_TFTLCD_FillRectangle(60, 120, 140, 30, BLACK);
    showString(80, 150, "PAUSED", YELLOW, BLACK);
}

void showGameOverScreen(void) {
    EIE3810_TFTLCD_FillScreen(BLACK);

    showString(60, 60, "GAME OVER", RED, BLACK);

    if(winner == 1) {
        showString(40, 120, "Player A Wins!", GREEN, BLACK);
    } else if(winner == 2) {
        showString(40, 120, "Player B Wins!", GREEN, BLACK);
    }

    // Show game stats
    showString(20, 180, "Time:", WHITE, BLACK);
    showNumber(80, 180, gameTime / 100, 5, YELLOW, BLACK);

    showString(20, 210, "Bounces:", WHITE, BLACK);
    showNumber(100, 210, bounceCount, 4, YELLOW, BLACK);

    showString(20, 270, "Press KEY0 to restart", WHITE, BLACK);
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
    
    // Reset frame counter
    frameCounter = 0;

    // Set velocity based on seed (0-7)
    // Ball moves 1 pixel every BALL_SPEED_DIVIDER frames
    switch(seed % 8) {
        case 0: ballVx = 1; ballVy = 1; break;   // Right, down
        case 1: ballVx = 0; ballVy = 1; break;   // Straight down
        case 2: ballVx = -1; ballVy = 1; break;  // Left, down
        case 3: ballVx = -1; ballVy = 1; break;  // Left, down
        case 4: ballVx = -1; ballVy = -1; break; // Left, up
        case 5: ballVx = 0; ballVy = -1; break;  // Straight up
        case 6: ballVx = 1; ballVy = -1; break;  // Right, up
        case 7: ballVx = 1; ballVy = -1; break;  // Right, up
    }
    
    EIE3810_TFTLCD_FillScreen(WHITE);
    drawPads();
    drawBall();

    // HUD display - fit on 240px width
    showString(5, 5, "Time:", BLACK, WHITE);
    showString(130, 5, "Bounces:", BLACK, WHITE);
}

void updateBallPosition(void) {
    if(!gameStarted || currentState != STATE_PLAYING) return;

    gameTime++;

    // Frame skipping for slower ball movement
    frameCounter++;
    if(frameCounter < BALL_SPEED_DIVIDER) {
        return; // Skip this frame
    }
    frameCounter = 0;

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
    
    // Check game over - Player B wins (ball passed Player A)
    if(ballY >= SCREEN_HEIGHT) {
        winner = 2;
        currentState = STATE_GAME_OVER;
        gameStarted = 0;
        showGameOverScreen();
        return;
    }

    // Check game over - Player A wins (ball passed Player B)
    if(ballY <= 0) {
        winner = 1;
        currentState = STATE_GAME_OVER;
        gameStarted = 0;
        showGameOverScreen();
        return;
    }
    
    clearBall();
    drawBall();

    // Redraw pads in case ball clearing erased part of them
    drawPads();
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
            // Check if both players are ready to proceed
            if(playerA_ready) {
                currentState = STATE_WAIT_USART;
                showWaitUSARTScreen();
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
            // Clear pause text (fixed coordinates for 2.8" LCD)
            EIE3810_TFTLCD_FillRectangle(60, 120, 140, 30, WHITE);
        }
    }
}

void updateGameDisplay(void) {
    if(currentState != STATE_PLAYING) return;
    
    static u32 lastDisplayTime = 0;
    if(gameTime - lastDisplayTime >= 100) {
        lastDisplayTime = gameTime;

        // Update Time display (3 digits * 8px = 24px wide, 16px tall)
        EIE3810_TFTLCD_FillRectangle(45, 24, 5, 16, WHITE);
        showNumber(45, 5, gameTime / 100, 3, BLACK, WHITE);

        // Update Bounces display
        EIE3810_TFTLCD_FillRectangle(200, 24, 5, 16, WHITE);
        showNumber(200, 5, bounceCount, 3, BLACK, WHITE);
    }
}