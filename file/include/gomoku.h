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

// Project Libraries
#include "../MLX42/include/MLX42/MLX42.h"

// Structures
typedef struct screen
{
    mlx_t           *mlx;           // MLX instance
    mlx_image_t     *img;           // MLX image
    uint32_t        width;          // window width
    uint32_t        height;         // window height
    double          x;              // mouse x position 
    double          y;              // mouse y position
    bool            moved;          // mouse moved
    bool            resized;        // window resized
    bool            isClicked;      // mouse button clicked
    bool            changed;        // screen needs to be redrawn
}    screen;

typedef struct game
{
    int         board[50][50];        // game board, 1 for player 1, 2 for player 2, 0 for empty, 3 for previsualization
    int         board_size;     // size of the board
    int         turn;           // current turn
    bool        game_over;      // is the game over
}   game;


// Graphics utils
int     get_rgba(int r, int g, int b, int a);
void    printBlack(screen *windows);
void    keyhook(mlx_key_data_t keydata, void *param);
void    cursor(double xpos, double ypos, void *param);
void    resize(int32_t width, int32_t height, void *param);

void ft_hook(void* param);
void ft_randomize(void* param);
uint32_t main2();


#endif // GOMOKU_H