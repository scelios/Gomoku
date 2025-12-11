#include "../include/gomoku.h"
#include <limits.h>

// =========================================================
// SÉCURITÉ & HELPERS
// =========================================================

static inline bool is_valid_pos(int x, int y, int n) {
    return (x >= 0 && x < n && y >= 0 && y < n);
}

typedef struct {
    int x, y;
    int score;
} FastMove;

static int compareFastMoves(const void *a, const void *b) {
    return ((FastMove*)b)->score - ((FastMove*)a)->score;
}

// =========================================================
// OPTIMISATION : HEURISTIQUE DE TRI (MOVE ORDERING)
// =========================================================

// Cette fonction doit être TRÈS RAPIDE. Elle sert juste à dire :
// "Regarde ce coup là en premier, il a l'air dangereux".
static int rateMoveLight(const game *g, int x, int y, int player) {
    int score = 0;
    int n = g->board_size;
    int opponent = (player == 1) ? 2 : 1;

    // Directions
    int dirs[4][2] = {{1,0}, {0,1}, {1,1}, {1,-1}};
    
    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0], dy = dirs[d][1];
        
        // Scan Voisins immédiats (1 case)
        int c1 = -1, c2 = -1;
        if (is_valid_pos(x+dx, y+dy, n)) c1 = g->board[y+dy][x+dx];
        if (is_valid_pos(x-dx, y-dy, n)) c2 = g->board[y-dy][x-dx];

        // Scan Voisins distants (2 cases) - Pour voir les trous .X.X.
        int f1 = -1, f2 = -1;
        if (is_valid_pos(x+2*dx, y+2*dy, n)) f1 = g->board[y+2*dy][x+2*dx];
        if (is_valid_pos(x-2*dx, y-2*dy, n)) f2 = g->board[y-2*dy][x-2*dx];

        // --- ATTAQUE (Nos pierres) ---
        if (c1 == player && c2 == player) score += 1000; // Créer un 3 ou boucher un 4
        else if (c1 == player || c2 == player) score += 200;

        // --- DÉFENSE (PRIORITÉ ABSOLUE) ---
        
        // 1. Bloquer un 4 potentiel (XXX_)
        // Motif : [Opp][Opp] ou [Opp]...[Opp]
        if (c1 == opponent && c2 == opponent) score += 1000000; // BLOQUER IMMÉDIATEMENT (XXX)
        
        // 2. Bloquer un 3 potentiel (XX_)
        // Motif : [Opp] et [Opp] un peu plus loin (X.X)
        if ((c1 == opponent && f1 == opponent) || (c2 == opponent && f2 == opponent)) score += 500000;
        
        // 3. Bloquer un 3 potentiel collé (XX)
        if (c1 == opponent || c2 == opponent) score += 10000;

        // 4. Défense Bordure
        if ((c1 == -1 && c2 == opponent) || (c2 == -1 && c1 == opponent)) score += 50000;
    }

    // 3. CAPTURES (inchangé, c'est vital)
    int cap_dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,-1},{1,-1},{-1,1}};
    for (int d = 0; d < 8; ++d) {
        int dx = cap_dirs[d][0], dy = cap_dirs[d][1];
        int x1 = x + dx, y1 = y + dy;
        int x2 = x + 2*dx, y2 = y + 2*dy;
        int x3 = x + 3*dx, y3 = y + 3*dy;

        if (is_valid_pos(x3, y3, n)) {
            if (g->board[y1][x1] == opponent && 
                g->board[y2][x2] == opponent && 
                g->board[y3][x3] == player) {
                score += 10000; // Capture offensive
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

    int opponent = (player == 1) ? 2 : 1;
    int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,-1},{1,-1},{-1,1}};

    for (int d = 0; d < 8; ++d) {
        int dx = dirs[d][0], dy = dirs[d][1];
        int x1 = x + dx, y1 = y + dy;
        int x2 = x + 2*dx, y2 = y + 2*dy;
        int x3 = x + 3*dx, y3 = y + 3*dy;

        if (is_valid_pos(x3, y3, g->board_size)) {
            if (g->board[y1][x1] == opponent && 
                g->board[y2][x2] == opponent && 
                g->board[y3][x3] == player) {
                
                if (rec->captured_count + 2 <= MAX_CAPTURED_STONES) {
                    rec->cap_x[rec->captured_count] = x1; rec->cap_y[rec->captured_count] = y1; rec->cap_value[rec->captured_count] = opponent; rec->captured_count++;
                    rec->cap_x[rec->captured_count] = x2; rec->cap_y[rec->captured_count] = y2; rec->cap_value[rec->captured_count] = opponent; rec->captured_count++;
                    
                    g->board[y1][x1] = 0; g->board[y2][x2] = 0;
                    g->score[player-1] += 2;
                    
                    if (key_ptr) { updateZobrist(key_ptr, x1, y1, opponent); updateZobrist(key_ptr, x2, y2, opponent); }
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
        int cx = rec->cap_x[i], cy = rec->cap_y[i], val = rec->cap_value[i];
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
    
    // Check Time limit
    if ((++node_check & 2047) == 0) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - g->ia_timer.start_ts.tv_sec) + 
                         (now.tv_nsec - g->ia_timer.start_ts.tv_nsec) / 1e9;
        if (elapsed > time_limit_sec) stop_search = true;
    }
    if (stop_search) return 0;

    // Terminal conditions
    if (g->score[0] >= 10 || g->score[1] >= 10) return evaluateBoard(g);
    if (depth <= 0) return evaluateBoard(g);

    // TT Lookup
    uint64_t key = *key_ptr;
    TTEntry *entry = &transpositionTable[key % TRANSPOSITION_TABLE_SIZE];
    
    if (entry->key == key && entry->depth >= depth) {
        if (entry->flag == EXACT) return entry->value;
        if (entry->flag == LOWERBOUND && entry->value > alpha) alpha = entry->value;
        else if (entry->flag == UPPERBOUND && entry->value < beta) beta = entry->value;
        if (alpha >= beta) return entry->value;
    }

    // Move Generation
    FastMove moveList[361]; 
    int moveCount = 0;
    int n = g->board_size;
    bool visited[19][19] = {false};

    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            if (g->board[y][x] != 0) {
                // Rayon réduit (1) dans l'arbre pour speeder
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int nx = x + dx, ny = y + dy;
                        if (is_valid_pos(nx, ny, n) && g->board[ny][nx] == 0 && !visited[ny][nx]) {
                            visited[ny][nx] = true;
                            if (moveCount < 361) {
                                moveList[moveCount].x = nx;
                                moveList[moveCount].y = ny;
                                moveList[moveCount].score = rateMoveLight(g, nx, ny, g->turn);
                                moveCount++;
                            }
                        }
                    }
                }
            }
        }
    }

    if (moveCount == 0) return evaluateBoard(g);

    qsort(moveList, moveCount, sizeof(FastMove), compareFastMoves);

    // =============================================================
    // 1. DYNAMIC BRANCHING (PRUNING)
    // =============================================================
    int max_branches;
    if (depth >= 8) max_branches = 25; // Un peu plus strict pour la vitesse
    else if (depth >= 5) max_branches = 10;
    else if (depth >= 3) max_branches = 6;
    else max_branches = 4;

    // SÉCURITÉ : Ne jamais ignorer un coup qui semble vital (score > 1000)
    // On parcourt la liste pour voir combien de coups sont "dangereux"
    int urgent_moves = 0;
    for (int k = 0; k < moveCount; k++) {
        if (moveList[k].score > 1000) urgent_moves++;
        else break; // Comme c'est trié, on peut arrêter
    }
    
    // Si on a beaucoup de coups urgents, on force l'IA à les regarder tous
    if (urgent_moves > max_branches) max_branches = urgent_moves;

    // Plafond absolu pour ne pas exploser (par exemple 40)
    if (max_branches > 40) max_branches = 40;

    int branches = (moveCount > max_branches) ? max_branches : moveCount;
    int best_val = maximizingPlayer ? MIN_SCORE : WINNING_SCORE;
    int alphaOrig = alpha;
    int player = g->turn;

    for (int i = 0; i < branches; i++) {
        MoveRecord rec;
        if (!make_move_inplace(g, moveList[i].x, moveList[i].y, player, &rec, &key)) continue;

        int val;
        bool needs_full_search = true;

        // =============================================================
        // 2. LMR (LATE MOVE REDUCTION)
        // =============================================================
        // Si on est profond, pas sur les 4 meilleurs coups, et coup pas tactique
        if (depth >= 3 && i >= 4 && moveList[i].score < 4000) {
            int reduction = 1;
            // Recherche avec fenêtre nulle (Null Window Search)
            // On vérifie juste si val > alpha sans chercher la valeur exacte
            val = minimax(g, depth - 1 - reduction, alpha, alpha + 1, !maximizingPlayer, &key);

            if (stop_search) return 0;

            // Si le résultat ne bat pas alpha, la réduction était bonne, on garde ce score.
            // Sinon (val > alpha), le coup est meilleur que prévu, il faut re-chercher à fond.
            if (maximizingPlayer && val <= alpha) needs_full_search = false;
            else if (!maximizingPlayer && val >= beta) needs_full_search = false;
        }

        if (needs_full_search) {
            val = minimax(g, depth - 1, alpha, beta, !maximizingPlayer, &key);
        }

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
// FIND BEST MOVE (AVEC DEBUG PRINT)
// =========================================================
move findBestMove(game *g, int max_depth_limit) {
    move bestMove = {{-1,-1}, MIN_SCORE};
    
    // =========================================================
    // 1. CRÉATION DE LA SANDBOX (COPIE DE SÉCURITÉ)
    // =========================================================
    game sandbox_game = *g; 
    
    // Initialisation Zobrist sur la copie
    uint64_t key = computeZobrist(&sandbox_game);
    
    // Initialisation Timer SUR LA COPIE (car minimax lira sandbox_game)
    stop_search = false;
    clock_gettime(CLOCK_MONOTONIC, &sandbox_game.ia_timer.start_ts);

    // =========================================================
    // 2. GÉNÉRATION DES COUPS (RACINE) SUR SANDBOX
    // =========================================================
    FastMove rootMoves[361];
    int moveCount = 0;
    int n = sandbox_game.board_size;
    bool visited[19][19] = {false};

    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            if (sandbox_game.board[y][x] != 0) {
                for (int dy=-2; dy<=2; dy++) {
                    for (int dx=-2; dx<=2; dx++) {
                        int nx=x+dx, ny=y+dy;
                        if (is_valid_pos(nx, ny, n) && sandbox_game.board[ny][nx] == 0 && !visited[ny][nx]) {
                            visited[ny][nx] = true;
                            rootMoves[moveCount].x = nx;
                            rootMoves[moveCount].y = ny;
                            rootMoves[moveCount].score = rateMoveLight(&sandbox_game, nx, ny, sandbox_game.turn);
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

    int bestMoveIndex = 0;
    int previous_score = 0;

    // =========================================================
    // 3. BOUCLE ITERATIVE DEEPENING (ID)
    // =========================================================
    for (int current_depth = 2; current_depth <= max_depth_limit; current_depth += 2) {
        
        // Root Pruning: On ne regarde que les 40 meilleurs coups à haute profondeur
        int moves_to_scan = moveCount;
        if (current_depth >= 6 && moves_to_scan > 40) moves_to_scan = 40;

        int alpha = MIN_SCORE;
        int beta = WINNING_SCORE;
        
        // Aspiration Windows logic
        bool use_aspiration = (current_depth >= 6);
        int window = 400; 
        if (use_aspiration) {
            alpha = previous_score - window;
            beta  = previous_score + window;
        }

        bool research_needed = true;
        
        // Boucle de "Re-search" en cas d'échec de l'Aspiration Window
        while (research_needed) {
            research_needed = false;
            
            int best_val_iter = MIN_SCORE;
            int best_idx_iter = -1;

            // Move Ordering: Mettre le meilleur coup précédent en premier
            if (current_depth > 2 && bestMoveIndex > 0 && bestMoveIndex < moves_to_scan) {
                FastMove temp = rootMoves[0];
                rootMoves[0] = rootMoves[bestMoveIndex];
                rootMoves[bestMoveIndex] = temp;
            }

            // --- SCAN DES COUPS ---
            for (int i = 0; i < moves_to_scan; i++) {
                MoveRecord rec;
                
                // ACTION SUR LA SANDBOX
                if (!make_move_inplace(&sandbox_game, rootMoves[i].x, rootMoves[i].y, sandbox_game.turn, &rec, &key)) continue;

                // APPEL MINIMAX SUR LA SANDBOX
                int val = minimax(&sandbox_game, current_depth - 1, alpha, beta, false, &key);
                
                // UNDO SUR LA SANDBOX
                undo_move_inplace(&sandbox_game, &rec, &key);

                if (stop_search) break; // Sortie immédiate de la boucle for

                if (val > best_val_iter) {
                    best_val_iter = val;
                    best_idx_iter = i;
                }
                
                if (best_val_iter > alpha) alpha = best_val_iter;
                
                // FAIL HIGH (Beta Cutoff dans Aspiration)
                if (use_aspiration && best_val_iter >= beta) {
                    use_aspiration = false;
                    alpha = MIN_SCORE;
                    beta = WINNING_SCORE;
                    research_needed = true; 
                    break; // On casse la boucle for pour relancer le while
                }
            }
            
            // Si timeout pendant la boucle for
            if (stop_search) break; 

            // FAIL LOW (Alpha Cutoff dans Aspiration)
            if (!research_needed && use_aspiration && best_val_iter <= (previous_score - window)) {
                use_aspiration = false;
                alpha = MIN_SCORE;
                beta = WINNING_SCORE;
                research_needed = true;
                continue; // On relance le while
            }

            // Si recherche réussie (pas de timeout, résultats valides)
            if (best_idx_iter != -1) {
                printf(">> Depth %d completed. Move: (%d, %d) Score: %d\n", 
                       current_depth, rootMoves[best_idx_iter].x, rootMoves[best_idx_iter].y, best_val_iter);
                
                previous_score = best_val_iter;
                bestMoveIndex = best_idx_iter;
                bestMove.position.x = rootMoves[best_idx_iter].x;
                bestMove.position.y = rootMoves[best_idx_iter].y;
                bestMove.score = best_val_iter;
            }
        } // Fin While (Research)

        if (stop_search) {
            printf(">> TIMEOUT reached while searching Depth %d\n", current_depth);
            break; // On sort de la boucle Iterative Deepening
        }

        if (previous_score >= WINNING_SCORE - 5000) {
            printf(">> Winning move found at Depth %d!\n", current_depth);
            break;
        }
    } // Fin For (Iterative Deepening)

    return bestMove;
}