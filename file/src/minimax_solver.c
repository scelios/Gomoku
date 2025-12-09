#include "../include/gomoku.h"
#include <limits.h>

// =========================================================
// SÉCURITÉ & HELPERS
// =========================================================

// Définition locale et inline pour être sûr qu'elle existe et est rapide
static inline bool is_valid_pos(int x, int y, int n) {
    return (x >= 0 && x < n && y >= 0 && y < n);
}

typedef struct {
    int x, y;
    int score;
} FastMove;

// Comparaison pour qsort (tri décroissant)
static int compareFastMoves(const void *a, const void *b) {
    return ((FastMove*)b)->score - ((FastMove*)a)->score;
}

// Heuristique légère pour le tri (Move Ordering)
static int rateMoveLight(const game *g, int x, int y) {
    int score = 0;
    int n = g->board_size;
    int center = n / 2;

    // Distance au centre
    score += (n - (abs(x - center) + abs(y - center)));

    // Voisinnage immédiat (Rayon 1 et 2)
    // On booste si on joue à côté d'une pierre
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (is_valid_pos(nx, ny, n) && g->board[ny][nx] != 0) {
                score += 20; // Pierre adjacente
            }
        }
    }
    return score;
}

// =========================================================
// MAKE / UNDO (IN-PLACE)
// =========================================================

bool make_move_inplace(game *g, int x, int y, int player, MoveRecord *rec, uint64_t *key_ptr) {
    if (!is_valid_pos(x, y, g->board_size) || g->board[y][x] != 0) return false;

    rec->x = x; rec->y = y;
    rec->prev_turn = g->turn;
    rec->prev_score[0] = g->score[0];
    rec->prev_score[1] = g->score[1];
    rec->captured_count = 0;

    g->board[y][x] = player;
    g->turn = (player == 1) ? 2 : 1;
    if (key_ptr) updateZobrist(key_ptr, x, y, player);

    // Gestion Captures
    int opponent = (player == 1) ? 2 : 1;
    int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,-1},{1,-1},{-1,1}};

    for (int d = 0; d < 8; ++d) {
        int dx = dirs[d][0], dy = dirs[d][1];
        int x1 = x + dx, y1 = y + dy;
        int x2 = x + 2*dx, y2 = y + 2*dy;
        int x3 = x + 3*dx, y3 = y + 3*dy;

        // Vérification stricte des bornes avant lecture
        if (is_valid_pos(x3, y3, g->board_size)) {
            if (g->board[y1][x1] == opponent && 
                g->board[y2][x2] == opponent && 
                g->board[y3][x3] == player) {
                
                if (rec->captured_count + 2 <= MAX_CAPTURED_STONES) {
                    rec->cap_x[rec->captured_count] = x1; 
                    rec->cap_y[rec->captured_count] = y1; 
                    rec->cap_value[rec->captured_count] = opponent; 
                    rec->captured_count++;

                    rec->cap_x[rec->captured_count] = x2; 
                    rec->cap_y[rec->captured_count] = y2; 
                    rec->cap_value[rec->captured_count] = opponent; 
                    rec->captured_count++;
                    
                    g->board[y1][x1] = 0;
                    g->board[y2][x2] = 0;
                    g->score[player-1] += 2;
                    
                    if (key_ptr) { 
                        updateZobrist(key_ptr, x1, y1, opponent); 
                        updateZobrist(key_ptr, x2, y2, opponent); 
                    }
                }
            }
        }
    }
    return true;
}

void undo_move_inplace(game *g, MoveRecord *rec, uint64_t *key_ptr) {
    int current_owner = g->board[rec->y][rec->x];
    g->board[rec->y][rec->x] = 0;
    if (key_ptr) updateZobrist(key_ptr, rec->x, rec->y, current_owner);

    for (int i = 0; i < rec->captured_count; i++) {
        int cx = rec->cap_x[i];
        int cy = rec->cap_y[i];
        int val = rec->cap_value[i];
        g->board[cy][cx] = val;
        if (key_ptr) updateZobrist(key_ptr, cx, cy, val);
    }

    g->score[0] = rec->prev_score[0];
    g->score[1] = rec->prev_score[1];
    g->turn = rec->prev_turn;
}

// =========================================================
// MINIMAX
// =========================================================

static bool stop_search = false;
static double time_limit_sec = 0.45;

int minimax(game *g, int depth, int alpha, int beta, bool maximizingPlayer, uint64_t *key_ptr) {
    static int node_check = 0;
    // Check timeout tous les 2048 noeuds pour moins d'appel système
    if ((++node_check & 2047) == 0) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - g->ia_timer.start_ts.tv_sec) + 
                         (now.tv_nsec - g->ia_timer.start_ts.tv_nsec) / 1e9;
        if (elapsed > time_limit_sec) stop_search = true;
    }
    if (stop_search) return 0;

    if (g->score[0] >= 10 || g->score[1] >= 10) return evaluateBoard(g);
    if (depth <= 0) return evaluateBoard(g);

    uint64_t key = *key_ptr;
    TTEntry *entry = &transpositionTable[key % TRANSPOSITION_TABLE_SIZE];
    
    if (entry->key == key && entry->depth >= depth) {
        if (entry->flag == EXACT) return entry->value;
        if (entry->flag == LOWERBOUND && entry->value > alpha) alpha = entry->value;
        else if (entry->flag == UPPERBOUND && entry->value < beta) beta = entry->value;
        if (alpha >= beta) return entry->value;
    }

    // Génération de coups restreinte aux voisins
    // 361 max, mais on vérifie l'index pour éviter le segfault
    FastMove moveList[361]; 
    int moveCount = 0;
    int n = g->board_size;
    
    // Tableau visited statique pour éviter allocation stack trop grosse ?
    // Non, il faut qu'il soit propre à la récursion. 361 bools = 361 octets. C'est safe.
    bool visited[19][19] = {false};

    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            if (g->board[y][x] != 0) {
                // Rayon court (1) pour réduire le nombre de coups explorés en profondeur
                int rad = 1; 
                for (int dy = -rad; dy <= rad; dy++) {
                    for (int dx = -rad; dx <= rad; dx++) {
                        int nx = x + dx, ny = y + dy;
                        if (is_valid_pos(nx, ny, n) && g->board[ny][nx] == 0 && !visited[ny][nx]) {
                            visited[ny][nx] = true;
                            if (moveCount < 361) { // SECURITE CRITIQUE
                                moveList[moveCount].x = nx;
                                moveList[moveCount].y = ny;
                                moveList[moveCount].score = rateMoveLight(g, nx, ny);
                                moveCount++;
                            }
                        }
                    }
                }
            }
        }
    }

    if (moveCount == 0) {
        if (g->board[n/2][n/2] == 0) return evaluateBoard(g); // Plateau vide traité au root
        return evaluateBoard(g); // Plus de coups ?
    }

    qsort(moveList, moveCount, sizeof(FastMove), compareFastMoves);

    int best_val = maximizingPlayer ? MIN_SCORE : WINNING_SCORE;
    int alphaOrig = alpha;
    int player = g->turn;

    // Pruning agressif sur le nombre de coups à explorer par noeud (Beam Search partiel)
    // On ne regarde que les 20 meilleurs coups si la profondeur est grande, pour éviter l'explosion
    int moves_to_scan = moveCount;
    // if (depth > 4 && moves_to_scan > 15) moves_to_scan = 15; // Optionnel si encore trop lent/crash

    for (int i = 0; i < moves_to_scan; i++) {
        MoveRecord rec;
        if (!make_move_inplace(g, moveList[i].x, moveList[i].y, player, &rec, &key)) continue;

        int val = minimax(g, depth - 1, alpha, beta, !maximizingPlayer, &key);
        undo_move_inplace(g, &rec, &key);

        if (stop_search) return 0;

        if (maximizingPlayer) {
            if (val > best_val) best_val = val;
            if (best_val > alpha) alpha = best_val;
        } else {
            if (val < best_val) best_val = val;
            if (best_val < beta) beta = best_val;
        }
        if (beta <= alpha) break;
    }

    if (!stop_search) {
        entry->key = key;
        entry->depth = depth;
        entry->value = best_val;
        if (best_val <= alphaOrig) entry->flag = UPPERBOUND;
        else if (best_val >= beta) entry->flag = LOWERBOUND;
        else entry->flag = EXACT;
    }

    return best_val;
}

// =========================================================
// RACINE
// =========================================================

move findBestMove(game *g, int max_depth_limit) {
    move bestMove = {{-1,-1}, MIN_SCORE};
    uint64_t key = computeZobrist(g);
    stop_search = false;
    clock_gettime(CLOCK_MONOTONIC, &g->ia_timer.start_ts);

    FastMove rootMoves[361];
    int moveCount = 0;
    int n = g->board_size;
    bool visited[19][19] = {false};

    // Génération root
    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            if (g->board[y][x] != 0) {
                for (int dy=-2; dy<=2; dy++) { // Rayon plus large (2) à la racine
                    for (int dx=-2; dx<=2; dx++) {
                        int nx=x+dx, ny=y+dy;
                        if (is_valid_pos(nx, ny, n) && g->board[ny][nx] == 0 && !visited[ny][nx]) {
                            visited[ny][nx] = true;
                            rootMoves[moveCount].x = nx;
                            rootMoves[moveCount].y = ny;
                            rootMoves[moveCount].score = rateMoveLight(g, nx, ny);
                            moveCount++;
                        }
                    }
                }
            }
        }
    }

    if (moveCount == 0) {
        bestMove.position.x = n/2; bestMove.position.y = n/2; bestMove.score = 0;
        return bestMove;
    }

    qsort(rootMoves, moveCount, sizeof(FastMove), compareFastMoves);

    // Iterative Deepening
    int bestMoveIndex = 0;
    for (int current_depth = 2; current_depth <= max_depth_limit; current_depth += 2) {
        
        int alpha = MIN_SCORE;
        int beta = WINNING_SCORE;
        int best_val_iter = MIN_SCORE;
        int best_idx_iter = -1;

        // Move Ordering: Essayer le meilleur coup précédent en premier
        if (current_depth > 2 && bestMoveIndex > 0) {
            FastMove temp = rootMoves[0];
            rootMoves[0] = rootMoves[bestMoveIndex];
            rootMoves[bestMoveIndex] = temp;
        }

        for (int i = 0; i < moveCount; i++) {
            MoveRecord rec;
            if (!make_move_inplace(g, rootMoves[i].x, rootMoves[i].y, g->turn, &rec, &key)) continue;

            int val = minimax(g, current_depth - 1, alpha, beta, false, &key);
            undo_move_inplace(g, &rec, &key);

            if (stop_search) break;

            if (val > best_val_iter) {
                best_val_iter = val;
                best_idx_iter = i;
            }
            if (best_val_iter > alpha) alpha = best_val_iter;
        }

        if (stop_search) break;

        if (best_idx_iter != -1) {
            bestMoveIndex = best_idx_iter;
            bestMove.position.x = rootMoves[best_idx_iter].x;
            bestMove.position.y = rootMoves[best_idx_iter].y;
            bestMove.score = best_val_iter;
        }
        
        // Si victoire trouvée, on arrête
        if (best_val_iter >= WINNING_SCORE - 5000) break;
    }

    return bestMove;
}