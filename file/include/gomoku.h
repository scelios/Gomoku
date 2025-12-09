#ifndef GOMOKU_H
# define GOMOKU_H

# define WIDTH 528
# define HEIGHT 528
# define true 1
# define false 0
# define TRUE 1
# define FALSE 0

# define BOARD_SIZE 19
# define BOARD_MARGIN_LEFT 30
# define BOARD_MARGIN_TOP 30
# define BOARD_MARGIN_RIGHT 30
# define BOARD_MARGIN_BOTTOM 30
# define MAX_ROW_LENGTH 10
# define MAX_NROWS 200

// Définir les valeurs maximales/minimales pour Minimax
#define WINNING_SCORE 10000000 
#define MAX_SCORE (WINNING_SCORE - 1)
#define MIN_SCORE (-WINNING_SCORE + 1)
#define SEARCH_RADIUS 2 // Regarder dans un carré 5x5 autour de chaque pièce
#define F_THREE_OPEN 10 // Trois ouverts (sans blocage aux extrémités)
#define F_THREE_HALF_OPEN 5 // Trois à moitié ouverts (bloqués d'un côté)
#define F_FOUR_OPEN 100 // Quatre ouverts
#define F_FOUR_HALF_OPEN 50 // Quatre à moitié ouverts
#define F_FIVE 10000000 // Victoire (5)
#define MAX_CANDIDATES 12   // nombre max de coups à explorer (tuneable)
#define MAX_CAPTURED_STONES 16 // max de pierres qui peuvent être capturées en un coup (sécurisé)

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
    bool running;
    struct timespec start_ts;
    double elapsed; /* elapsed seconds accumulated */
}   timer;

typedef struct game
{
    int         board[19][19];  // game board, 1 for player 1, 2 for player 2, 0 for empty, 3 for previsualization
    int         board_size;     // size of the board
    int         turn;           // player turn (1 or 2)
    int         iaTurn;         // 1 or 2 will be the ia turn
    bool        game_over;      // is the game over
    timer       ia_timer;       // ia time for making a move
    int         score[2];       // scores for player 1 and player 2f
}       game;

// -----------------------------
// Structure d'historique d'un coup pour undo
// -----------------------------
typedef struct {
    int x, y;
    int prev_turn;
    bool prev_game_over;
    int prev_score[2];
    int captured_count;
    int cap_x[MAX_CAPTURED_STONES];
    int cap_y[MAX_CAPTURED_STONES];
    int cap_value[MAX_CAPTURED_STONES]; // valeur des pierres enlevées (1 ou 2 selon convention)
} MoveRecord;

typedef struct both
{
    screen  *windows;
    game    *gameData;
}   both;

typedef struct vector2
{
    int x;
    int y;
} vector2;

typedef struct rowCoordinates
{
    vector2 coords[10]; // max 10 coordinates per row of 5 or more
    int length;       // length of the row
} rowCoordinates;

typedef struct nRowList
{
    rowCoordinates rows[MAX_NROWS]; // max rows of 5 or more
} nRowList;

typedef struct move
{
    vector2 position; // Coordonnées (x, y) du coup
    int     score;    // Score Minimax associé à ce coup
} move;

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
game copyGameData(const game *source);

// Information
void printInformation(screen *windows, game *gameData);
void    checkPieceCapture(game *gameData, screen *windows, int lx, int ly);

// Captures
bool    captureParamsValid(game *gameData, screen *windows, int lx, int ly);
int     markCapturesFromMove(game *gameData, int lx, int ly, bool marked[19][19], int capturesByPlayer[2]);
int     removeMarkedPieces(game *gameData, screen *windows, bool marked[19][19]);
void    checkPieceCapture(game *gameData, screen *windows, int lx, int ly);
bool    in_bounds(int x, int y, int n);

// Algo
bool is_empty_and_in_bounds(const game *gameData, int x, int y);
move findBestMove(game *gameData, int depth);
int evaluateBoard(game *gameData);
vector2 *getValidMoves(const game *gameData, int *num_moves);
void countFormations(const game *gameData, int player, int *score_out, int *open_threes);
int compareMovesPriority(const void *a, const void *b);
move *getScoredMoves(const game *gameData, int *num_moves);
int minimax(game *gameData, int depth, int alpha, int beta, bool maximizingPlayer);

// Victory
void    checkVictoryCondition(game *gameData, screen *windows);
nRowList getCoordinatesOfNRow(int board[19][19], int player);

#endif // GOMOKU_H