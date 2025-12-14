#ifndef GOMOKU_H
# define GOMOKU_H

// --- STANDARD LIBS ---
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// --- PROJECT LIBS ---
#include "../MLX42/include/MLX42/MLX42.h"

// --- CONSTANTES DE JEU & PERFORMANCE ---
#define WIDTH 528
#define HEIGHT 528

// Marges Graphiques (Réintégrées)
#define BOARD_MARGIN_LEFT 30
#define BOARD_MARGIN_TOP 30
#define BOARD_MARGIN_RIGHT 30
#define BOARD_MARGIN_BOTTOM 30

// Paramètres du plateau
#define BOARD_SIZE 19
#define MAX_BOARD (BOARD_SIZE * BOARD_SIZE) // 361 cases
#define WIN_LENGTH 5
#define MAX_CAPTURES 10 // 5 paires = Victoire

// Valeurs des cases (Optimisé pour lecture rapide)
#define EMPTY 0
#define P1 1     // Joueur 1 (Noir)
#define P2 2     // Joueur 2 (Blanc/IA)
#define PREVIS 3 // Prévisualisation

// --- MACROS (CRITIQUE POUR LA VITESSE) ---
#define GET_INDEX(x, y) ((y) * BOARD_SIZE + (x))
#define GET_X(index) ((index) % BOARD_SIZE)
#define GET_Y(index) ((index) / BOARD_SIZE)
#define IS_VALID(x, y) ((x) >= 0 && (x) < BOARD_SIZE && (y) >= 0 && (y) < BOARD_SIZE)

// --- STRUCTURES ---

typedef struct screen
{
    mlx_t       *mlx;
    mlx_image_t *img;
    mlx_image_t *text_img;
    uint32_t    width;
    uint32_t    height;
    double      x;          // mouse x
    double      y;          // mouse y
    bool        moved;
    bool        resized;
    bool        isClicked;
    bool        changed;
    int         board_size;
} screen;

typedef struct timer
{
    bool running;
    struct timespec start_ts;
    double elapsed;
} timer;

// Structure du Jeu (Optimisée IA)
typedef struct game
{
    int     board[MAX_BOARD]; // TABLEAU 1D
    int     captures[3];      // captures[P1] et captures[P2]
    int     score[3];         
    
    int     board_size;
    int     turn;             // 1 ou 2
    int     iaTurn;           // 1 ou 2, ou 0 si pas d'IA
    bool    game_over;
    
    timer   ia_timer;
} game;

typedef struct both
{
    screen  *windows;
    game    *gameData;
} both;

// --- PROTOTYPES ---

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
void    mousehook(mouse_key_t button, action_t action, modifier_key_t mods, void *param);

// Timer
void    stopTimer(timer *t);
void    launchTimer(timer *t);
void    resetTimer(timer *t);

// Utils
bool    isIaTurn(int iaTurn, int turn);

// Information & Logique
bool checkFiveInARow(int *board, int idx, int player);
int  checkCaptures(game *gameData, int idx, int player);
void playMoveAndCheck(game *gameData, int idx, screen *windows);
void checkVictoryCondition(game *gameData, screen *windows);

// IA
void    makeIaMove(game *gameData, screen *windows);

#endif