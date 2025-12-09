#include "game.h"
#include "EIE3810_TFTLCD.h"
#include "EIE3810_GPIO.h"

// External functions
extern void Delay(u32 count);
extern void DisplayDelay(u32 count);

// External variables (volatile - modified in interrupts)
extern volatile GameState currentState;
extern volatile u8 difficulty;
extern volatile u8 playerA_ready;
extern volatile u8 playerB_ready;

// Game constants
#define BALL_RADIUS 6
#define INITIAL_PAD_WIDTH 50  // Starting paddle width
#define MIN_PAD_WIDTH 20      // Minimum paddle width
#define PAD_HEIGHT 8          // Pong-style paddle height
#define SCREEN_WIDTH 240      // 2.8" TFT LCD width
#define SCREEN_HEIGHT 320     // 2.8" TFT LCD height
#define INITIAL_SPEED_DIVIDER 3  // Initial ball speed (higher = slower)
#define MIN_SPEED_DIVIDER 1      // Maximum ball speed

// Ball state
u16 ballX = 120;
u16 ballY = 160;
s8 ballVx = 1;
s8 ballVy = 1;
u16 oldBallX = 120;
u16 oldBallY = 160;
u8 frameCounter = 0;  // For slowing down ball
u8 currentSpeedDivider = INITIAL_SPEED_DIVIDER;  // Current ball speed

// Pad positions and size
u16 padA_x = 95;   // Player A (bottom) - centered
u16 padB_x = 95;   // Player B (top) - centered
u16 padA_y = 305;  // Near bottom
u16 padB_y = 8;    // Near top
u8 currentPadWidth = INITIAL_PAD_WIDTH;  // Current paddle width (shrinks over time)

// Game stats
u32 gameTime = 0;
u16 bounceCount = 0;
u8 gameStarted = 0;
u8 winner = 0;
u8 speedMultiplier = 1;
u16 lastSpeedIncreaseBounce = 0;   // Track when speed last increased
u16 lastPadShrinkBounce = 0;       // Track when pads last shrunk

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
    EIE3810_TFTLCD_FillRectangle(padA_x, currentPadWidth, padA_y, PAD_HEIGHT, YELLOW);

    // Player B pad (top) - Blue
    EIE3810_TFTLCD_FillRectangle(padB_x, currentPadWidth, padB_y, PAD_HEIGHT, BLUE);
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
    DisplayDelay(7500000);  // ~0.75 seconds

    // Show "2"
    showString(100, 140, "  2  ", RED, WHITE);
    DisplayDelay(7500000);  // ~0.75 seconds

    // Show "1"
    showString(100, 140, "  1  ", RED, WHITE);
    DisplayDelay(7500000);  // ~0.75 seconds

    // Show "GO!"
    showString(90, 140, " GO! ", GREEN, WHITE);
    DisplayDelay(5000000);  // ~0.5 seconds
}

void showPauseScreen(void) {
    EIE3810_TFTLCD_FillRectangle(60, 120, 140, 30, BLACK);
    showString(80, 150, "PAUSED", YELLOW, BLACK);
}

void showGameOverScreen(void) {
    EIE3810_TFTLCD_FillScreen(BLACK);

    showString(60, 40, "GAME OVER", RED, BLACK);

    if(winner == 1) {
        showString(40, 80, "Player A Wins!", GREEN, BLACK);
    } else if(winner == 2) {
        showString(40, 80, "Player B Wins!", GREEN, BLACK);
    }

    // Show game stats
    showString(20, 130, "Time:", WHITE, BLACK);
    showNumber(80, 130, gameTime / 100, 5, YELLOW, BLACK);

    showString(20, 160, "Bounces:", WHITE, BLACK);
    showNumber(100, 160, bounceCount, 4, YELLOW, BLACK);

    // Show final difficulty stats
    showString(20, 190, "Final Speed:", WHITE, BLACK);
    showNumber(130, 190, INITIAL_SPEED_DIVIDER - currentSpeedDivider + 1, 1, YELLOW, BLACK);

    showString(20, 220, "Pad Size:", WHITE, BLACK);
    showNumber(100, 220, currentPadWidth, 2, YELLOW, BLACK);

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

    // Reset paddle width and positions
    currentPadWidth = INITIAL_PAD_WIDTH;
    padA_x = (SCREEN_WIDTH - currentPadWidth) / 2;
    padB_x = (SCREEN_WIDTH - currentPadWidth) / 2;

    gameTime = 0;
    bounceCount = 0;
    gameStarted = 1;
    winner = 0;

    // Reset speed and tracking variables
    currentSpeedDivider = INITIAL_SPEED_DIVIDER;
    lastSpeedIncreaseBounce = 0;
    lastPadShrinkBounce = 0;

    if(diff == 0) {
        speedMultiplier = 1;
    } else {
        speedMultiplier = 2;
        // Hard mode starts faster
        currentSpeedDivider = 2;
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

    // Frame skipping for slower ball movement (uses dynamic speed)
    frameCounter++;
    if(frameCounter < currentSpeedDivider) {
        return; // Skip this frame
    }
    frameCounter = 0;

    oldBallX = ballX;
    oldBallY = ballY;

    ballX += ballVx;
    ballY += ballVy;

    // ========== IMPROVEMENT: Speed increase every 5 bounces ==========
    if(bounceCount >= lastSpeedIncreaseBounce + 5) {
        lastSpeedIncreaseBounce = bounceCount;
        if(currentSpeedDivider > MIN_SPEED_DIVIDER) {
            currentSpeedDivider--;  // Faster ball!
        }
    }

    // ========== IMPROVEMENT: Shrink paddles every 10 bounces ==========
    if(bounceCount >= lastPadShrinkBounce + 10) {
        lastPadShrinkBounce = bounceCount;
        if(currentPadWidth > MIN_PAD_WIDTH) {
            // Clear old pads before shrinking
            EIE3810_TFTLCD_FillRectangle(padA_x, currentPadWidth, padA_y, PAD_HEIGHT, WHITE);
            EIE3810_TFTLCD_FillRectangle(padB_x, currentPadWidth, padB_y, PAD_HEIGHT, WHITE);
            currentPadWidth -= 5;  // Shrink by 5 pixels
        }
    }

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

    // Player A's pad (bottom) with angle-based reflection
    if(ballY + BALL_RADIUS >= padA_y &&
       ballY + BALL_RADIUS <= padA_y + PAD_HEIGHT &&
       ballX >= padA_x &&
       ballX <= padA_x + currentPadWidth) {
        ballVy = -ballVy;
        ballY = padA_y - BALL_RADIUS;

        // ========== IMPROVEMENT: Angle-based reflection ==========
        // Hit left edge = ball goes left, hit right edge = ball goes right
        s16 hitPos = ballX - padA_x;  // Position on paddle (0 to currentPadWidth)
        if(hitPos < currentPadWidth / 3) {
            ballVx = -1;  // Left third = go left
        } else if(hitPos > (currentPadWidth * 2) / 3) {
            ballVx = 1;   // Right third = go right
        }
        // Middle third keeps current direction

        bounceCount++;
        Buzzer_On();
        Delay(5000);
        Buzzer_Off();
    }

    // Player B's pad (top) with angle-based reflection
    if(ballY - BALL_RADIUS <= padB_y + PAD_HEIGHT &&
       ballY - BALL_RADIUS >= padB_y &&
       ballX >= padB_x &&
       ballX <= padB_x + currentPadWidth) {
        ballVy = -ballVy;
        ballY = padB_y + PAD_HEIGHT + BALL_RADIUS;

        // ========== IMPROVEMENT: Angle-based reflection ==========
        s16 hitPos = ballX - padB_x;
        if(hitPos < currentPadWidth / 3) {
            ballVx = -1;  // Left third = go left
        } else if(hitPos > (currentPadWidth * 2) / 3) {
            ballVx = 1;   // Right third = go right
        }

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

    // Clear old pad
    EIE3810_TFTLCD_FillRectangle(padA_x, currentPadWidth, padA_y, PAD_HEIGHT, WHITE);

    padA_x += direction * 10;

    if(padA_x < 0) padA_x = 0;
    if(padA_x > SCREEN_WIDTH - currentPadWidth) padA_x = SCREEN_WIDTH - currentPadWidth;

    EIE3810_TFTLCD_FillRectangle(padA_x, currentPadWidth, padA_y, PAD_HEIGHT, YELLOW);
}

void movePlayerBPad(s8 direction) {
    if(currentState != STATE_PLAYING) return;

    EIE3810_TFTLCD_FillRectangle(padB_x, currentPadWidth, padB_y, PAD_HEIGHT, WHITE);

    padB_x += direction * 10;

    if(padB_x < 0) padB_x = 0;
    if(padB_x > SCREEN_WIDTH - currentPadWidth) padB_x = SCREEN_WIDTH - currentPadWidth;

    EIE3810_TFTLCD_FillRectangle(padB_x, currentPadWidth, padB_y, PAD_HEIGHT, BLUE);
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