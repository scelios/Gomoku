#ifndef GOMOKU_H
# define GOMOKU_H

# define WIDTH 254
# define HEIGHT 254
# define true 1
# define false 0
# define TRUE 1
# define FALSE 0

# define BOARD_SIZE 19

// Standard Libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// Project Libraries
#include "../MLX42/include/MLX42/MLX42.h"

// Structures
typedef struct screen
{
    mlx_t           *mlx;           // MLX instance
    mlx_image_t     *img;           // MLX image
    mlx_image_t    *text_img;       // image used for text (last mlx_put_string)
    uint32_t        width;          // window width
    uint32_t        height;         // window height
    double          x;              // mouse x position 
    double          y;              // mouse y position
    bool            moved;          // mouse moved
    bool            resized;        // window resized
    bool            isClicked;      // mouse button clicked
    bool            changed;        // screen needs to be redrawn
    int             board_size;     // size of the board

}    screen;

typedef struct timer
{
    clock_t start_time;
    clock_t elapsed_time;
    bool    running;
}   timer;

typedef struct game
{
    int         board[50][50];        // game board, 1 for player 1, 2 for player 2, 0 for empty, 3 for previsualization
    int         board_size;     // size of the board
    int         turn;           // player turn (1 or 2)
    int         iaTurn;         // 1 or 2 will be the ia turn
    bool        game_over;      // is the game over
    timer       ia_timer;       // ia time for making a move
    int         score[2];       // scores for player 1 and player 2
}   game;

typedef struct both
{
    screen  *windows;
    game    *gameData;
}   both;


// Graphics utils
int     get_rgba(int r, int g, int b, int a);
void    printBlack(screen *windows);
void    putCadrillage(screen *windows);
int     teamColor(unsigned short int team);
void    drawSquare(screen *windows, int x0, int y0, unsigned short int team);


// Hooks
void    keyhook(mlx_key_data_t keydata, void *param);
void    cursor(double xpos, double ypos, void *param);
void    resize(int32_t width, int32_t height, void *param);
void	mousehook(mouse_key_t button, action_t action, modifier_key_t mods, void *param);

// timer
void stopTimer(timer *t);
void launchTimer(timer *t);
void resetTimer(timer *t);

// Utils
bool isIaTurn(int iaTurn, int turn);

// Information
void printInformation(screen *windows, game *gameData);



#endif // GOMOKU_H