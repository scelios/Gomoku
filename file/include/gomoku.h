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
#include <limits.h>

// --- PROJECT LIBS ---
#include "../MLX42/include/MLX42/MLX42.h"

// --- CONSTANTES DE JEU & PERFORMANCE ---
#define WIDTH 818
#define HEIGHT 818

// Bouton Reset
#define BTN_X 430
#define BTN_Y 10
#define BTN_W 80
#define BTN_H 30

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

// --- POIDS DES SCORES ---
// Ces valeurs déterminent la personnalité de l'IA.
// Elles sont très espacées (puissances de 10) pour que Minimax priorise nettement.

#define WIN_SCORE       100000000 // Victoire immédiate (5 alignés ou 10 captures)
#define OPEN_FOUR       10000000  // 4 alignés ouverts des deux côtés (Victoire imparable au prochain tour sauf capture)
#define CLOSED_FOUR     100000    // 4 alignés bloqués d'un côté (Force l'adversaire à bloquer)
#define OPEN_THREE      100000    // 3 alignés ouverts (Crée une menace de OPEN_FOUR)
#define CLOSED_THREE    1000      // 3 alignés bloqués
#define OPEN_TWO        100       // 2 alignés ouverts
#define CAPTURE_SCORE   200000    // Valeur d'une paire capturée (Très haute !)

// Vérifie si un pixel (x, y) est dans les limites de la fenêtre 'win'
// On cast en (int) pour éviter les warnings de comparaison signé/non-signé
#define IS_VALID_PIXEL(x, y, win) \
    ((x) >= 0 && (y) >= 0 && \
     (x) < (int)(win)->width && (y) < (int)(win)->height)

// --- STRUCTURES ---

typedef struct screen
{
    mlx_t       *mlx;
    mlx_image_t *img;
    mlx_image_t *text_img;
    mlx_image_t *restart_text;
    uint32_t    width;
    uint32_t    height;
    double      x;
    double      y;
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

typedef struct game
{
    int     board[MAX_BOARD];
    int     captures[3];
    int     score[3];
    int     board_size;
    int     turn;
    int     iaTurn;
    bool    game_over;
    timer   ia_timer;
} game;

typedef struct both
{
    screen  *windows;
    game    *gameData;
} both;

// --- PROTOTYPES ---

// graphicsUtils.c
int     get_rgba(int r, int g, int b, int a);
void    printBlack(screen *windows);
void    putCadrillage(screen *windows);
int     teamColor(unsigned short int team);
void    drawSquare(screen *windows, int x0, int y0, unsigned short int team);
void    initGUI(screen *windows);

// hook.c
void    keyhook(mlx_key_data_t keydata, void *param);
void    cursor(double xpos, double ypos, void *param);
void    resize(int32_t width, int32_t height, void *param);
void    mousehook(mouse_key_t button, action_t action, modifier_key_t mods, void *param);

// timer.c
void    stopTimer(timer *t);
void    launchTimer(timer *t);
void    resetTimer(timer *t);

// utils.c
bool    isIaTurn(int iaTurn, int turn);
void    resetGame(game *gameData, screen *windows);

// information.c (C'était manquant !)
void    printInformation(screen *windows, game *gameData);

// captures.c (C'était manquant !)
void    checkPieceCapture(game *gameData, screen *windows, int lx, int ly);
bool    in_bounds(int x, int y);

// victory.c
void    checkVictoryCondition(game *gameData, screen *windows);

// heuristics.c 
int     evaluate_board(game *g, int player);

// ai.c
void    makeIaMove(game *gameData, screen *windows);

#endif