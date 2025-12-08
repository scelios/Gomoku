#include "../include/gomoku.h"

bool in_bounds(int x, int y, int n){
    return x >= 0 && y >= 0 && x < n && y < n;
}

/* helper: validate parameters for capture checking */
bool captureParamsValid(game *gameData, screen *windows, int lx, int ly)
{
    if (!gameData || !windows)
        return false;
    int n = gameData->board_size;
    if (n <= 0 || n > 50)
        return false;
    if (!in_bounds(lx, ly, n))
        return false;
    int owner = gameData->board[ly][lx];
    if (owner != 1 && owner != 2)
        return false;
    return true;
}

/* helper: mark opponent stones that should be captured by the move at (lx,ly)
   Fills `marked` and increments capturesByPlayer[owner-1] per pair captured.
   Returns number of stones marked (not pairs). */
int markCapturesFromMove(game *gameData, int lx, int ly,
                                   bool marked[50][50], int capturesByPlayer[2])
{
    int n = gameData->board_size;
    int owner = gameData->board[ly][lx];
    int opponent = (owner == 1) ? 2 : 1;

    const int dirs[8][2] = {
        { 1, 0}, { 0, 1}, {-1, 0}, { 0,-1},
        { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1}
    };      // all 8 directions

    int total_marked = 0;

    for (int d = 0; d < 8; d++)
    {
        int dx = dirs[d][0];
        int dy = dirs[d][1];

        // positions of the three stones in the direction
        int x1 = lx + dx, y1 = ly + dy;
        int x2 = lx + 2*dx, y2 = ly + 2*dy;
        int x3 = lx + 3*dx, y3 = ly + 3*dy;

        if (!in_bounds(x1, y1, n) || !in_bounds(x2, y2, n) || !in_bounds(x3, y3, n))
            continue;

        /* pattern: owner (lx,ly) - opponent - opponent - owner (x3,y3) */
        if (gameData->board[y1][x1] == opponent &&
            gameData->board[y2][x2] == opponent &&
            gameData->board[y3][x3] == owner)
        {
            /* mark the two opponent stones for removal (avoid double counting) */
            if (!marked[y1][x1] && !marked[y2][x2])
            {
                marked[y1][x1] = true;
                marked[y2][x2] = true;
                capturesByPlayer[owner - 1] += 1; /* one pair captured */
                total_marked += 2;
            }
        }
    }

    return total_marked;
}

/* helper: remove marked stones from the board and redraw their cells.
   Returns number of stones removed. */
int removeMarkedPieces(game *gameData, screen *windows, bool marked[50][50])
{
    int n = gameData->board_size;
    int removed = 0;

    for (int y = 0; y < n; y++)
    {
        for (int x = 0; x < n; x++)
        {
            if (marked[y][x])
            {
                gameData->board[y][x] = 0;
                removed++;
                drawSquare(windows, x, y, 0); /* redraw empty square */
            }
        }
    }
    return removed;
}

/* main function kept simple: validate -> mark -> remove -> update scores/flags */
void checkPieceCapture(game *gameData, screen *windows, int lx, int ly)
{
    if (!captureParamsValid(gameData, windows, lx, ly))
        return;

    bool marked[50][50] = { false };
    int capturesByPlayer[2] = {0, 0};

    int marked_count = markCapturesFromMove(gameData, lx, ly, marked, capturesByPlayer);
    if (marked_count == 0)
        return;

    int removed = removeMarkedPieces(gameData, windows, marked);
    if (removed > 0)
    {
        /* update scores: capturesByPlayer counts pairs */
        gameData->score[0] += capturesByPlayer[0];
        gameData->score[1] += capturesByPlayer[1];

        windows->changed = true;
    }
}