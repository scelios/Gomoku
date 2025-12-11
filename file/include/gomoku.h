#ifndef GOMOKU_H
# define GOMOKU_H

// ============================
// Includes Standard
// ============================
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// ============================
// Project Includes
// ============================
#include "../MLX42/include/MLX42/MLX42.h"

// ============================
// Constants
// ============================
#define WIDTH 528
#define HEIGHT 528

#define BOARD_SIZE 19
// Marges graphiques...
#define BOARD_MARGIN_LEFT 30
#define BOARD_MARGIN_TOP 30
#define BOARD_MARGIN_RIGHT 30
#define BOARD_MARGIN_BOTTOM 30

#define MAX_ROW_LENGTH 10
#define MAX_NROWS 200

#define BUTTON_X 10
#define BUTTON_Y 5
#define BUTTON_WIDTH 60
#define BUTTON_HEIGHT 20

// ============================
// AI Constants
// ============================
#define SEARCH_RADIUS 3          // Rayon de recherche autour des pierres
#define MAX_CAPTURED_STONES 16   // Sécurité

#define MAX_SCORE (WINNING_SCORE - 1)
#define MIN_SCORE (-WINNING_SCORE + 1)


// ============================
// Formation heuristics (RÉÉQUILIBRAGE AGRESSIF)
// ============================
// Une menace de victoire doit toujours être supérieure à une capture (150 000)

#define WINNING_SCORE 1000000000

#define F_FIVE           100000000 
#define F_FOUR_OPEN       50000000 
#define F_FOUR_HALF_OPEN  10000000 

// Augmentez encore celui-ci. Un 3 ouvert, c'est la mort assurée contre un bon joueur.
#define F_THREE_OPEN      20000000 

#define F_THREE_HALF_OPEN   100000 
#define F_TWO_OPEN            5000 
#define F_TWO_HALF_OPEN        100

// Transposition Table
#define ZOBRIST_TABLE_SIZE 19*19*2
#define TRANSPOSITION_TABLE_SIZE 1048576 // 2^20
typedef enum { EXACT, LOWERBOUND, UPPERBOUND } TTFlag;

// ============================
// Structures
// ============================

typedef struct {
    uint64_t key;
    int depth;
    int value;
    int flag; // TTFlag
} TTEntry;

typedef struct screen {
    mlx_t        *mlx;
    mlx_image_t  *img;
    mlx_image_t  *text_img;
    uint32_t      width;
    uint32_t      height;
    double        x;
    double        y;
    bool          moved;
    bool          resized;
    bool          isClicked;
    bool          changed;
    int           board_size;
} screen;

typedef struct timer {
    bool running;
    struct timespec start_ts;
    double elapsed;
} timer;

typedef struct game {
    int   board[19][19];
    int   board_size;
    int   turn;
    int   iaTurn;
    bool  game_over;
    timer ia_timer;
    int   score[2];
    int min_x, max_x, min_y, max_y;
} game;

// Structure utilisée par l'IA pour annuler les coups
typedef struct MoveRecord {
    int x, y;
    int prev_turn;
    bool prev_game_over;
    int prev_score[2];
    int captured_count;
    int cap_x[MAX_CAPTURED_STONES];
    int cap_y[MAX_CAPTURED_STONES];
    int cap_value[MAX_CAPTURED_STONES];
} MoveRecord;

typedef struct both {
    screen *windows;
    game   *gameData;
} both;

typedef struct vector2 {
    int x;
    int y;
} vector2;

typedef struct rowCoordinates
{
    vector2 coords[10]; // max 10 coordinates per row of 5 or more
    int length;       // length of the row
} rowCoordinates;

// Structure de retour simple pour l'IA
typedef struct move {
    vector2 position;
    int     score;
} move;

// ============================
// Graphics & Logic Utils
// ============================
int     get_rgba(int r, int g, int b, int a);
void    printBlack(screen *windows);
void    putCadrillage(screen *windows);
int     teamColor(unsigned short int team);
void    drawSquare(screen *windows, int x0, int y0, unsigned short int team);

void    keyhook(mlx_key_data_t keydata, void *param);
void    cursor(double xpos, double ypos, void *param);
void    resize(int32_t width, int32_t height, void *param);
void    mousehook(mouse_key_t button, action_t action, modifier_key_t mods, void *param);

void    stopTimer(timer *t);
void    launchTimer(timer *t);
void    resetTimer(timer *t);

bool    isIaTurn(int iaTurn, int turn);
game    copyGameData(const game *source);

void    printInformation(screen *windows, game *gameData);
// bool    in_bounds(int x, int y, int n);
void    checkPieceCapture(game *gameData, screen *windows, int lx, int ly);
void    checkVictoryCondition(game *gameData, screen *windows);
void    resetGame(game *gameData, screen *windows);
void    drawReplayButton(screen *windows);

// ============================
// AI Interfaces (Cleaned)
// ============================

// Le point d'entrée principal de l'IA
move    findBestMove(game *gameData, int depth);

// Fonctions d'évaluation (utilisées par le solveur)
int     evaluateBoard(game *gameData);
void    countFormations(const game *gameData, int player, int *score_out, int *open_threes, int min_x, int max_x, int min_y, int max_y);

// Zobrist & TT
extern uint64_t zobristTable[19][19][2];
extern TTEntry transpositionTable[TRANSPOSITION_TABLE_SIZE];

void    initZobrist();
uint64_t computeZobrist(const game *g);
void    updateZobrist(uint64_t *key, int x, int y, int player);

#endif // GOMOKU_H