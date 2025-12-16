#include "../include/gomoku.h"
#include <limits.h>
#include <string.h> // Pour memset
#include <stdlib.h> // Pour rand, abs

// Structure interne pour trier les coups
typedef struct {
    int index;
    int score_estim;
} MoveCandidate;

// --- GLOBALES ZOBRIST & TT ---

uint64_t zobrist_table[MAX_BOARD][3]; // [Case][Joueur (Vide, P1, P2)]
TTEntry transposition_table[TT_SIZE];

// Générateur de nombres aléatoires 64 bits
uint64_t rand64() {
    return (uint64_t)rand() ^ ((uint64_t)rand() << 15) ^ 
           ((uint64_t)rand() << 30) ^ ((uint64_t)rand() << 45) ^ 
           ((uint64_t)rand() << 60);
}

// À APPELER UNE FOIS DANS LE MAIN AU DÉBUT DU PROGRAMME
void init_zobrist() {
    for (int i = 0; i < MAX_BOARD; i++) {
        zobrist_table[i][0] = rand64(); // Pas vraiment utilisé pour vide, mais sécu
        zobrist_table[i][P1] = rand64();
        zobrist_table[i][P2] = rand64();
    }
    // Initialiser la TT à 0
    memset(transposition_table, 0, sizeof(transposition_table));
}

// Fonction pour calculer le hash complet (au début ou debug)
uint64_t compute_hash(game *g) {
    uint64_t h = 0;
    for (int i = 0; i < MAX_BOARD; i++) {
        if (g->board[i] != EMPTY) {
            h ^= zobrist_table[i][g->board[i]];
        }
    }
    return h;
}

// Killer Heuristic : [Depth][Slot]. On garde 2 coups tueurs par profondeur.
int killer_moves[MAX_DEPTH][2];

// History Heuristic : [BoardIndex]. Score cumulatif.
int history_heuristic[MAX_BOARD];

// Pour le debug des cutoffs
long long debug_node_count = 0;
long long debug_cutoff_count = 0;

// Reset des heuristiques au début d'un tour complet de l'IA
void clear_heuristics() {
    memset(killer_moves, -1, sizeof(killer_moves));
    memset(history_heuristic, 0, sizeof(history_heuristic));
    debug_node_count = 0;
    debug_cutoff_count = 0;
}

// --- GESTION DE LA TRANSPOSITION TABLE ---

void tt_save(uint64_t key, int depth, int val, int flag, int best_move) {
    int idx = key % TT_SIZE; 
    
    // Stratégie de remplacement : 
    // On remplace si la nouvelle entrée est plus profonde ou si c'est une collision ancienne
    if (transposition_table[idx].key != key || depth >= transposition_table[idx].depth) {
        transposition_table[idx].key = key;
        transposition_table[idx].depth = depth;
        transposition_table[idx].value = val;
        transposition_table[idx].flag = flag;
        transposition_table[idx].best_move = best_move;
    }
}

TTEntry* tt_probe(uint64_t key) {
    int idx = key % TT_SIZE;
    if (transposition_table[idx].key == key) {
        return &transposition_table[idx];
    }
    return NULL;
}

// --- FONCTIONS UTILITAIRES ---

/* Vérifie si une case a des voisins (rayon 2) non vides. */
bool has_neighbors(game *g, int idx) {
    int cx = GET_X(idx);
    int cy = GET_Y(idx);
    int radius = 2; // Rayon de recherche

    for (int y = cy - radius; y <= cy + radius; y++) {
        for (int x = cx - radius; x <= cx + radius; x++) {
            if (!IS_VALID(x, y)) continue;
            if (g->board[GET_INDEX(x, y)] != EMPTY) return true;
        }
    }
    return false;
}

/* Compare pour le qsort (Tri des coups) */
int compare_moves(const void *a, const void *b) {
    MoveCandidate *m1 = (MoveCandidate *)a;
    MoveCandidate *m2 = (MoveCandidate *)b;
    // Tri décroissant
    return m2->score_estim - m1->score_estim;
}

/* Évaluation rapide d'un coup unique pour le tri (Move Ordering). */
int quick_evaluate_move(game *g, int idx, int player) {
    int score = 0;
    int opponent = (player == P1) ? P2 : P1;
    
    int x = GET_X(idx);
    int y = GET_Y(idx);

    // 1. Potentiel d'ATTAQUE
    g->board[idx] = player; 
    int attack_score = get_point_score(g, x, y, player);
    
    // 2. Potentiel de DÉFENSE
    g->board[idx] = opponent;
    int defense_score = get_point_score(g, x, y, opponent);
    
    g->board[idx] = EMPTY;

    score = attack_score + defense_score;
    return score;
}

/* Génère les coups et les trie par Move Ordering Avancé */
int generate_moves(game *g, MoveCandidate *moves, int player, int depth, int tt_best_move) {
    int count = 0;
    int center = BOARD_SIZE / 2;

    for (int i = 0; i < MAX_BOARD; i++) {
        // Filtre de base
        if (g->board[i] != EMPTY) continue;
        if (!has_neighbors(g, i)) continue;

        moves[count].index = i;
        int score = 0;

        // --- HIERARCHIE DE TRI ---

        // 1. PV-MOVE / TT-MOVE (Priorité Absolue)
        if (i == tt_best_move) {
            score = 20000000; 
        }
        // 2. KILLER MOVES (Priorité Haute)
        else if (i == killer_moves[depth][0]) {
            score = 10000000;
        }
        else if (i == killer_moves[depth][1]) {
            score = 9000000;
        }
        // 3. HISTORY + TACTIQUE (Le reste)
        else {
            int tactical = quick_evaluate_move(g, i, player);
            int history = history_heuristic[i];
            int dist_center = abs(GET_X(i) - center) + abs(GET_Y(i) - center);
            
            score = tactical + history + (100 - dist_center);
        }

        moves[count].score_estim = score;
        count++;
    }

    // Cas plateau vide (premier coup)
    if (count == 0) {
        int center_idx = GET_INDEX(center, center);
        if (g->board[center_idx] == EMPTY) {
            moves[0].index = center_idx;
            moves[0].score_estim = 20000000;
            count = 1;
        }
    }

    qsort(moves, count, sizeof(MoveCandidate), compare_moves);

    // --- BEAM SEARCH (LIMITATION DE LARGEUR) ---
    // C'est ici qu'on force la profondeur 10.
    // On ne garde que les meilleurs coups pour éviter l'explosion exponentielle.
    
    int beam_width = 18; // Base un peu plus stricte
    if (depth >= 4) beam_width = 10; // Très sélectif en profondeur

    if (count > beam_width) {
        count = beam_width;
    }

    return count;
}

/* Helper pour mettre à jour le score global incrémentalement */
void update_score_impact(game *g, int idx) {
    int x = GET_X(idx);
    int y = GET_Y(idx);
    
    int p1_before = get_point_score(g, x, y, P1);
    int p2_before = get_point_score(g, x, y, P2);

    g->score[P1] -= p1_before;
    g->score[P2] -= p2_before;
}

// --- LOGIQUE DO / UNDO ---

void apply_move(game *g, int idx, int player, MoveUndo *undo) {
    int x = GET_X(idx);
    int y = GET_Y(idx);
    int opponent = (player == P1) ? P2 : P1;

    undo->move_idx = idx;
    undo->prev_captures[P1] = g->captures[P1];
    undo->prev_captures[P2] = g->captures[P2];

    // 1. Score Update
    update_score_impact(g, idx);
    
    // 2. Pose de pierre et UPDATE ZOBRIST
    g->board[idx] = player;
    g->current_hash ^= zobrist_table[idx][player]; 
    
    g->score[P1] += get_point_score(g, x, y, P1);
    g->score[P2] += get_point_score(g, x, y, P2);

    // 3. Captures
    undo->captured_count = apply_captures_for_ai(g, x, y, player, undo->captured_indices);
    g->captures[player] += (undo->captured_count / 2);

    if (undo->captured_count > 0) {
        for (int i = 0; i < undo->captured_count; i++) {
            int cap_idx = undo->captured_indices[i];
            int cx = GET_X(cap_idx);
            int cy = GET_Y(cap_idx);
            
            // UPDATE ZOBRIST : RETRAIT ADVERSAIRE
            g->current_hash ^= zobrist_table[cap_idx][opponent];

            // Mise à jour scores
            g->board[cap_idx] = opponent; 
            g->score[P1] -= get_point_score(g, cx, cy, P1);
            g->score[P2] -= get_point_score(g, cx, cy, P2);
            g->board[cap_idx] = EMPTY; 
            g->score[P1] += get_point_score(g, cx, cy, P1);
            g->score[P2] += get_point_score(g, cx, cy, P2);
        }
    }
}

void undo_move(game *g, int player, MoveUndo *undo) {
    int idx = undo->move_idx;
    int x = GET_X(idx);
    int y = GET_Y(idx);
    int opponent = (player == P1) ? P2 : P1;

    // A. Annulation Captures
    if (undo->captured_count > 0) {
        for (int i = 0; i < undo->captured_count; i++) {
            int cap_idx = undo->captured_indices[i];
            int cx = GET_X(cap_idx);
            int cy = GET_Y(cap_idx);

            g->score[P1] -= get_point_score(g, cx, cy, P1);
            g->score[P2] -= get_point_score(g, cx, cy, P2);

            g->board[cap_idx] = opponent;
            
            // UPDATE ZOBRIST : ON REMET L'ADVERSAIRE
            g->current_hash ^= zobrist_table[cap_idx][opponent]; 

            g->score[P1] += get_point_score(g, cx, cy, P1);
            g->score[P2] += get_point_score(g, cx, cy, P2);
        }
    }

    // B. Annulation Coup Principal
    g->score[P1] -= get_point_score(g, x, y, P1);
    g->score[P2] -= get_point_score(g, x, y, P2);

    g->board[idx] = EMPTY;
    g->current_hash ^= zobrist_table[idx][player]; 

    g->score[P1] += get_point_score(g, x, y, P1);
    g->score[P2] += get_point_score(g, x, y, P2);

    g->captures[P1] = undo->prev_captures[P1];
    g->captures[P2] = undo->prev_captures[P2];
}

// --- MOTEUR ALPHA-BETA ---
// --- MOTEUR PVS (Principal Variation Search) ---

int minimax(game *g, int depth, int alpha, int beta, bool maximizingPlayer, int ia_player, clock_t start_time) {
    debug_node_count++;

    // 1. TT PROBE (Lecture Mémoire)
    int alpha_orig = alpha; 
    TTEntry *entry = tt_probe(g->current_hash);
    
    if (entry != NULL && entry->depth >= depth) {
        if (entry->flag == TT_EXACT) {
            return entry->value;
        }
        else if (entry->flag == TT_LOWERBOUND) {
            if (entry->value > alpha) alpha = entry->value;
        }
        else if (entry->flag == TT_UPPERBOUND) {
            if (entry->value < beta) beta = entry->value;
        }
        if (alpha >= beta) {
            return entry->value;
        }
    }
    
    // Check Temps
    if ((clock() - start_time) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) return -2;

    int current_eval = evaluate_board(g, ia_player);
    // Condition de fin
    if (depth == 0 || abs(current_eval) > WIN_SCORE / 2) return current_eval;

    MoveCandidate moves[MAX_BOARD];
    int current_player = maximizingPlayer ? ia_player : ((ia_player == P1) ? P2 : P1);

    // Récupérer le coup suggéré par la TT pour le tri
    int tt_move = (entry != NULL) ? entry->best_move : -1; 
    
    int move_count = generate_moves(g, moves, current_player, depth, tt_move);
    if (move_count == 0) return 0;

    int best_val = maximizingPlayer ? INT_MIN : INT_MAX;
    int best_move_this_node = -1; 

    for (int i = 0; i < move_count; i++) {
        MoveUndo undo;
        int idx = moves[i].index;

        apply_move(g, idx, current_player, &undo);
        
        int val;

        // --- LOGIQUE PVS (C'est ici que ça change) ---
        
        if (i == 0) {
            // A. PREMIER COUP (PV-Node) : Recherche Complète
            // On suppose que c'est le meilleur grâce au tri, on cherche avec fenêtre pleine.
            val = minimax(g, depth - 1, alpha, beta, !maximizingPlayer, ia_player, start_time);
        } 
        else {
            // B. AUTRES COUPS : Late Move Reduction (LMR) + Null Window Search
            
            int reduction = 0;

            // Conditions pour activer LMR :
            // 1. Profondeur suffisante (ne pas réduire près des feuilles)
            // 2. On est loin dans la liste (les premiers coups sont les Killers/History)
            if (depth >= 3 && i >= 4) {
                reduction = 1;
                // Si on est très profond et très loin, on réduit plus agressivement
                if (depth > 6 && i > 10) reduction = 2;
                
                // Sécurité : ne pas réduire en dessous de 0
                if (depth - 1 - reduction < 0) reduction = depth - 1;
            }

            if (maximizingPlayer) {
                // 1. Recherche avec RÉDUCTION (Null Window)
                // On cherche : est-ce que ce coup > alpha ?
                val = minimax(g, depth - 1 - reduction, alpha, alpha + 1, !maximizingPlayer, ia_player, start_time);
                
                // 2. Re-search si LMR a échoué (Le coup était meilleur que prévu)
                // Si val > alpha, cela veut dire que même réduit, ce coup est intéressant.
                // On doit vérifier si c'était un faux positif dû à la faible profondeur.
                if (reduction > 0 && val > alpha) {
                    val = minimax(g, depth - 1, alpha, alpha + 1, !maximizingPlayer, ia_player, start_time);
                }

                // 3. Re-search si PVS a échoué (Fail High classique)
                // Le coup est confirmé meilleur que alpha, on a besoin de sa valeur exacte.
                if (val > alpha && val < beta) {
                    val = minimax(g, depth - 1, alpha, beta, !maximizingPlayer, ia_player, start_time);
                }
            } else {
                // Minimizing : On cherche : est-ce que ce coup < beta ?
                
                // 1. Recherche avec RÉDUCTION
                val = minimax(g, depth - 1 - reduction, beta - 1, beta, !maximizingPlayer, ia_player, start_time);
                
                // 2. Re-search si LMR a échoué (Le coup est dangereux pour nous)
                if (reduction > 0 && val < beta) {
                    val = minimax(g, depth - 1, beta - 1, beta, !maximizingPlayer, ia_player, start_time);
                }

                // 3. Re-search si PVS a échoué
                if (val < beta && val > alpha) {
                    val = minimax(g, depth - 1, alpha, beta, !maximizingPlayer, ia_player, start_time);
                }
            }
        }
        // ---------------------------------------------

        undo_move(g, current_player, &undo);

        if (val == -2) return -2; // Timeout propagate

        if (maximizingPlayer) {
            if (val > best_val) {
                best_val = val;
                best_move_this_node = idx;
            }            
            if (val > alpha) alpha = val;
            if (beta <= alpha) {
                // COUPURE BETA
                if (killer_moves[depth][0] != idx) {
                    killer_moves[depth][1] = killer_moves[depth][0];
                    killer_moves[depth][0] = idx;
                }
                history_heuristic[idx] += (depth * depth);
                debug_cutoff_count++;
                break;
            }
        } else {
            if (val < best_val) {
                best_val = val;
                best_move_this_node = idx;
            }
            if (val < beta) beta = val;
            if (beta <= alpha) {
                // COUPURE ALPHA
                if (killer_moves[depth][0] != idx) {
                    killer_moves[depth][1] = killer_moves[depth][0];
                    killer_moves[depth][0] = idx;
                }
                history_heuristic[idx] += (depth * depth);
                debug_cutoff_count++;
                break; 
            }
        }
    }

    // 2. TT SAVE (Écriture Mémoire)
    int flag;
    if (best_val <= alpha_orig) flag = TT_UPPERBOUND; 
    else if (best_val >= beta) flag = TT_LOWERBOUND;
    else flag = TT_EXACT;
    
    tt_save(g->current_hash, depth, best_val, flag, best_move_this_node);

    return best_val;
}

// --- FONCTION PRINCIPALE APPELÉE PAR LE MAIN ---

void makeIaMove(game *gameData, screen *windows) {
    if (gameData->game_over) return;

    clock_t start = clock();
    int best_move_idx = -1;
    int ia_player = gameData->iaTurn; 
    
    // Reset des stats pour ce tour
    clear_heuristics(); 

    // Optimisation premier coup (centre)
    if (gameData->board[GET_INDEX(BOARD_SIZE/2, BOARD_SIZE/2)] == EMPTY && 
        gameData->score[P1] == 0 && gameData->score[P2] == 0) {
         bool empty = true;
         for(int i=0; i<MAX_BOARD; i++) if(gameData->board[i] != EMPTY) { empty = false; break; }
         if (empty) {
             best_move_idx = GET_INDEX(BOARD_SIZE/2, BOARD_SIZE/2);
             goto play_move;
         }
    }

    // --- ITERATIVE DEEPENING AVEC ASPIRATION WINDOWS ---
    // On stocke le score de la profondeur précédente pour prédire le futur
    int prev_score = 0; 

    for (int depth = 2; depth <= MAX_DEPTH; depth += 2) {
        
        // Préparation de la fenêtre d'aspiration
        int alpha_start = INT_MIN;
        int beta_start = INT_MAX;
        int window = 500; // Largeur de la fenêtre (à ajuster, 500 est standard pour Gomoku)

        // À partir de profondeur 4, on tente de deviner le score
        if (depth > 2) {
            alpha_start = prev_score - window;
            beta_start = prev_score + window;
        }

        // Boucle de "Retry" : Si la fenêtre échoue, on recommence avec l'infini
        // false au départ, true tant qu'on doit chercher
        bool need_research = true; 
        
        // Sauvegarde des coups pour ne pas regénérer en cas de retry
        MoveCandidate moves[MAX_BOARD];
        int previous_best_move = best_move_idx;
        int count = generate_moves(gameData, moves, ia_player, depth, previous_best_move);

        // Fallback premier coup
        if (best_move_idx == -1 && count > 0) best_move_idx = moves[0].index;
        if (count == 0) return;

        while (need_research) {
            // Par défaut, on suppose qu'on réussira du premier coup, on ne recommencera pas
            need_research = false; 

            int alpha = alpha_start;
            int beta = beta_start;
            int current_best_idx = -1;
            int current_best_score = INT_MIN;
            
            bool time_out = false;
            game working_game = *gameData; 

            for (int i = 0; i < count; i++) {
                int idx = moves[i].index;
                MoveUndo undo;

                apply_move(&working_game, idx, ia_player, &undo);
                
                // Appel PVS (qui utilise alpha/beta)
                int val = minimax(&working_game, depth - 1, alpha, beta, false, ia_player, start);
                
                undo_move(&working_game, ia_player, &undo);

                if (val == -2) { 
                    time_out = true;
                    break; 
                }

                if (val > current_best_score) {
                    current_best_score = val;
                    current_best_idx = idx;
                }
                
                // Mise à jour dynamique d'alpha (classique)
                if (val > alpha) alpha = val;
            }

            // --- GESTION DU TIMEOUT ---
            if (time_out) {
                #ifdef DEBUG
                    printf("Timeout at depth %d. Keeping best move from depth %d.\n", depth, depth-2);
                #endif
                goto end_search; // On sort de tout
            }

            // --- GESTION ASPIRATION FAILURE (Le cœur du système) ---
            // Si le score est hors de la fenêtre, notre prédiction était mauvaise.
            if (depth > 2 && (current_best_score <= alpha_start || current_best_score >= beta_start)) {
                #ifdef DEBUG
                    printf("Aspiration Fail at depth %d (Score %d outside [%d, %d]). Re-searching full window.\n", 
                           depth, current_best_score, alpha_start, beta_start);
                #endif
                
                // On élargit à l'infini pour être sûr de trouver le vrai score
                alpha_start = INT_MIN;
                beta_start = INT_MAX;
                need_research = true; // On relance la boucle while
                continue; 
            }

            // Si on arrive ici, le score est valide ou on a fini le re-search
            best_move_idx = current_best_idx;
            prev_score = current_best_score; // On mémorise pour la prochaine profondeur

            #ifdef DEBUG
                printf("Depth %d complete. Best: %d. Nodes: %lld, Cutoffs: %lld.\n", 
                    depth, current_best_score, debug_node_count, debug_cutoff_count);
            #endif
            
            if (current_best_score > WIN_SCORE / 2) goto end_search;
        } // Fin while(retry)

        if ((clock() - start) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) break;
    }

    end_search:;

play_move:
    if (best_move_idx != -1) {
        int x = GET_X(best_move_idx);
        int y = GET_Y(best_move_idx);
        
        MoveUndo final_undo;
        apply_move(gameData, best_move_idx, ia_player, &final_undo);

        drawSquare(windows, x, y, ia_player);
        
        if (final_undo.captured_count > 0) {
            for (int k = 0; k < final_undo.captured_count; k++) {
                int cap_idx = final_undo.captured_indices[k];
                drawSquare(windows, GET_X(cap_idx), GET_Y(cap_idx), EMPTY);
            }
        }

        windows->changed = true; 
        
        #ifdef DEBUG
            printf("IA plays at (%d, %d)\n", x, y);
        #endif
    } else {
        #ifdef DEBUG
            printf("IA cannot move.\n");
        #endif
    }

    clock_t end = clock();
    gameData->ia_timer.elapsed = (double)(end - start) / CLOCKS_PER_SEC;
}

// --- FONCTION D'AIDE AU JOUEUR (HINT) ---

void suggest_move(game *g, screen *s, int player) {
    // On utilise la même logique que makeIaMove (Iterative Deepening)
    // pour trouver le meilleur coup possible en profondeur.
    
    clock_t start = clock();
    int best_move_idx = -1;
    
    // On nettoie les heuristiques pour une nouvelle recherche
    clear_heuristics(); 

    // --- ITERATIVE DEEPENING (Copie adaptée de makeIaMove) ---
    int prev_score = 0; 

    for (int depth = 2; depth <= MAX_DEPTH; depth += 2) {
        
        int alpha_start = INT_MIN;
        int beta_start = INT_MAX;
        int window = 500;

        if (depth > 2) {
            alpha_start = prev_score - window;
            beta_start = prev_score + window;
        }

        bool need_research = true; 
        
        MoveCandidate moves[MAX_BOARD];
        int previous_best_move = best_move_idx;
        // On génère les coups pour le JOUEUR HUMAIN (player)
        int count = generate_moves(g, moves, player, depth, previous_best_move);

        if (best_move_idx == -1 && count > 0) best_move_idx = moves[0].index;
        if (count == 0) return;

        while (need_research) {
            need_research = false; 

            int alpha = alpha_start;
            int beta = beta_start;
            int current_best_idx = -1;
            int current_best_score = INT_MIN;
            
            bool time_out = false;
            game working_game = *g; // Copie locale pour simuler

            for (int i = 0; i < count; i++) {
                int idx = moves[i].index;
                MoveUndo undo;

                apply_move(&working_game, idx, player, &undo);
                
                // On appelle minimax en considérant 'player' comme le maximisateur
                int val = minimax(&working_game, depth - 1, alpha, beta, false, player, start);
                
                undo_move(&working_game, player, &undo);

                if (val == -2) { 
                    time_out = true;
                    break; 
                }

                if (val > current_best_score) {
                    current_best_score = val;
                    current_best_idx = idx;
                }
                
                if (val > alpha) alpha = val;
            }

            if (time_out) goto end_search;

            // Aspiration Window Failure Check
            if (depth > 2 && (current_best_score <= alpha_start || current_best_score >= beta_start)) {
                alpha_start = INT_MIN;
                beta_start = INT_MAX;
                need_research = true;
                continue; 
            }

            best_move_idx = current_best_idx;
            prev_score = current_best_score;
            
            if (current_best_score > WIN_SCORE / 2) goto end_search;
        }

        // On arrête si on dépasse le temps imparti
        if ((clock() - start) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) break;
    }

    end_search:;

    // Affichage du résultat
    if (best_move_idx != -1) {
        g->hint_idx = best_move_idx;
        s->changed = true;
        
        #ifdef DEBUG
            printf("💡 Smart Hint found at (%d, %d)\n", GET_X(best_move_idx), GET_Y(best_move_idx));
        #endif
    } else {
        printf("❌ No hint found.\n");
    }
}