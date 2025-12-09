#include "../include/gomoku.h"
#include <string.h>



// returns list of coordinates of the 5 in a row found on the board for the given player
nRowList getCoordinatesOfNRow(int board[19][19], int player)
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
    for (int y = 0; y < 19; y++)
    {
        for (int x = 0; x < 19; x++)
        {
            if (board[y][x] != player)
                continue;
            for (int d = 0; d < 4; d++)
            {
                int dx = dirs[d][0];
                int dy = dirs[d][1];

                // if (dx == 0 && dy == 0) // defensive : jamais accepter (0,0)
                //     continue;
                printf("Checking from (%d, %d) direction (%d, %d)\n", x, y, dx, dy);
                int length = 1;
                rowBuf.coords[0].x = x;
                rowBuf.coords[0].y = y;
                rowBuf.length = 1;
                int nx = x + dx;
                int ny = y + dy;
                while (nx >= 0 && nx < 19 && ny >= 0 && ny < 19 && board[ny][nx] == player && length < 19)
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

bool canBeCutAt(int board[19][19], vector2 coord, int player) // returns true if the 5 in a row at (x,y) can be cut
{
    // check all 8 directions for opponent stones adjacent to coord and own stones on the opposite side
    const int dirs[8][2] = {
        { 1, 0}, { 0, 1}, {-1, 0}, { 0,-1},
        { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1}
    };      // all 8 directions
    int opponent = (player == 1) ? 2 : 1;

    for (int d = 0; d < 8; d++)
    {
        int dx = dirs[d][0];
        int dy = dirs[d][1];

        int x1 = coord.x + dx;
        int y1 = coord.y + dy;
        int x2 = coord.x - dx;
        int y2 = coord.y - dy;


        if (x1 < 0 || x1 >= 19 || y1 < 0 || y1 >= 19)
            continue;
        if (x2 < 0 || x2 >= 19 || y2 < 0 || y2 >= 19)
            continue;

        if (board[y1][x1] == opponent && board[y2][x2] == player)
        {
            // printf("Can be cut at (%d, %d)\n", coord.x, coord.y);
            return true;
        }
    }
    return false;
}

bool RowIsLessThan5(int board[19][19], vector2 coord, int player) // returns true if the row at (x,y) is less than 5 in a row
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
                nCanBeCut++;
                break;
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