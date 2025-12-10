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
    int center = n / 2;
    int opponent = (player == 1) ? 2 : 1;

    // 1. Centralité (poids faible)
    score += (n - (abs(x - center) + abs(y - center)));

    // 2. Proximité immédiate (Attaque/Défense basique)
    bool has_neighbor = false;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (is_valid_pos(nx, ny, n) && g->board[ny][nx] != 0) {
                has_neighbor = true;
                score += 50; // On joue collé aux pierres
            }
        }
    }
    if (!has_neighbor) return 0; // Coup isolé = inutile (sauf premier coup)

    // 3. KILLER HEURISTIC : DÉTECTION DE CAPTURE IMMÉDIATE
    // Si ce coup capture, on le booste énormément pour le vérifier en premier.
    // Pattern: X O O [NOUS]
    int dirs[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,-1},{1,-1},{-1,1}};
    
    for (int d = 0; d < 8; ++d) {
        int dx = dirs[d][0], dy = dirs[d][1];
        // On regarde dans la direction inverse du vecteur pour voir si on "ferme" une capture
        // Exemple : NOUS (x,y) <- Opp <- Opp <- NOUS
        int x1 = x + dx, y1 = y + dy;
        int x2 = x + 2*dx, y2 = y + 2*dy;
        int x3 = x + 3*dx, y3 = y + 3*dy;

        if (is_valid_pos(x3, y3, n)) {
            // Est-ce qu'on capture l'adversaire ?
            if (g->board[y1][x1] == opponent && 
                g->board[y2][x2] == opponent && 
                g->board[y3][x3] == player) {
                score += 5000; // PRIORITÉ ABSOLUE
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

    FastMove moveList[361]; 
    int moveCount = 0;
    int n = g->board_size;
    bool visited[19][19] = {false};

    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            if (g->board[y][x] != 0) {
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

    if (moveCount == 0) {
        if (g->board[n/2][n/2] == 0) return evaluateBoard(g); 
        return evaluateBoard(g);
    }

    qsort(moveList, moveCount, sizeof(FastMove), compareFastMoves);

    int best_val = maximizingPlayer ? MIN_SCORE : WINNING_SCORE;
    int alphaOrig = alpha;
    int player = g->turn;
    
// =============================================================
    // DYNAMIC BRANCHING (PRUNING AGRESSIF SELON LA PROFONDEUR)
    // =============================================================
    // Plus on est proche des feuilles (depth petit), moins on regarde de coups.
    // Cela permet de descendre très profond sur les coups principaux.
    
    int max_branches;
    
    if (depth >= 8)      max_branches = 20; // Au début de la recherche (haut de l'arbre), on est large
    else if (depth >= 5) max_branches = 12; // Milieu de partie
    else if (depth >= 3) max_branches = 8;  // On commence à filtrer sévèrement
    else                 max_branches = 5;  // Aux feuilles, on ne regarde que l'essentiel (Top 5)

    // Si on est en situation d'échec/capture critique (score élevé), 
    // on élargit un peu pour ne pas rater la défense.
    if (moveList[0].score > 1000) max_branches += 4;

    int branches = (moveCount > max_branches) ? max_branches : moveCount;

    for (int i = 0; i < branches; i++) {
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
// FIND BEST MOVE (AVEC DEBUG PRINT)
// =========================================================

move findBestMove(game *g, int max_depth_limit) {
    move bestMove = {{-1,-1}, MIN_SCORE};
    uint64_t key = computeZobrist(g);
    stop_search = false;
    clock_gettime(CLOCK_MONOTONIC, &g->ia_timer.start_ts);

    // 1. Génération et tri initial des coups
    FastMove rootMoves[361];
    int moveCount = 0;
    int n = g->board_size;
    bool visited[19][19] = {false};

    for (int y = 0; y < n; y++) {
        for (int x = 0; x < n; x++) {
            if (g->board[y][x] != 0) {
                // Rayon large à la racine (2 cases)
                for (int dy=-2; dy<=2; dy++) {
                    for (int dx=-2; dx<=2; dx++) {
                        int nx=x+dx, ny=y+dy;
                        if (is_valid_pos(nx, ny, n) && g->board[ny][nx] == 0 && !visited[ny][nx]) {
                            visited[ny][nx] = true;
                            rootMoves[moveCount].x = nx;
                            rootMoves[moveCount].y = ny;
                            rootMoves[moveCount].score = rateMoveLight(g, nx, ny, g->turn);
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
    int previous_score = 0; // Pour l'Aspiration Window

    // --- ITERATIVE DEEPENING LOOP ---
    for (int current_depth = 2; current_depth <= max_depth_limit; current_depth += 2) {
        
        // Optimisation ROOT PRUNING :
        // Si on est profond, inutile de regarder les 200 coups possibles.
        // Les 40 premiers suffisent largement (car ils sont triés).
        int moves_to_scan = moveCount;
        if (current_depth >= 6 && moves_to_scan > 40) moves_to_scan = 40;

        int alpha = MIN_SCORE;
        int beta = WINNING_SCORE;
        
        // ASPIRATION WINDOWS LOGIC
        // On suppose que le score sera : previous_score +/- window
        // Cela réduit drastiquement la zone de recherche.
        bool use_aspiration = (current_depth >= 6); // On active seulement si stable
        int window = 400; 
        
        if (use_aspiration) {
            alpha = previous_score - window;
            beta  = previous_score + window;
        }

        // Boucle de tentative (permet de relancer si l'Aspiration Window échoue)
        bool research_needed = true;
        
        while (research_needed) {
            research_needed = false; // Par défaut, on suppose que ça va marcher
            
            int best_val_iter = MIN_SCORE;
            int best_idx_iter = -1;

            // Move Ordering : Meilleur coup précédent en premier
            if (current_depth > 2 && bestMoveIndex > 0 && bestMoveIndex < moves_to_scan) {
                FastMove temp = rootMoves[0];
                rootMoves[0] = rootMoves[bestMoveIndex];
                rootMoves[bestMoveIndex] = temp;
            }

            for (int i = 0; i < moves_to_scan; i++) {
                MoveRecord rec;
                if (!make_move_inplace(g, rootMoves[i].x, rootMoves[i].y, g->turn, &rec, &key)) continue;

                int val = minimax(g, current_depth - 1, alpha, beta, false, &key);
                undo_move_inplace(g, &rec, &key);

                if (stop_search) break;

                if (val > best_val_iter) {
                    best_val_iter = val;
                    best_idx_iter = i;
                }
                
                // Mise à jour de Alpha (classique)
                if (best_val_iter > alpha) alpha = best_val_iter;
                
                // Si on dépasse Beta (Fail High) dans une fenêtre réduite, 
                // c'est que notre prédiction était mauvaise -> on doit ré-élargir.
                if (use_aspiration && best_val_iter >= beta) {
                    // printf(">> Aspiration Fail High at depth %d (Score %d > %d). Re-searching...\n", current_depth, best_val_iter, beta);
                    use_aspiration = false;
                    alpha = MIN_SCORE;
                    beta = WINNING_SCORE;
                    research_needed = true; // On relance la boucle while
                    break; 
                }
            }
            
            if (stop_search) break;

            // Si le meilleur score est inférieur à notre fenêtre (Fail Low),
            // c'est qu'on a surestimé la position -> on doit ré-élargir.
            if (!research_needed && use_aspiration && best_val_iter <= (previous_score - window)) {
                // printf(">> Aspiration Fail Low at depth %d. Re-searching...\n", current_depth);
                use_aspiration = false;
                alpha = MIN_SCORE;
                beta = WINNING_SCORE;
                research_needed = true;
                continue;
            }

            // Si on arrive ici, le résultat est valide
            if (best_idx_iter != -1) {
                printf(">> Depth %d completed. Move: (%d, %d) Score: %d\n", current_depth, rootMoves[best_idx_iter].x, rootMoves[best_idx_iter].y, best_val_iter);
                previous_score = best_val_iter;
                bestMoveIndex = best_idx_iter;
                bestMove.position.x = rootMoves[best_idx_iter].x;
                bestMove.position.y = rootMoves[best_idx_iter].y;
                bestMove.score = best_val_iter;
            }
        }

        if (stop_search) {
            printf(">> TIMEOUT reached while searching Depth %d\n", current_depth);
            break;
        }

        if (previous_score >= WINNING_SCORE - 5000) {
            printf(">> Winning move found at Depth %d!\n", current_depth);
            break;
        }
    }

    return bestMove;
}