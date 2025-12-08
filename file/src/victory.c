#include "../include/gomoku.h"
#include <string.h>



// returns list of coordinates of the 5 in a row found on the board for the given player
nRowList getCoordinatesOfNRow(int board[50][50], int player)
{
    nRowList list;
    memset(&list, 0, sizeof(list)); // set all to zero
    int nRowIndex = 0;
    const int dirs[4][2] = {
        {1, 0}, // horizontal -
        {0, 1}, // vertical |
        {1, 1}, // diagonal 
        {1, -1} // diagonal /
    };
    rowCoordinates rowBuf;
    for (int y = 0; y < 50; y++)
    {
        for (int x = 0; x < 50; x++)
        {
            if (board[y][x] != player)
                continue;
            for (int d = 0; d < 4; d++)
            {
                int dx = dirs[d][0];
                int dy = dirs[d][1];

                // if (dx == 0 && dy == 0) // defensive : jamais accepter (0,0)
                //     continue;
                // printf("Checking from (%d, %d) direction (%d, %d)\n", x, y, dx, dy);
                int length = 1;
                rowBuf.coords[0].x = x;
                rowBuf.coords[0].y = y;
                rowBuf.length = 1;
                int nx = x + dx;
                int ny = y + dy;
                while (nx >= 0 && nx < 50 && ny >= 0 && ny < 50 && board[ny][nx] == player && length < 50)
                {
                    rowBuf.coords[length].x = nx;
                    rowBuf.coords[length].y = ny;
                    length++;
                    nx += dx;
                    ny += dy;
                }
                if (length >= 5)
                {
                    rowBuf.length = length;
                    list.rows[nRowIndex] = rowBuf;
                    nRowIndex++;
                    if (nRowIndex >= MAX_NROWS) // max rows
                        break;
                }
            }
        }
        if (nRowIndex >= MAX_NROWS) // max rows
            break;
    }
    if (nRowIndex < MAX_NROWS) {
        list.rows[nRowIndex].coords[0].x = -1; // end of list
        list.rows[nRowIndex].coords[0].y = -1;
    } else {
        list.rows[MAX_NROWS-1].coords[0].x = -1;
        list.rows[MAX_NROWS-1].coords[0].y = -1;
    }
    return list;
}

bool canBeCutAt(int board[50][50], vector2 coord, int player)
{
    const int dirs[8][2] = {
        { 1, 0}, { 0, 1}, {-1, 0}, { 0,-1},
        { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1}
    };
    int opponent = (player == 1) ? 2 : 1;

    for (int d = 0; d < 8; d++)
    {
        int dx = dirs[d][0];
        int dy = dirs[d][1];

        int ax = coord.x + dx;      // adjacent in dir: must be ally
        int ay = coord.y + dy;
        int bx = coord.x - dx;      // one step behind coord
        int by = coord.y - dy;
        int cx = coord.x + 2*dx;    // two steps forward
        int cy = coord.y + 2*dy;

        if (ax < 0 || ax >= 50 || ay < 0 || ay >= 50) continue;
        if (bx < 0 || bx >= 50 || by < 0 || by >= 50) continue;
        if (cx < 0 || cx >= 50 || cy < 0 || cy >= 50) continue;

        if (board[ay][ax] != player) continue; // need the second allied stone right next to coord

        // Check that this is EXACTLY a pair (not 3+ in a row)
        // Look beyond the pair to ensure no third ally
        int dx3 = coord.x + 3*dx;
        int dy3 = coord.y + 3*dy;
        int dxm = coord.x - 2*dx;
        int dym = coord.y - 2*dy;

        // If within bounds and there's an ally, this is 3+ stones, so can't be cut
        if ((dx3 >= 0 && dx3 < 50 && dy3 >= 0 && dy3 < 50 && board[dy3][dx3] == player))
            continue;
        if ((dxm >= 0 && dxm < 50 && dym >= 0 && dym < 50 && board[dym][dxm] == player))
            continue;

        /* patterns to allow a cut (only for exactly 2 in a row):
           enemy | coord | ally | empty   (1220)
           empty | coord | ally | enemy   (0221) */
        bool forwardCut  = (board[by][bx] == opponent) && (board[cy][cx] == 0);
        bool reverseCut  = (board[by][bx] == 0)        && (board[cy][cx] == opponent);

        if (forwardCut || reverseCut)
        {
            printf("Can be cut at (%d, %d)\n", coord.x, coord.y);
            
            return true;
        }
    }
    return false;
}

bool RowIsLessThan5(int board[50][50], vector2 coord, int player) // returns true if the row at (x,y) is less than 5 in a row
{
    // TODO
    return false;
}

void checkVictoryCondition(game *gameData, screen *windows)
{
    // return;
    // if score >= 5, game over
    if (gameData->score[0] >= 5 || gameData->score[1] >= 5)
    {
        gameData->game_over = true;
    }
    
    // if 5 or more in a row and it can't be cut, game over
    nRowList nRowList = getCoordinatesOfNRow(gameData->board, gameData->turn); // get list of the coordinates of 5 or more in a row
    int nCanBeCut = 0;
    int i = 0;
    int tab[] = {0,1,2,3,4}; // offsets to check based on length of the row
    // done this way to avoid checking the ends of the row which would be useless to cut
    while ( nRowList.rows[i].coords[0].x != -1 && nRowList.rows[i].coords[0].y != -1 && nRowList.rows[i].length >=5)
    {
        
        int length = nRowList.rows[i].length;
        for (int offset = tab[length - 5]; offset < length - tab[length - 5]; offset++)
        {
            vector2 coord = nRowList.rows[i].coords[offset];
            bool canBeCut = canBeCutAt(gameData->board, coord, gameData->turn);
            if (canBeCut)
            {
                printf("Row at index %d can be cut at offset %d (coord %d,%d)\n", i, offset, coord.x, coord.y);
                drawSquare(windows, coord.x, coord.y, 3); // debug: draw nothing at this coord
                nCanBeCut++;
                // break;
            }
        }
        i++;
    }
    if (nCanBeCut < i)
    {
        printf("Player %d wins! (%d %d in a row that cannot be cut)\n", gameData->turn, i - nCanBeCut, i);
        gameData->game_over = true;
    }
}