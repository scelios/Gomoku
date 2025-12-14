#include "../include/gomoku.h"
#include <limits.h>

// Limites de temps et de profondeur
#define MAX_DEPTH 10
#define TIME_LIMIT_MS 450 // On garde une marge de sécurité (50ms) pour l'affichage

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

// --- MOTEUR ALPHA-BETA ---

/* Retourne le score du plateau du point de vue du joueur 'ia_player'.
   alpha : le meilleur score déjà garanti pour l'IA
   beta : le meilleur score déjà garanti pour l'adversaire (Minimizer)
*/
int minimax(game *g, int depth, int alpha, int beta, bool maximizingPlayer, int ia_player, clock_t start_time) {
    
    // 1. Check Temps (Safety Stop)
    if ((clock() - start_time) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) {
        return -2; // Code d'erreur pour "Temps écoulé"
    }

    // 2. Vérification Fin de partie ou Profondeur atteinte
    // Note: checkVictoryCondition est trop lourd, on utilise evaluate_board ou on check les captures
    int current_eval = evaluate_board(g, ia_player);
    
    // Si victoire détectée (score très haut) ou profondeur 0
    if (depth == 0 || abs(current_eval) > WIN_SCORE / 2) {
        return current_eval;
    }

    // 3. Génération des coups
    MoveCandidate moves[MAX_BOARD];
    int move_count = generate_moves(g, moves);
    
    if (move_count == 0) return 0; // Match nul

    int best_val;
    
    if (maximizingPlayer) {
        best_val = INT_MIN;
        for (int i = 0; i < move_count; i++) {
            // SIMULATION DU COUP
            // On copie le jeu pour ne pas avoir à gérer un "Undo" complexe avec les captures
            game next_state = *g; 
            int idx = moves[i].index;
            
            next_state.board[idx] = ia_player;
            // Gestion des captures (sans affichage window = NULL)
            checkPieceCapture(&next_state, NULL, GET_X(idx), GET_Y(idx));
            
            // APPEL RÉCURSIF
            int val = minimax(&next_state, depth - 1, alpha, beta, false, ia_player, start_time);
            
            // Si timeout remonté
            if (val == -2) return -2; 

            if (val > best_val) best_val = val;
            if (val > alpha) alpha = val;
            if (beta <= alpha) break; // Élagage Beta
        }
    } else {
        // Tour de l'adversaire (Minimizing)
        int opponent = (ia_player == P1) ? P2 : P1;
        best_val = INT_MAX;
        
        for (int i = 0; i < move_count; i++) {
            game next_state = *g;
            int idx = moves[i].index;
            
            next_state.board[idx] = opponent;
            checkPieceCapture(&next_state, NULL, GET_X(idx), GET_Y(idx));
            
            int val = minimax(&next_state, depth - 1, alpha, beta, true, ia_player, start_time);
            
            if (val == -2) return -2;

            if (val < best_val) best_val = val;
            if (val < beta) beta = val;
            if (beta <= alpha) break; // Élagage Alpha
        }
    }
    return best_val;
}

// --- FONCTION PRINCIPALE APPELÉE PAR LE MAIN ---

void makeIaMove(game *gameData, screen *windows) {
    if (gameData->game_over) return;

    clock_t start = clock();
    int best_move_idx = -1;
    int ia_player = gameData->iaTurn; // 1 ou 2

    // Si c'est le tout premier coup et que le plateau est vide, jouer au centre direct
    // Optimisation pour éviter de réfléchir pour rien
    if (gameData->board[GET_INDEX(BOARD_SIZE/2, BOARD_SIZE/2)] == EMPTY && 
        gameData->score[P1] == 0 && gameData->score[P2] == 0) { // Check rapide si vide
         bool empty = true;
         for(int i=0; i<MAX_BOARD; i++) if(gameData->board[i] != EMPTY) { empty = false; break; }
         if (empty) {
             best_move_idx = GET_INDEX(BOARD_SIZE/2, BOARD_SIZE/2);
             goto play_move;
         }
    }

    // --- ITERATIVE DEEPENING ---
    // On commence à profondeur 2, et on augmente tant qu'on a du temps.
    for (int depth = 2; depth <= MAX_DEPTH; depth += 2) {
        
        int alpha = INT_MIN;
        int beta = INT_MAX;
        int current_best_idx = -1;
        int current_best_score = INT_MIN;

        // On génère les coups à la racine
        MoveCandidate moves[MAX_BOARD];
        int count = generate_moves(gameData, moves);

        if (count == 0) return; // Plus de coups

        bool time_out = false;

        // Boucle sur les coups racines
        for (int i = 0; i < count; i++) {
            game next_state = *gameData;
            int idx = moves[i].index;

            next_state.board[idx] = ia_player;
            checkPieceCapture(&next_state, NULL, GET_X(idx), GET_Y(idx));

            // Appel Minimax
            int val = minimax(&next_state, depth - 1, alpha, beta, false, ia_player, start);

            if (val == -2) { // Timeout détecté
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
            break; // On arrête et on garde le résultat de la profondeur précédente (ou celle en cours partielle)
        } else {
            // Si on a fini la profondeur sans timeout, on valide ce coup comme le nouveau meilleur
            best_move_idx = current_best_idx;
            printf("Depth %d complete. Best score: %d\n", depth, current_best_score);
            
            // Si on a trouvé une victoire forcée, inutile d'aller plus loin
            if (current_best_score > WIN_SCORE / 2) break;
        }
        
        // Check temps avant de relancer une profondeur
        if ((clock() - start) * 1000 / CLOCKS_PER_SEC > TIME_LIMIT_MS) break;
    }

play_move:
    if (best_move_idx != -1) {
        int x = GET_X(best_move_idx);
        int y = GET_Y(best_move_idx);
        
        gameData->board[best_move_idx] = ia_player;
        
        checkPieceCapture(gameData, windows, x, y);
        drawSquare(windows, x, y, ia_player);
        
        printf("IA plays at (%d, %d)\n", x, y);
    } else {
        printf("IA cannot move / No move found.\n");
    }

    // --- AJOUT : Stockage du temps ---
    clock_t end = clock();
    gameData->ia_timer.elapsed = (double)(end - start) / CLOCKS_PER_SEC;
}