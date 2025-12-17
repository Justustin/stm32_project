# ECE3080 Introduction to Embedded Systems
# Project Report: Two-Player Bouncing Ball Game

**School of Science and Engineering**
**The Chinese University of Hong Kong, Shenzhen**
**2025-2026 Term 1**

---

## 1. Introduction

This project implements a two-player Pong-style bouncing ball game on an STM32F103ZE microcontroller with a TFT LCD display. The game incorporates GPIO, USART, LCD, external interrupts, and timer functionalities learned throughout Labs 1-5.

---

## 2. Experiment 1: Bouncing Ball Game Implementation

### 2.1 Hardware Configuration

- **Microcontroller**: STM32F103ZE (72MHz)
- **Display**: TFT LCD (480x800 pixels)
- **Input Devices**:
  - KEY0 (PE4), KEY1 (PE3), KEY2 (PE2), KEY_UP (PA0)
  - JOYPAD (via COM3)
- **Output**: Buzzer (PB8)
- **Communication**: USART1

### 2.2 Welcome Screen and Difficulty Selection

**Implementation**: Upon power-on, a welcome screen displays introductory text matching the PDF design (Fig. 3). Players can then select difficulty:

- **Player A Controls**: KEY_UP and KEY1 to toggle between Easy/Hard, KEY0 to confirm ready
- **Player B Controls**: JOYPAD UP/DOWN to select difficulty, SELECT to confirm ready

**Code Implementation** (`game.c`):
```c
void showWelcomeScreen(void) {
    EIE3810_TFTLCD_FillScreen(BLUE);
    showString(40, 200, "Welcome to mini", WHITE, BLUE);
    showString(40, 230, "Project!", WHITE, BLUE);
    showString(40, 300, "This is the Final Lab.", YELLOW, BLUE);
    showString(40, 380, "Are you ready?", YELLOW, BLUE);
    showString(40, 460, "OK! Let's start.", GREEN, BLUE);
    showString(40, 600, "Press KEY0...", WHITE, BLUE);
}

void showDifficultyScreen(void) {
    EIE3810_TFTLCD_FillScreen(WHITE);
    showString(20, 120, "Select difficulty:", RED, WHITE);

    if(difficulty == 0) {
        showString(20, 180, "> Easy", BLUE, WHITE);
        showString(20, 210, "  Hard", BLACK, WHITE);
    } else {
        showString(20, 180, "  Easy", BLACK, WHITE);
        showString(20, 210, "> Hard", BLUE, WHITE);
    }
    // Display player ready status...
}
```

### 2.3 USART Random Seed Reception

**Implementation**: After both players confirm ready, the game waits for a random seed (0-7) via USART1. This seed determines the initial ball direction.

**Code Implementation** (`stm32f10x_it.c`):
```c
void USART1_IRQHandler(void)
{
    if(USART1->SR & (1<<5)) // RXNE flag
    {
        u8 data = USART1->DR;
        if(currentState == STATE_WAIT_USART && data >= '0' && data <= '7')
        {
            randomSeed = data - '0';
            usartReceived = 1;
        }
    }
}
```

### 2.4 Countdown Display

**Implementation**: A 3-second countdown (3, 2, 1, GO!) is displayed before the game starts.

**Code Implementation** (`game.c`):
```c
void startCountdown(void) {
    EIE3810_TFTLCD_FillScreen(WHITE);
    EIE3810_TFTLCD_DrawRectangle(0, 0, SCREEN_WIDTH-1, SCREEN_HEIGHT-1, BLACK);

    showString(200, 380, "  3  ", RED, WHITE);
    DisplayDelay(7500000);
    showString(200, 380, "  2  ", RED, WHITE);
    DisplayDelay(7500000);
    showString(200, 380, "  1  ", RED, WHITE);
    DisplayDelay(7500000);
    showString(180, 380, " GO! ", GREEN, WHITE);
    DisplayDelay(5000000);
}
```

### 2.5 Paddle Control and Ball Bouncing

**Implementation**:
- **Player A**: KEY2 (move right) and KEY0 (move left)
- **Player B**: JOYPAD LEFT/RIGHT

Ball bounces off walls and paddles with buzzer feedback.

**Code Implementation** (`game.c`):
```c
void movePlayerAPad(s8 direction) {
    if(currentState != STATE_PLAYING) return;

    // Clear old pad
    EIE3810_TFTLCD_FillRectangle(padA_x, currentPadWidth, padA_y, PAD_HEIGHT, WHITE);

    padA_x += direction * 10;

    // Boundary checking
    if(padA_x < 0) padA_x = 0;
    if(padA_x > SCREEN_WIDTH - currentPadWidth)
        padA_x = SCREEN_WIDTH - currentPadWidth;

    // Draw new pad
    EIE3810_TFTLCD_FillRectangle(padA_x, currentPadWidth, padA_y, PAD_HEIGHT, YELLOW);
}
```

**Bounce Detection with Buzzer**:
```c
// Wall bounce
if(ballX <= BALL_RADIUS || ballX >= SCREEN_WIDTH - BALL_RADIUS) {
    ballVx = -ballVx;
    bounceCount++;
    Buzzer_On();
    Delay(50000);
    Buzzer_Off();
}

// Paddle bounce
if(ballY + BALL_RADIUS >= padA_y && ballX >= padA_x &&
   ballX <= padA_x + currentPadWidth) {
    ballVy = -ballVy;
    bounceCount++;
    Buzzer_On();
    Delay(50000);
    Buzzer_Off();
}
```

**Buzzer Configuration** (`EIE3810_GPIO.c`):
```c
void Buzzer_Init(void)
{
    RCC->APB2ENR |= 1<<3;  // Enable PORTB clock
    GPIOB->CRH &= 0xFFFFFFF0;
    GPIOB->CRH |= 0x00000003;  // PB8: Output push-pull
    GPIOB->ODR &= ~(1<<8);  // Buzzer off initially
}

void Buzzer_On(void)
{
    GPIOB->ODR |= (1<<8);  // Set HIGH
}

void Buzzer_Off(void)
{
    GPIOB->ODR &= ~(1<<8);  // Set LOW
}
```

### 2.6 HUD Display (Time and Bounces)

**Implementation**: Elapsed time and bounce count are displayed at the top of the screen during gameplay.

**Code Implementation** (`game.c`):
```c
void updateGameDisplay(void) {
    if(currentState != STATE_PLAYING) return;

    static u32 lastDisplayTime = 0;
    if(gameTime - lastDisplayTime >= 100) {
        lastDisplayTime = gameTime;

        // Update Time display
        EIE3810_TFTLCD_FillRectangle(60, 40, 10, 16, WHITE);
        showNumber(60, 10, gameTime / 100, 4, BLACK, WHITE);

        // Update Bounces display
        EIE3810_TFTLCD_FillRectangle(380, 40, 10, 16, WHITE);
        showNumber(380, 10, bounceCount, 4, BLACK, WHITE);
    }
}
```

### 2.7 Game Over Detection

**Implementation**: When the ball passes a player's paddle, that player loses. A game over screen displays the winner and game statistics.

**Code Implementation** (`game.c`):
```c
// Check game over - Player B wins
if(ballY >= SCREEN_HEIGHT) {
    winner = 2;
    currentState = STATE_GAME_OVER;
    gameStarted = 0;
    showGameOverScreen();
    return;
}

// Check game over - Player A wins
if(ballY <= 0) {
    winner = 1;
    currentState = STATE_GAME_OVER;
    gameStarted = 0;
    showGameOverScreen();
    return;
}
```

### 2.8 Pause Functionality

**Implementation**:
- **Player A**: KEY1 to pause/resume
- **Player B**: JOYPAD START to pause/resume

**Code Implementation** (`stm32f10x_it.c`):
```c
void EXTI3_IRQHandler(void)  // KEY1
{
    if(currentState == STATE_PLAYING) {
        currentState = STATE_PAUSED;
        showPauseScreen();
    }
    else if(currentState == STATE_PAUSED) {
        currentState = STATE_PLAYING;
        // Clear pause overlay
        EIE3810_TFTLCD_FillRectangle(140, 200, 360, 40, WHITE);
    }
}
```

---

## 3. Experiment 2: Game Improvements

### 3.1 Progressive Difficulty - Ball Speed Increase

**Description**: The ball speed increases every 5 bounces, making the game progressively harder as players rally longer.

**Implementation** (`game.c`):
```c
#define INITIAL_SPEED_DIVIDER 3  // Initial ball speed (higher = slower)
#define MIN_SPEED_DIVIDER 1      // Maximum ball speed

// In updateBallPosition():
if(bounceCount >= lastSpeedIncreaseBounce + 5) {
    lastSpeedIncreaseBounce = bounceCount;
    if(currentSpeedDivider > MIN_SPEED_DIVIDER) {
        currentSpeedDivider--;  // Faster ball!
    }
}
```

**Result**: The ball starts slow, giving players time to learn the controls, then gradually accelerates to test their reflexes.

### 3.2 Progressive Difficulty - Paddle Shrinking

**Description**: Paddles shrink by 5 pixels every 10 bounces, requiring more precise positioning as the game progresses.

**Implementation** (`game.c`):
```c
#define INITIAL_PAD_WIDTH 100
#define MIN_PAD_WIDTH 40

// In updateBallPosition():
if(bounceCount >= lastPadShrinkBounce + 10) {
    lastPadShrinkBounce = bounceCount;
    if(currentPadWidth > MIN_PAD_WIDTH) {
        // Clear old pads before shrinking
        EIE3810_TFTLCD_FillRectangle(padA_x, currentPadWidth, padA_y, PAD_HEIGHT, WHITE);
        EIE3810_TFTLCD_FillRectangle(padB_x, currentPadWidth, padB_y, PAD_HEIGHT, WHITE);
        currentPadWidth -= 5;
    }
}
```

**Result**: Combined with speed increases, this creates exponentially increasing difficulty.

### 3.3 Angle-Based Reflection

**Description**: Ball reflection angle depends on where it hits the paddle, giving players strategic control:
- **Left third**: Ball bounces left
- **Middle third**: Ball maintains direction
- **Right third**: Ball bounces right

**Implementation** (`game.c`):
```c
// Calculate hit position on paddle
s16 hitPos = ballX - padA_x;

if(hitPos < currentPadWidth / 3) {
    ballVx = -1;  // Left third = go left
} else if(hitPos > (currentPadWidth * 2) / 3) {
    ballVx = 1;   // Right third = go right
}
// Middle third keeps current direction
```

**Result**: Players can aim their shots strategically, adding skill-based gameplay beyond simple reaction time.

### 3.4 Power-Up System

**Description**: A comprehensive power-up system with 4 types that spawn randomly during gameplay:

| Type | Color | Effect | Duration |
|------|-------|--------|----------|
| Speed Up | Red | Ball speeds up | 500 frames |
| Speed Down | Green | Ball slows down | 500 frames |
| Paddle Grow | Cyan | Paddles grow +15px | Permanent |
| Paddle Shrink | Magenta | Paddles shrink -10px | Permanent |

**Implementation** (`game.c`):
```c
#define POWERUP_SPEED_UP 1
#define POWERUP_SPEED_DOWN 2
#define POWERUP_PAD_GROW 3
#define POWERUP_PAD_SHRINK 4
#define POWERUP_SPAWN_INTERVAL 300

void spawnPowerup(void) {
    if(powerupActive) return;

    // Random position in play area
    powerupX = 30 + (randomValue() % (SCREEN_WIDTH - 60));
    powerupY = 80 + (randomValue() % (SCREEN_HEIGHT - 160));

    // Random type (1-4)
    powerupType = 1 + (randomValue() % 4);
    powerupActive = 1;
}

void applyPowerup(u8 type) {
    activePowerupType = type;
    powerupTimer = POWERUP_DURATION;

    switch(type) {
        case POWERUP_SPEED_UP:
            currentSpeedDivider = MIN_SPEED_DIVIDER;
            break;
        case POWERUP_SPEED_DOWN:
            currentSpeedDivider = INITIAL_SPEED_DIVIDER + 1;
            break;
        case POWERUP_PAD_GROW:
            currentPadWidth += 15;
            break;
        case POWERUP_PAD_SHRINK:
            currentPadWidth -= 10;
            break;
    }

    // Audio feedback
    Buzzer_On();
    Delay(10000);
    Buzzer_Off();
}
```

**Power-up Display**:
```c
void drawPowerup(void) {
    if(!powerupActive) return;

    u16 color = getPowerupColor(powerupType);
    EIE3810_TFTLCD_FillRectangle(powerupX, POWERUP_SIZE, powerupY, POWERUP_SIZE, color);

    // Draw symbol (S for speed, P for paddle)
    if(powerupType == POWERUP_SPEED_UP || powerupType == POWERUP_SPEED_DOWN) {
        EIE3810_TFTLCD_ShowChar(powerupX + 2, powerupY, 'S', WHITE, color);
    } else {
        EIE3810_TFTLCD_ShowChar(powerupX + 2, powerupY, 'P', WHITE, color);
    }
}
```

**Result**: Power-ups add unpredictability and strategic decision-making. Players must decide whether to intercept beneficial power-ups or avoid harmful ones.

---

## 4. State Machine Design

The game uses a finite state machine with the following states:

```
STATE_WELCOME → STATE_DIFFICULTY_SELECT → STATE_WAIT_USART →
STATE_COUNTDOWN → STATE_PLAYING ⟷ STATE_PAUSED → STATE_GAME_OVER
                                                        ↓
                                          (Returns to STATE_DIFFICULTY_SELECT)
```

**Implementation** (`game.h`):
```c
typedef enum {
    STATE_WELCOME,
    STATE_DIFFICULTY_SELECT,
    STATE_WAIT_USART,
    STATE_COUNTDOWN,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} GameState;
```

---

## 5. Interrupt Configuration

### External Interrupts (EXTI)

| Interrupt | Pin | Trigger | Function |
|-----------|-----|---------|----------|
| EXTI0 | PA0 (KEY_UP) | Rising edge | Toggle difficulty |
| EXTI2 | PE2 (KEY2) | Falling edge | Move Player A pad right |
| EXTI3 | PE3 (KEY1) | Falling edge | Toggle difficulty / Pause |
| EXTI4 | PE4 (KEY0) | Falling edge | Confirm / Move pad left |

### Timer Interrupt (TIM3)
- **Period**: 10ms
- **Function**: Calls `updateBallPosition()` and `updateGameDisplay()`

### USART Interrupt (USART1)
- **Function**: Receives random seed (0-7) for initial ball direction

---

## 6. Control Summary

| Action | Player A | Player B |
|--------|----------|----------|
| Select Difficulty | KEY_UP / KEY1 | JOYPAD UP/DOWN |
| Confirm Ready | KEY0 | JOYPAD SELECT |
| Move Pad Left | KEY0 | JOYPAD LEFT |
| Move Pad Right | KEY2 | JOYPAD RIGHT |
| Pause/Resume | KEY1 | JOYPAD START |

---

## 7. Conclusion

This project successfully implements a fully functional two-player Pong game with the following features:

1. **Complete game flow** as specified in sections 2.2-2.8 of the handout
2. **Progressive difficulty** through automatic speed increases and paddle shrinking
3. **Strategic gameplay** with angle-based ball reflection
4. **Dynamic elements** with a 4-type power-up system
5. **Audio feedback** via buzzer for all bounce events
6. **Robust state management** using a finite state machine

The improvements make the game significantly more challenging and engaging compared to a basic Pong implementation. The combination of progressive difficulty, strategic aiming, and random power-ups creates varied gameplay that remains interesting across multiple sessions.

---

## 8. File Structure

```
stm32_project/
├── User/
│   ├── main.c              # Main game loop and initialization
│   └── stm32f10x_it.c      # Interrupt handlers
├── Board/
│   ├── game.c              # Game logic and display functions
│   ├── game.h              # Game state definitions
│   ├── EIE3810_TFTLCD.c    # LCD driver
│   ├── EIE3810_TFTLCD.h    # LCD function declarations
│   ├── EIE3810_GPIO.c      # GPIO (buzzer, keys) configuration
│   └── EIE3810_GPIO.h      # GPIO function declarations
└── Font.H                  # Font definitions (12x6, 16x8, 24x12)
```

---

*Report generated for ECE3080 Introduction to Embedded Systems Project*
