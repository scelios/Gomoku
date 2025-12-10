// evaluation.c
// Implementation of countFormations and evaluateBoard for Gomoku
// Uses the project types and constants declared in gomoku.h

#include "../include/gomoku.h"
#include <string.h>

// Helper: check cell value with bounds
static inline int cell_at(const game *g, int x, int y)
{
    if (x < 0 || x >= g->board_size || y < 0 || y >= g->board_size) return -1; // outside
    return g->board[y][x];
}

// Helper: detect "gapped" patterns (one gap inside window length 5)
// returns true if there is a pattern of 4 stones of 'player' within any length-5 window centered or starting
static bool has_gapped_four(const game *g, int player, int cx, int cy, int dx, int dy)
{
    // We'll scan windows of length 5 that include (cx,cy) in a stretch along direction (dx,dy)
    for (int offset = -4; offset <= 0; ++offset) {
        int player_count = 0;
        int opp_found = 0;
        for (int k = 0; k < 5; ++k) {
            int x = cx + (offset + k) * dx;
            int y = cy + (offset + k) * dy;
            int v = cell_at(g, x, y);
            if (v == player) player_count++;
            else if (v != 0) opp_found = 1;
        }
        if (!opp_found && player_count == 4) return true;
    }
    return false;
}

// countFormations: scans the board for sequences for 'player'
// Writes a formation score (sum of pattern weights) into *score_out and number of open threes into *open_threes
void countFormations(const game *gameData, int player, int *score_out, int *open_threes, int min_x, int max_x, int min_y, int max_y)
{
    if (score_out) *score_out = 0;
    if (open_threes) *open_threes = 0;
    int n = gameData->board_size;
    int opponent = (player == 1) ? 2 : 1;

    // Directions: right, down, down-right, up-right
    const int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};

    // To avoid double counting contiguous sequences multiple times, we will only start a scan from
    // cells that are the "beginning" of a consecutive run in that direction (previous cell != player)

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (gameData->board[y][x] != player) continue;

            for (int d = 0; d < 4; ++d) {
                int dx = dirs[d][0], dy = dirs[d][1];
                int px = x - dx, py = y - dy;
                if (px >= 0 && px < n && py >= 0 && py < n && gameData->board[py][px] == player) {
                    // Not the start of the sequence
                    continue;
                }

                // count consecutive length starting from (x,y)
                int len = 0;
                int cx = x, cy = y;
                while (cx >= 0 && cx < n && cy >= 0 && cy < n && gameData->board[cy][cx] == player) {
                    len++;
                    cx += dx; cy += dy;
                }

                // positions just before and after the run
                int ax = x - dx, ay = y - dy; // before
                int bx = x + len * dx, by = y + len * dy; // after

                int blocked_ends = 0; // 0 = open both ends; 1 = one blocked; 2 = both blocked
                int v;
                v = cell_at(gameData, ax, ay);
                if (v == opponent || v == -1) blocked_ends++;
                v = cell_at(gameData, bx, by);
                if (v == opponent || v == -1) blocked_ends++;

                // If both ends blocked, this sequence is dead (no extension)
                if (blocked_ends == 2) continue;

                // scoring
                if (len >= 5) {
                    if (score_out) *score_out = F_FIVE;
                    if (open_threes) *open_threes = *open_threes; // unchanged
                    return; // immediate winning formation found
                }

                if (len == 4) {
                    if (blocked_ends == 0) {
                        // open four: very strong
                        if (score_out) *score_out += F_FOUR_OPEN;
                    } else if (blocked_ends == 1) {
                        if (score_out) *score_out += F_FOUR_HALF_OPEN;
                    }
                    // additionally check for gapped four patterns that are not consecutive
                }
                else if (len == 3) {
                    if (blocked_ends == 0) {
                        if (score_out) *score_out += F_THREE_OPEN;
                        if (open_threes) *open_threes += 1;
                    } else if (blocked_ends == 1) {
                        if (score_out) *score_out += F_THREE_HALF_OPEN;
                    }
                }
                else if (len == 2) {
                    // minor scoring for pairs (optionnal), skip for now
                }

                // detect gapped patterns like X_XXX or XX_XX (single gap within window 5)
                // We'll check windows of length 5 around the run's first cell
                if (has_gapped_four(gameData, player, x, y, dx, dy)) {
                    if (score_out) *score_out += F_FOUR_OPEN; // promote as open four
                }

                // move on to next
            }
        }
    }
}

// evaluateBoard: returns score relative to current player (gameData->turn)
// Positive values are good for the player to move. High magnitude near WINNING_SCORE indicates wins.
int evaluateBoard(game *gameData)
{
    int current_player = gameData->turn;
    int opponent = (current_player == 1) ? 2 : 1;

    // Check immediate capture win
    if (gameData->score[current_player - 1] >= 10) return WINNING_SCORE;
    if (gameData->score[opponent - 1] >= 10) return MIN_SCORE;

    int min_x = 19, max_x = 0, min_y = 19, max_y = 0;
    bool empty = true;
    
    // On scanne pour réduire la zone de recherche des patterns
    for (int y=0; y<19; y++) {
        for (int x=0; x<19; x++) {
            if (gameData->board[y][x] != 0) {
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
                empty = false;
            }
        }
    }
    
    if (empty) return 0;

    // Ajouter une marge de sécurité pour les alignements potentiels
    min_x = (min_x - 4 < 0) ? 0 : min_x - 4;
    max_x = (max_x + 4 > 18) ? 18 : max_x + 4;
    min_y = (min_y - 4 < 0) ? 0 : min_y - 4;
    max_y = (max_y + 4 > 18) ? 18 : max_y + 4;

        // Formation scores
    int score_p = 0, threes_p = 0;
    int score_o = 0, threes_o = 0;

    countFormations(gameData, current_player, &score_p, &threes_p, min_x, max_x, min_y, max_y);
    countFormations(gameData, opponent, &score_o, &threes_o, min_x, max_x, min_y, max_y);

    // Immediate five-in-row terminal
    if (score_p >= F_FIVE) return WINNING_SCORE;
    if (score_o >= F_FIVE) return MIN_SCORE;

    // If opponent has open four, immediate danger
    if (score_o >= F_FOUR_OPEN) return MIN_SCORE + 10000; // large negative

    // Basic weighting scheme (tweakable)
    long formation_score = 0;
    formation_score += (long)score_p * 1000L; // own formations weighted
    formation_score -= (long)score_o * 1200L; // opponent formations slightly more urgent

    // double-three threats
    if (threes_p >= 2) formation_score += 50000L;
    if (threes_o >= 2) formation_score -= 70000L;

    // capture differential
    int capture_diff = gameData->score[current_player - 1] - gameData->score[opponent - 1];
    formation_score += (long)capture_diff * 30000L;

    // center control bonus: prefer moves that are nearer to center of board
    // compute a small penalty based on sum of distances of stones from center for current player minus opponent
    int center = gameData->board_size / 2;
    long center_score = 0;
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            int v = gameData->board[y][x];
            if (v == 0) continue; // Skip vide rapidement
            
            if (v == current_player) {
                int dx = x - center; int dy = y - center;
                center_score += ( (gameData->board_size - (abs(dx) + abs(dy))) );
            } else if (v == opponent) {
                int dx = x - center; int dy = y - center;
                center_score -= ( (gameData->board_size - (abs(dx) + abs(dy))) );
            }
        }
    }
    formation_score += center_score * 10L;

    // clamp to int range and return
    if (formation_score > WINNING_SCORE) return WINNING_SCORE - 1;
    if (formation_score < MIN_SCORE) return MIN_SCORE + 1;
    return (int)formation_score;
}

