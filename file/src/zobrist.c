#include "../include/gomoku.h"

uint64_t zobristTable[19][19][2];
TTEntry transpositionTable[TRANSPOSITION_TABLE_SIZE];

void initZobrist() {
    srand(time(NULL));
    for (int x = 0; x < 19; x++) {
        for (int y = 0; y < 19; y++) {
            for (int p = 0; p < 2; p++) {
                uint64_t high = ((uint64_t)rand() << 32);
                uint64_t low  = (uint64_t)rand();
                zobristTable[x][y][p] = high | low;
            }
        }
    }

    // Initialiser la transposition table
    for (int i = 0; i < TRANSPOSITION_TABLE_SIZE; i++)
        transpositionTable[i].key = 0;
}

uint64_t computeZobrist(const game *g) {
    uint64_t key = 0;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            int player = g->board[y][x];
            if (player > 0)
                key ^= zobristTable[x][y][player - 1];
        }
    }
    return key;
}

void updateZobrist(uint64_t *key, int x, int y, int player) {
    *key ^= zobristTable[x][y][player - 1];
}
