/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"

// Display
#define SDA_PIN 0  // SDA is on GPIO 0
#define SCL_PIN 1  // SCL is on GPIO 1
#define I2C_PORT i2c0  // Use i2c1

// Input
#define BUTTON_UP 2
#define BUTTON_DOWN 3
#define BUTTON_LEFT 4
#define BUTTON_RIGHT 5
#define BUTTON_SELECT 6
#define BUTTON_START 7


#define SCREEN_WIDTH    120
#define SCREEN_HEIGHT   64

ssd1306_t disp;     // OLED Display

// Todo - Add Sound

/* GameMode
 * Menu = 0
 * TETRIS = 1
 * PONG = 2
 * SNAKE = 3
 * BREAKOUT = 4
 * PACMAN = 5
 */
int Game = 0;
int gameTempSelect = 1;
int gametempButton = 10;

/*      Menu        */
void MENU() {
    /*  Input   */
    if (gpio_get(BUTTON_UP) == 0) { gameTempSelect--; if (gameTempSelect <= 0) { gameTempSelect = 5; } }
    if (gpio_get(BUTTON_DOWN) == 0) { gameTempSelect++; if (gameTempSelect > 3) { gameTempSelect = 1; } }
    if (gpio_get(BUTTON_START) == 0)  {
        if (gameTempSelect == 1) { Game = 1; }
        if (gameTempSelect == 2) { Game = 2; }
        if (gameTempSelect == 3) { Game = 3; }
        if (gameTempSelect == 4) { Game = 4; }
        if (gameTempSelect == 5) { Game = 5; }
    }
    
    /*  Update  */
    if (gameTempSelect == 1) { gametempButton = 13; }
    if (gameTempSelect == 2) { gametempButton = 28; }
    if (gameTempSelect == 3) { gametempButton = 43; }
    if (gameTempSelect == 4) { gametempButton = 53; }
    if (gameTempSelect == 5) { gametempButton = 63; }

    /*  Render  */
    ssd1306_draw_square(&disp, 35, gametempButton, 10, 10);
    ssd1306_draw_string(&disp, 50, 15, 1, "TETRIS");
    ssd1306_draw_string(&disp, 52, 30, 1, "PONG");
    ssd1306_draw_string(&disp, 50, 45, 1, "SNAKE");
    ssd1306_draw_string(&disp, 50, 55, 1, "BREAKOUT");
    ssd1306_draw_string(&disp, 50, 65, 1, "PACMAN");
    ssd1306_show(&disp);
    sleep_ms(200);
    ssd1306_clear(&disp);
}

////////////////////////////////////////////////////////////////////
/*      Tetris      */
////////////////////////////////////////////////////////////////////

#define HEIGHT 15
#define WIDTH 10

bool tetrisInit = false;


const int FALL_SPEED = 10;
unsigned int seed = 12345;

const int TETROMINOES[7][4][4] = {
    // I
    {{0, 0, 0, 0}, 
     {1, 1, 1, 1}, 
     {0, 0, 0, 0}, 
     {0, 0, 0, 0}},

    // O
    {{0, 0, 0, 0}, 
     {0, 1, 1, 0}, 
     {0, 1, 1, 0}, 
     {0, 0, 0, 0}},

    // T
    {{0, 1, 0, 0}, 
     {1, 1, 1, 0}, 
     {0, 0, 0, 0}, 
     {0, 0, 0, 0}},

    // Z
    {{1, 1, 0, 0}, 
     {0, 1, 1, 0}, 
     {0, 0, 0, 0}, 
     {0, 0, 0, 0}},

    // S
    {{0, 1, 1, 0}, 
     {1, 1, 0, 0}, 
     {0, 0, 0, 0}, 
     {0, 0, 0, 0}},

    // J
    {{1, 0, 0, 0}, 
     {1, 1, 1, 0}, 
     {0, 0, 0, 0}, 
     {0, 0, 0, 0}},

    // L
    {{0, 0, 1, 0}, 
     {1, 1, 1, 0}, 
     {0, 0, 0, 0}, 
     {0, 0, 0, 0}}
};

int board[HEIGHT][WIDTH] = {0};
int currentPiece;
int pieceX, pieceY, rotation, fallCounter;
bool gameOver = false;

int randInt() {
    seed ^= seed << 21;
    seed ^= seed >> 21;
    seed ^= seed << 4; 
    return seed % 7;  // Generate numbers between 0 and 6
}

bool isCollision(int x, int y, int r) {
    int rotatedShape[4][4] = {0}; // Temporary rotated shape

    // Get the rotated shape
    int shapeSize = 4; // Maximum 4x4
    int originalShape[4][4];

    // Copy the original piece
    for (int i = 0; i < shapeSize; i++) {
        for (int j = 0; j < shapeSize; j++) {
            originalShape[i][j] = TETROMINOES[currentPiece][i][j];
        }
    }

    // Apply rotation
    for (int rotationStep = 0; rotationStep < r; rotationStep++) {
        int temp[4][4] = {0};  // Temporary buffer for rotation
        for (int i = 0; i < shapeSize; i++) {
            for (int j = 0; j < shapeSize; j++) {
                temp[j][shapeSize - 1 - i] = originalShape[i][j];
            }
        }
        // Copy rotated shape back
        for (int i = 0; i < shapeSize; i++) {
            for (int j = 0; j < shapeSize; j++) {
                originalShape[i][j] = temp[i][j];
            }
        }
    }

    // Now check collisions using the rotated shape
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (originalShape[i][j]) {  // Check if block exists in shape
                int newX = x + j;
                int newY = y + i;

                // Out of bounds check
                if (newX < 0 || newX >= WIDTH || newY >= HEIGHT) {
                    return true;  // Collision detected
                }

                // Check if it overlaps with an existing piece
                if (newY >= 0 && board[newY][newX]) {
                    return true;
                }
            }
        }
    }
    return false;  // No collision
}

void spawnPiece() {
    currentPiece = randInt() % 7;
    pieceX = WIDTH / 2 - 1;
    pieceY = 0;
    rotation = 0;
    if (isCollision(pieceX, pieceY, rotation)) { gameOver = true; }
}

void rotateTetromino(int tetromino[4][4]) {
    int temp[4][4];

    // Copy rotated values into temp array
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            temp[j][3 - i] = tetromino[i][j];
        }
    }

    // Copy back into the original array
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            tetromino[i][j] = temp[i][j];
        }
    }
}
void lockPiece() {
    int rotatedShape[4][4];
    
    // Copy the original shape
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            rotatedShape[i][j] = TETROMINOES[currentPiece][i][j];
        }
    }

    // Apply rotation
    for (int r = 0; r < rotation; r++) {
        rotateTetromino(rotatedShape);  // Rotate piece based on its current rotation state
    }

    // Lock the rotated shape into the board
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (rotatedShape[i][j]) {
                int x = pieceX + j;
                int y = pieceY + i;
                
                // Ensure it does not write out of bounds
                if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                    board[y][x] = currentPiece + 1;  // Place the block on the board
                }
            }
        }
    }
}

// Function to clear full rows
void clearRows() {
    int newRow = HEIGHT - 1;  // Start from the bottom row

    // Traverse from bottom to top
    for (int y = HEIGHT - 1; y >= 0; y--) {
        int isFull = 1;  // Assume the row is full

        for (int x = 0; x < WIDTH; x++) {
            if (board[y][x] == 0) {
                isFull = 0;  // Found an empty spot, row is not full
                break;
            }
        }

        if (!isFull) {
            // Copy this row to the `newRow` position
            for (int x = 0; x < WIDTH; x++) {
                board[newRow][x] = board[y][x];
            }
            newRow--;  // Move up for the next valid row
        }
    }

    // Fill remaining rows at the top with empty spaces
    for (int y = newRow; y >= 0; y--) {
        for (int x = 0; x < WIDTH; x++) {
            board[y][x] = 0;
        }
    }
}

void initTetris() {

    // Clear board
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            board[y][x] = 0;
        }
    }

    currentPiece = 0;

    pieceX = WIDTH / 2 - 1;
    pieceY = 0;

    rotation = 0;

    fallCounter = 0;

    gameOver = false;

    spawnPiece();

    tetrisInit = true;
}

void Tetris(void) {
    if (!tetrisInit) { initTetris(); }
    
    /*      Input        */
    if (gpio_get(BUTTON_LEFT) == 0) { if (!isCollision(pieceX - 1, pieceY, rotation)) { pieceX--; } }
    if (gpio_get(BUTTON_RIGHT) == 0) { if (!isCollision(pieceX + 1, pieceY, rotation)) { pieceX++; } }

    if (gpio_get(BUTTON_DOWN) == 0) { 
        if (!isCollision(pieceX, pieceY + 1, rotation)) { pieceY++; } 
        else {  lockPiece();  clearRows();  spawnPiece();  }
    }
    
    if (gpio_get(BUTTON_UP) == 0) { 
        int newRotation = (rotation + 1) % 4; 
        if (!isCollision(pieceX, pieceY, newRotation)) {  rotation = newRotation; }
    }

    if (gpio_get(BUTTON_SELECT) == 0) {
        tetrisInit = false;
        Game = 0;
    }

    /*      Update       */
    fallCounter++;
    if (fallCounter >= FALL_SPEED) {
        pieceY++;
        fallCounter = 0;
    }
    if (isCollision(pieceX, pieceY, rotation)) {
        pieceY--;
        lockPiece();
        clearRows();
        spawnPiece();
    }

    if (gameOver) {
        tetrisInit = false;
        Game = 0;
    }


    /*      Render       */
    int tempBoard[HEIGHT][WIDTH];  // Temporary copy of the board to overlay the active piece

    // Copy the current board state into tempBoard
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) { tempBoard[y][x] = board[y][x]; }
    }

    // Get the current piece shape
    int rotatedShape[4][4] = {0}; // Assume max piece size is 4x4
    int originalShape[4][4];
    int shapeSize = 4;

    // Copy original shape into a temporary array
    for (int i = 0; i < shapeSize; i++) {
        for (int j = 0; j < shapeSize; j++) { originalShape[i][j] = TETROMINOES[currentPiece][i][j]; }
    }

    memcpy(rotatedShape, originalShape, sizeof(originalShape));
    for(int r = 0; r < rotation; r++) { rotateTetromino(rotatedShape); }

    // Overlay the rotated piece on the temporary board
    for (int i = 0; i < shapeSize; i++) {
        for (int j = 0; j < shapeSize; j++) {
            if (rotatedShape[i][j]) {
                int x = pieceX + j;
                int y = pieceY + i;
                if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
                    tempBoard[y][x] = currentPiece + 1; // Temporarily add the active piece
                }
            }
        }
    }

    // Draw the temporary board with the active piece to the OLED display
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (tempBoard[y][x] != 0) { ssd1306_draw_square(&disp, x * 4, y * 4, 3, 3); }  // Draw filled block (8x8 pixels per block)
            else { ssd1306_draw_pixel(&disp, x * 4 + 1, y * 4 + 1); } // Draw empty space
        }
    }

    ssd1306_show(&disp);
    sleep_ms(50);
    ssd1306_clear(&disp);
}

////////////////////////////////////////////////////////////////////
/*      PONG        */
////////////////////////////////////////////////////////////////////

#define PADDLE_HEIGHT 8
#define BALL_SPEED 2
#define AI_SPEED 3  // Adjust speed to make AI harder/easier
#define PLAYER_SPEED 2

int paddle1_y = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
int paddle2_y = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
int ball_x = SCREEN_WIDTH / 2, ball_y = SCREEN_HEIGHT / 2;
int ball_dx = 1, ball_dy = 1;
int score1 = 0;
int score2 = 0;

bool pongInit = false;

void initPong() {

    paddle1_y = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
    paddle2_y = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;

    ball_x = SCREEN_WIDTH / 2;
    ball_y = SCREEN_HEIGHT / 2;

    ball_dx = 1;
    ball_dy = 1;

    score1 = 0;
    score2 = 0;

    pongInit = true;
}

void PONG() {
    if (!pongInit) {
        initPong();
    }
    /*      Input    */
    if (gpio_get(BUTTON_UP) == 0 && paddle1_y > 0) paddle1_y -= PLAYER_SPEED;
    if (gpio_get(BUTTON_DOWN) == 0 && paddle1_y < SCREEN_HEIGHT - PADDLE_HEIGHT) paddle1_y += PLAYER_SPEED;
    if (gpio_get(BUTTON_SELECT) == 0) {
        pongInit = false;
        Game = 0;
    }

    

    /*      Update    */
    // AI for Player 2 - Follow the ball
    if (ball_dx > 0) {  // Only move when ball is coming towards AI
        if (ball_y < paddle2_y) paddle2_y -= AI_SPEED;  // Move up
        if (ball_y > paddle2_y + PADDLE_HEIGHT) paddle2_y += AI_SPEED;  // Move down
    }

    // Keep AI paddle within bounds
    if (paddle2_y < 0) paddle2_y = 0;
    if (paddle2_y > SCREEN_HEIGHT - PADDLE_HEIGHT) paddle2_y = SCREEN_HEIGHT - PADDLE_HEIGHT;

    // Move ball
    ball_x += ball_dx * BALL_SPEED;
    ball_y += ball_dy * BALL_SPEED;

    // Ball collision with top and bottom
    if (ball_y <= 0 || ball_y >= SCREEN_HEIGHT - 2) { ball_dy = -ball_dy; }

    // Ball collision with paddles
    if ((ball_x <= 3 && ball_y + 2 > paddle1_y && ball_y < paddle1_y + 6) || (ball_x >= SCREEN_WIDTH - 4 && ball_y + 2 > paddle2_y && ball_y < paddle2_y + 6)) 
    { ball_dx = -ball_dx; }


    // Check if ball goes off screen (score update & reset game)
    if (ball_x < 0) {
        score2++;  // Player 2 (AI) scores
        ball_x = SCREEN_WIDTH / 2;
        ball_y = SCREEN_HEIGHT / 2;
        ball_dx = 1;  // Reset ball direction
    }
    if (ball_x >= SCREEN_WIDTH) {
        score1++;  // Player 1 scores
        ball_x = SCREEN_WIDTH / 2;
        ball_y = SCREEN_HEIGHT / 2;
        ball_dx = -1;  // Reset ball direction
    }

    /*      Render    */
    char score_text1[16] = {0};
    char score_text2[16] = {0};
    sprintf(score_text1, "%d", score1);  // Convert score1 to string
    sprintf(score_text2, "%d", score2);  // Convert score2 to string

    ssd1306_draw_string(&disp, (SCREEN_WIDTH / 2) - 14, 5, 1, score_text1);  // Player Score
    ssd1306_draw_string(&disp, (SCREEN_WIDTH / 2) + 10, 5, 1, score_text2);  // Bot Score

    ssd1306_draw_line(&disp, SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT); // Line

    // Draw paddle 1 (Left Paddle)
    ssd1306_draw_square(&disp, 1, paddle1_y, 2, PADDLE_HEIGHT);

    // Draw paddle 2 (Right Paddle)
    ssd1306_draw_square(&disp, SCREEN_WIDTH - 3, paddle2_y, 2, PADDLE_HEIGHT);

    // Draw Ball (2x2 square)
    ssd1306_draw_square(&disp, ball_x, ball_y, 2, 2);

    ssd1306_show(&disp);
    sleep_ms(50);
    ssd1306_clear(&disp);
}

////////////////////////////////////////////////////////////////////
/*      SNAKE       */
////////////////////////////////////////////////////////////////////

#define SNAKE_MAX_LEN 50

int snakeX[SNAKE_MAX_LEN] = {SCREEN_WIDTH / 2};
int snakeY[SNAKE_MAX_LEN] = {SCREEN_HEIGHT / 2};
int snakeLen = 5;
int foodX = 5, foodY = 5;
int dirX = 1, dirY = 0;  // Start moving right
bool snakeInit = false;


void spawnFood() {
    foodX = randInt() % SCREEN_WIDTH;
    foodY = randInt() % SCREEN_HEIGHT;
}

void initSnake() {

    snakeLen = 5;

    for (int i = 0; i < SNAKE_MAX_LEN; i++) {

        snakeX[i] = SCREEN_WIDTH / 2 - i;
        snakeY[i] = SCREEN_HEIGHT / 2;
    }

    dirX = 1;
    dirY = 0;

    foodX = 5;
    foodY = 5;

    spawnFood();

    gameOver = false;

    snakeInit = true;
}

void SNAKE() {
    if (!snakeInit) {
        initSnake();
    }
    /*      Input    */
    if (gpio_get(BUTTON_UP) == 0 && dirY == 0) { dirX = 0; dirY = -1; }
    if (gpio_get(BUTTON_DOWN) == 0 && dirY == 0) { dirX = 0; dirY = 1; }
    if (gpio_get(BUTTON_LEFT) == 0 && dirX == 0) { dirX = -1; dirY = 0; }
    if (gpio_get(BUTTON_RIGHT) == 0 && dirX == 0) { dirX = 1; dirY = 0; }
    if (gpio_get(BUTTON_SELECT) == 0) {
        snakeInit = false;
        Game = 0;
    }

    /*      Update    */
    // Move snake
    for (int i = snakeLen - 1; i > 0; i--) {
        snakeX[i] = snakeX[i - 1];
        snakeY[i] = snakeY[i - 1];
    }
    snakeX[0] += dirX;
    snakeY[0] += dirY;

    // Check collision with walls
    // Wrap-around screen logic
    if (snakeX[0] < 0) { 
        snakeX[0] = SCREEN_WIDTH;  // Wrap to right side
    } // Wrap to left side
    else if (snakeX[0] >= SCREEN_WIDTH) {  snakeX[0] = 0; }

    if (snakeY[0] < 0) { // Wrap to bottom
        snakeY[0] = SCREEN_HEIGHT - 2; } 
    else if (snakeY[0] >= SCREEN_HEIGHT) { // Wrap to top
        snakeY[0] = 0; }

    // Check if any part of the snake's head touches the food
    if ((snakeX[0] == foodX || snakeX[0] == foodX + 1) && (snakeY[0] == foodY || snakeY[0] == foodY + 1)) {
        snakeLen++;  // Increase length
        spawnFood(); // Generate new food position
    }

    // Check if head collides with any part of the body
    for (int i = 1; i < snakeLen; i++) { if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) { gameOver = true; } }
    if (gameOver) {
        snakeInit = false;
        Game = 0;
    }

    /*      Render    */
    for (int i = 0; i < snakeLen; i++) {
        ssd1306_draw_square(&disp, snakeX[i], snakeY[i], 2, 2);
    }
    ssd1306_draw_square(&disp, foodX, foodY, 2, 2);  // Draw food
    ssd1306_show(&disp);
    sleep_ms(50);
    ssd1306_clear(&disp);
}
////////////////////////////////////////////////////////////////////
/*      BREAKOUT     */
////////////////////////////////////////////////////////////////////

#define BRICK_ROWS 4
#define BRICK_COLS 10
#define BRICK_WIDTH 12
#define BRICK_HEIGHT 4

int breakoutPaddleX = SCREEN_WIDTH / 2 - 10;
int breakoutBallX = SCREEN_WIDTH / 2;
int breakoutBallY = SCREEN_HEIGHT / 2;
int breakoutBallDX = 1;
int breakoutBallDY = -1;

bool bricks[BRICK_ROWS][BRICK_COLS];

bool breakoutInit = false;


void BREAKOUT() {
    if (!breakoutInit) {
        initBreakout();
    }

    /*      Input    */
    if (gpio_get(BUTTON_LEFT) == 0 && breakoutPaddleX > 0) {
        breakoutPaddleX -= 3;
    }

    if (gpio_get(BUTTON_RIGHT) == 0 &&
        breakoutPaddleX < SCREEN_WIDTH - 20) {
        breakoutPaddleX += 3;
    }

    if (gpio_get(BUTTON_SELECT) == 0) {
        breakoutInit = false;
        Game = 0;
    }

    /*      Update    */
    breakoutBallX += breakoutBallDX;
    breakoutBallY += breakoutBallDY;

    // Wall collision
    if (breakoutBallX <= 0 || breakoutBallX >= SCREEN_WIDTH - 2) {
        breakoutBallDX *= -1;
    }

    if (breakoutBallY <= 0) {
        breakoutBallDY *= -1;
    }

    // Paddle collision
    if (breakoutBallY >= SCREEN_HEIGHT - 8 &&
        breakoutBallX >= breakoutPaddleX &&
        breakoutBallX <= breakoutPaddleX + 20) {

        breakoutBallDY = -1;
    }

    // Lose
    if (breakoutBallY > SCREEN_HEIGHT) {
        breakoutInit = false;
        Game = 0;
    }

    // Brick Collision
    for (int y = 0; y < BRICK_ROWS; y++) {
        for (int x = 0; x < BRICK_COLS; x++) {

            if (!bricks[y][x]) continue;

            int brickX = x * BRICK_WIDTH;
            int brickY = y * (BRICK_HEIGHT + 2);

            if (breakoutBallX >= brickX &&
                breakoutBallX <= brickX + BRICK_WIDTH &&
                breakoutBallY >= brickY &&
                breakoutBallY <= brickY + BRICK_HEIGHT) {

                bricks[y][x] = false;
                breakoutBallDY *= -1;
            }
        }
    }


    /*      Render    */
    for (int y = 0; y < BRICK_ROWS; y++) {
        for (int x = 0; x < BRICK_COLS; x++) {

            if (bricks[y][x]) {

                int brickX = x * BRICK_WIDTH;
                int brickY = y * (BRICK_HEIGHT + 2);

                ssd1306_draw_square(
                    &disp,
                    brickX,
                    brickY,
                    BRICK_WIDTH - 1,
                    BRICK_HEIGHT
                );
            }
        }
    }

    // Paddle
    ssd1306_draw_square(
        &disp,
        breakoutPaddleX,
        SCREEN_HEIGHT - 5,
        20,
        3
    );

    // Ball
    ssd1306_draw_square(&disp, breakoutBallX, breakoutBallY, 2, 2);
    
    ssd1306_show(&disp);
    sleep_ms(20);
    ssd1306_clear(&disp);

}

////////////////////////////////////////////////////////////////////
/*      PACMAN       */
////////////////////////////////////////////////////////////////////

#define MAP_W 16
#define MAP_H 8
#define TILE 8

char pacmap[MAP_H][MAP_W + 1] = {
    "################",
    "#..............#",
    "#.####..####...#",
    "#..............#",
    "#..##......##..#",
    "#..............#",
    "#......##......#",
    "################"
};

int pacX = 1;
int pacY = 1;

int ghostX = 10;
int ghostY = 5;

int pacDirX = 0;
int pacDirY = 0;

bool pacmanInit = false;

void initPacman() {
    pacX = 1;
    pacY = 1;

    ghostX = 10;
    ghostY = 5;

    pacDirX = 0;
    pacDirY = 0;

    pacmanInit = true;
}

void PACMAN() {
    if (!pacmanInit) { initPacman(); }
    
    /*      Input    */
    if (gpio_get(BUTTON_UP) == 0) {
        pacDirX = 0;
        pacDirY = -1;
    }

    if (gpio_get(BUTTON_DOWN) == 0) {
        pacDirX = 0;
        pacDirY = 1;
    }

    if (gpio_get(BUTTON_LEFT) == 0) {
        pacDirX = -1;
        pacDirY = 0;
    }

    if (gpio_get(BUTTON_RIGHT) == 0) {
        pacDirX = 1;
        pacDirY = 0;
    }

    if (gpio_get(BUTTON_SELECT) == 0) {
        pacmanInit = false;
        Game = 0;
    }
    
    /*      Update    */
    int nextX = pacX + pacDirX;
    int nextY = pacY + pacDirY;

    if (pacmap[nextY][nextX] != '#') {
        pacX = nextX;
        pacY = nextY;
    }

    // Eat pellets
    if (pacmap[pacY][pacX] == '.') { pacmap[pacY][pacX] = ' '; }

    // SIMPLE GHOST AI
    if (ghostX < pacX && pacmap[ghostY][ghostX + 1] != '#') { ghostX++; }
    else if (ghostX > pacX && pacmap[ghostY][ghostX - 1] != '#') { ghostX--; }

    if (ghostY < pacY && pacmap[ghostY + 1][ghostX] != '#') { ghostY++; }
    else if (ghostY > pacY && pacmap[ghostY - 1][ghostX] != '#') { ghostY--; }

    // COLLISION
    if (pacX == ghostX && pacY == ghostY) {
        pacmanInit = false;
        Game = 0;
    }

    /*      Render    */
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {

            int px = x * TILE;
            int py = y * TILE;

            if (pacmap[y][x] == '#') {

                ssd1306_draw_square(
                    &disp,
                    px,
                    py,
                    TILE,
                    TILE
                );
            }

            if (pacmap[y][x] == '.') {

                ssd1306_draw_pixel(
                    &disp,
                    px + 4,
                    py + 4
                );
            }
        }
    }

    // Pacman
    ssd1306_draw_square(&disp, pacX * TILE + 2, pacY * TILE + 2, 4, 4);

    // Ghost
    ssd1306_draw_square(&disp, ghostX * TILE + 2, ghostY * TILE + 2, 4, 4);

    ssd1306_show(&disp);
    sleep_ms(120);
    ssd1306_clear(&disp);
}


int main() {
    const uint led_pin = 25;

    stdio_init_all();
    
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    // Blink LED to show Pico is running
    gpio_put(led_pin, 1);
    sleep_ms(300);
    gpio_put(led_pin, 0);
    sleep_ms(300);

    // Initialize Display I2C on GPIO 2 (SDA) and GPIO 3 (SCL)
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    disp.external_vcc=false;        // Set false for output display
    bool res = ssd1306_init(&disp, 128, 64, 0x3C, I2C_PORT);
    if (!res) { 
        gpio_put(led_pin, 1);
        sleep_ms(5000);
        gpio_put(led_pin, 0);
        return 0; } // Exit if OLED initialization fails

    // Initalize Buttons

    // UP BUTTON
    gpio_init(BUTTON_UP);
    gpio_set_dir(BUTTON_UP, GPIO_IN);
    gpio_pull_up(BUTTON_UP);

    // DOWN BUTTON
    gpio_init(BUTTON_DOWN);
    gpio_set_dir(BUTTON_DOWN, GPIO_IN);
    gpio_pull_up(BUTTON_DOWN);

    // LEFT BUTTON
    gpio_init(BUTTON_LEFT);
    gpio_set_dir(BUTTON_LEFT, GPIO_IN);
    gpio_pull_up(BUTTON_LEFT);
    
    // RIGHT BUTTON
    gpio_init(BUTTON_RIGHT);
    gpio_set_dir(BUTTON_RIGHT, GPIO_IN);
    gpio_pull_up(BUTTON_RIGHT);

    // SELECT BUTTON
    gpio_init(BUTTON_SELECT);
    gpio_set_dir(BUTTON_SELECT, GPIO_IN);
    gpio_pull_up(BUTTON_SELECT);

    // START BUTTON
    gpio_init(BUTTON_START);
    gpio_set_dir(BUTTON_START, GPIO_IN);
    gpio_pull_up(BUTTON_START);

    while (true) {

        switch(Game)
        {
            case 0: // MENU
                MENU();
            break;
            case 1: // TETRIS
                Tetris();
            break;
            case 2: // PONG
                PONG();
            break;
            case 3: // SNAKE
                SNAKE();
            break;
            case 4: // BREAKOUT
                BREAKOUT();
            break;
            case 5: // PACMAN
                PACMAN();
            break;
        }
    }

    return 0;
}


