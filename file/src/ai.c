#include "../include/gomoku.h"
#include <limits.h>

// Structure interne pour trier les coups
typedef struct {
    int index;
    int score_estim;
} MoveCandidate;

// --- FONCTIONS UTILITAIRES ---

/* Vérifie si une case a des voisins (rayon 2) non vides.
   Cela permet de ne pas calculer des coups au milieu de nulle part. */
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
    // Tri décroissant : les meilleurs scores en premier
    return m2->score_estim - m1->score_estim;
}

/* Génère les coups plausibles et les trie grossièrement */
int generate_moves(game *g, MoveCandidate *moves) {
    int count = 0;
    int center = BOARD_SIZE / 2; // 9 pour un plateau 19x19

    for (int i = 0; i < MAX_BOARD; i++) {
        // On ne peut jouer que sur une case vide
        if (g->board[i] != EMPTY) continue;

        // Si la case n'a pas de voisins (rayon 2), on l'ignore pour gagner du temps
        if (!has_neighbors(g, i)) continue;

        moves[count].index = i;
        
        // --- Heuristique de tri (Move Ordering) ---
        int score = 0;
        // Petit bonus pour le centre
        int dist_center = abs(GET_X(i) - center) + abs(GET_Y(i) - center);
        score -= dist_center; 

        moves[count].score_estim = score;
        count++;
    }

    // --- CAS SPÉCIAL : DÉBUT DE PARTIE ---
    // Si 'count' est 0 ici, cela signifie que le plateau est vide 
    // (ou que les pierres sont trop loin, ce qui est impossible si on scanne tout).
    // On doit absolument proposer au moins un coup : le Centre.
    if (count == 0) {
        int center_idx = GET_INDEX(center, center);
        if (g->board[center_idx] == EMPTY) {
            moves[0].index = center_idx;
            moves[0].score_estim = 1000;
            count = 1;
        }
    }

    // Trier les coups
    qsort(moves, count, sizeof(MoveCandidate), compare_moves);
    return count;
}

/* Helper pour mettre à jour le score global incrémentalement */
void update_score_impact(game *g, int idx, bool is_adding_piece) {
    int x = GET_X(idx);
    int y = GET_Y(idx);
    
    // 1. Calculer le score qu'apportaient ces lignes AVANT le changement
    // (Note: on le fait pour les DEUX joueurs car poser un pion peut briser une ligne adverse)
    int p1_before = get_point_score(g, x, y, P1);
    int p2_before = get_point_score(g, x, y, P2);

    // 2. On change temporairement la case pour calculer l'après
    // ATTENTION : Cette fonction suppose que g->board[idx] a l'ancienne valeur
    // Si is_adding_piece est true, le board est VIDE, on va mettre la PIECE
    // Si false (undo), le board a la PIECE, on va mettre VIDE.
    
    // Mais pour simplifier l'appel dans apply_move, on va faire :
    // Retirer les scores actuels -> Modifier Board -> Ajouter les nouveaux scores
    
    g->score[P1] -= p1_before;
    g->score[P2] -= p2_before;
}

// --- LOGIQUE DO / UNDO (Le cœur de l'optimisation) ---

void apply_move(game *g, int idx, int player, MoveUndo *undo) {
    int x = GET_X(idx);
    int y = GET_Y(idx);
    int opponent = (player == P1) ? P2 : P1;

    undo->move_idx = idx;
    undo->prev_captures[P1] = g->captures[P1];
    undo->prev_captures[P2] = g->captures[P2];

    // --- A. GESTION DU COUP PRINCIPAL ---
    
    // 1. On retire le score des lignes actuelles (P1 et P2) passant par ce point vide
    // (Car elles vont être modifiées)
    update_score_impact(g, idx, true); // true = dummy param ici, la logique est ci-dessus
    
    // 2. On pose la pierre
    g->board[idx] = player;
    
    // 3. On ajoute le nouveau score des lignes (maintenant qu'il y a un pion)
    g->score[P1] += get_point_score(g, x, y, P1);
    g->score[P2] += get_point_score(g, x, y, P2);


    // --- B. GESTION DES CAPTURES ---
    
    // On détecte les captures
    undo->captured_count = apply_captures_for_ai(g, x, y, player, undo->captured_indices);
    g->captures[player] += (undo->captured_count / 2);

    // Si des captures ont eu lieu, le score du plateau a changé LA OU C'EST VIDE MAINTENANT !
    if (undo->captured_count > 0) {
        for (int i = 0; i < undo->captured_count; i++) {
            int cap_idx = undo->captured_indices[i];
            int cx = GET_X(cap_idx);
            int cy = GET_Y(cap_idx);
            
            // Le pion adverse vient d'être retiré (c'est maintenant EMPTY)
            // Donc AVANT (juste avant la ligne ci-dessous), il y avait un pion OPPONENT.
            // Mais apply_captures_for_ai a DEJA mis EMPTY dans le board.
            
            // Donc : Le board est EMPTY à cap_idx.
            // Il faut recalculer le score actuel (vide) et l'ajouter.
            // Et il faut soustraire ce qu'il valait quand il y avait 'opponent'.
            
            // Astuce : Pour soustraire correctement, on remet temporairement le pion
            g->board[cap_idx] = opponent; 
            g->score[P1] -= get_point_score(g, cx, cy, P1);
            g->score[P2] -= get_point_score(g, cx, cy, P2);
            
            g->board[cap_idx] = EMPTY; // On remet vide
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

    // --- A. ANNULATION DES CAPTURES (D'abord remettre les pions) ---
    if (undo->captured_count > 0) {
        for (int i = 0; i < undo->captured_count; i++) {
            int cap_idx = undo->captured_indices[i];
            int cx = GET_X(cap_idx);
            int cy = GET_Y(cap_idx);

            // Actuellement c'est VIDE. On retire le score du vide.
            g->score[P1] -= get_point_score(g, cx, cy, P1);
            g->score[P2] -= get_point_score(g, cx, cy, P2);

            // On remet le pion adverse
            g->board[cap_idx] = opponent;

            // On ajoute le score du pion
            g->score[P1] += get_point_score(g, cx, cy, P1);
            g->score[P2] += get_point_score(g, cx, cy, P2);
        }
    }

    // --- B. ANNULATION DU COUP PRINCIPAL ---
    
    // Actuellement il y a le PION JOUEUR. On retire son score.
    g->score[P1] -= get_point_score(g, x, y, P1);
    g->score[P2] -= get_point_score(g, x, y, P2);

    // On retire la pierre
    g->board[idx] = EMPTY;

    // On remet le score du vide (lignes potentiellement coupées qui se reconnectent ?)
    // Note: Le vide peut connecter des pièces adverses, donc il faut recalculer.
    g->score[P1] += get_point_score(g, x, y, P1);
    g->score[P2] += get_point_score(g, x, y, P2);

    // Restaurer les compteurs
    g->captures[P1] = undo->prev_captures[P1];
    g->captures[P2] = undo->prev_captures[P2];
}
// --- MOTEUR ALPHA-BETA ---

/* Retourne le score du plateau du point de vue du joueur 'ia_player'.
   alpha : le meilleur score déjà garanti pour l'IA
   beta : le meilleur score déjà garanti pour l'adversaire (Minimizer)
*/

int minimax(game *g, int depth, int alpha, int beta, bool maximizingPlayer, int ia_player, clock_t start_time) {
    
    // Check Temps
    if ((clock() - start_time) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) return -2;

    int current_eval = evaluate_board(g, ia_player);
    if (depth == 0 || abs(current_eval) > WIN_SCORE / 2) return current_eval;

    MoveCandidate moves[MAX_BOARD];
    int move_count = generate_moves(g, moves);
    if (move_count == 0) return 0;

    int best_val = maximizingPlayer ? INT_MIN : INT_MAX;
    int current_player = maximizingPlayer ? ia_player : ((ia_player == P1) ? P2 : P1);

    for (int i = 0; i < move_count; i++) {
        MoveUndo undo;
        
        // --- DO MOVE ---
        apply_move(g, moves[i].index, current_player, &undo);
        
        // --- RECURSION ---
        int val = minimax(g, depth - 1, alpha, beta, !maximizingPlayer, ia_player, start_time);
        
        // --- UNDO MOVE ---
        undo_move(g, current_player, &undo);

        if (val == -2) return -2;

        if (maximizingPlayer) {
            if (val > best_val) best_val = val;
            if (val > alpha) alpha = val;
            if (beta <= alpha) break;
        } else {
            if (val < best_val) best_val = val;
            if (val < beta) beta = val;
            if (beta <= alpha) break;
        }
    }
    return best_val;
}

// --- FONCTION PRINCIPALE APPELÉE PAR LE MAIN ---

void makeIaMove(game *gameData, screen *windows) {
    if (gameData->game_over) return;

    clock_t start = clock();
    int best_move_idx = -1;
    int ia_player = gameData->iaTurn; 

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

    // --- ITERATIVE DEEPENING ---
    for (int depth = 2; depth <= MAX_DEPTH; depth += 2) {
        
        int alpha = INT_MIN;
        int beta = INT_MAX;
        int current_best_idx = -1;
        int current_best_score = INT_MIN;

        // On génère les coups
        MoveCandidate moves[MAX_BOARD];
        int count = generate_moves(gameData, moves);

        // --- SECURITÉ FALLBACK ---
        // Si generate_moves trouve des coups mais qu'on timeout instantanément (très rare)
        // on doit jouer le premier coup "pas trop mauvais" du tri
        if (best_move_idx == -1 && count > 0) {
            best_move_idx = moves[0].index; 
        }

        if (count == 0) return;

        bool time_out = false;

        // --- CORRECTION MAJEURE ICI ---
        // On crée une copie de travail UNIQUE pour ne pas corrompre le vrai jeu si on timeout
        game working_game = *gameData; 

        for (int i = 0; i < count; i++) {
            int idx = moves[i].index;
            MoveUndo undo;

            // 1. On utilise apply_move pour avoir le score incrémental à jour !
            apply_move(&working_game, idx, ia_player, &undo);

            // 2. Appel Minimax
            int val = minimax(&working_game, depth - 1, alpha, beta, false, ia_player, start);

            // 3. On annule immédiatement pour nettoyer la working_game
            undo_move(&working_game, ia_player, &undo);

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

        if (time_out) {
            printf("Timeout at depth %d. Keeping best move from depth %d.\n", depth, depth-2);
            break; 
        } else {
            best_move_idx = current_best_idx;
            printf("Depth %d complete. Best score: %d\n", depth, current_best_score);
            if (current_best_score > WIN_SCORE / 2) break;
        }
        
        if ((clock() - start) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) break;
    }

play_move:
    if (best_move_idx != -1) {
        int x = GET_X(best_move_idx);
        int y = GET_Y(best_move_idx);
        
        // 1. Appliquer le coup logiquement (met à jour le board et remplit final_undo)
        MoveUndo final_undo;
        apply_move(gameData, best_move_idx, ia_player, &final_undo);

        // 2. Dessiner le pion que l'IA vient de poser
        drawSquare(windows, x, y, ia_player);
        
        // --- CORRECTION ICI ---
        // 3. Si des captures ont eu lieu, il faut les EFFACER visuellement tout de suite
        if (final_undo.captured_count > 0) {
            for (int k = 0; k < final_undo.captured_count; k++) {
                int cap_idx = final_undo.captured_indices[k];
                // On dessine du VIDE (Noir) aux coordonnées des pierres capturées
                drawSquare(windows, GET_X(cap_idx), GET_Y(cap_idx), EMPTY);
            }
        }
        // ----------------------

        // 4. Mettre à jour les infos et forcer le rafraîchissement
        windows->changed = true; 
        
        printf("IA plays at (%d, %d)\n", x, y);
    } else {
        printf("IA cannot move.\n");
    }

    clock_t end = clock();
    gameData->ia_timer.elapsed = (double)(end - start) / CLOCKS_PER_SEC;
}