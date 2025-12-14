#include "../include/gomoku.h"

// --- Poids des scores (Rappel) ---
// Assurez-vous que ces valeurs sont bien dans gomoku.h ou ici
#define WIN_SCORE 100000000
#define OPEN_FOUR 10000000
#define CLOSED_FOUR 100000
#define OPEN_THREE 100000
#define CLOSED_THREE 1000
#define OPEN_TWO 100

// Helper pour évaluer une séquence
static int evaluate_sequence(int consecutive, int open_ends) {
    if (consecutive >= 5) return WIN_SCORE;
    if (consecutive == 4) {
        if (open_ends == 2) return OPEN_FOUR;
        if (open_ends == 1) return CLOSED_FOUR;
    }
    if (consecutive == 3) {
        if (open_ends == 2) return OPEN_THREE;
        if (open_ends == 1) return CLOSED_THREE;
    }
    if (consecutive == 2 && open_ends == 2) return OPEN_TWO;
    return 0;
}

/*
    Calcule le score des 4 lignes (H, V, D1, D2) passant par (x, y)
    pour un joueur donné.
*/
int get_point_score(game *g, int x, int y, int player) {
    int total_score = 0;
    
    // Les 4 directions : Horizontal, Vertical, Diagonale \, Diagonale /
    int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0];
        int dy = dirs[d][1];
        
        // On scanne toute la ligne (de bord à bord ou limites raisonnables)
        // Pour être sûr de ne rien rater, on peut scanner une fenêtre de 9 cases centrée sur x,y
        // Ou scanner la ligne entière. Le plus simple et sûr est la ligne entière.
        
        int consecutive = 0;
        int open_ends = 0;
        int current_line_score = 0;

        // On cherche le début de la ligne (on recule tant qu'on est dans le board)
        int sx = x, sy = y;
        while (IS_VALID(sx - dx, sy - dy)) {
            sx -= dx;
            sy -= dy;
        }

        // On parcourt la ligne dans le sens avant
        int tx = sx, ty = sy;
        while (IS_VALID(tx, ty)) {
            int idx = GET_INDEX(tx, ty);
            int cell = g->board[idx];

            if (cell == player) {
                consecutive++;
            } else {
                // Fin d'une séquence
                if (consecutive > 0) {
                    // Check open ends
                    // Arrière: (tx - (cons+1)*dx, ...)
                    int bx = tx - (consecutive + 1) * dx;
                    int by = ty - (consecutive + 1) * dy;
                    int ends = 0;
                    
                    if (IS_VALID(bx, by) && g->board[GET_INDEX(bx, by)] == EMPTY) ends++;
                    if (cell == EMPTY) ends++; // Avant (la case actuelle)

                    current_line_score += evaluate_sequence(consecutive, ends);
                }
                consecutive = 0;
            }
            tx += dx;
            ty += dy;
        }
        // Fin de ligne, check dernière séquence
        if (consecutive > 0) {
             int bx = tx - (consecutive + 1) * dx;
             int by = ty - (consecutive + 1) * dy;
             int ends = 0;
             if (IS_VALID(bx, by) && g->board[GET_INDEX(bx, by)] == EMPTY) ends++;
             // Pas d'open end après car on est hors board
             current_line_score += evaluate_sequence(consecutive, ends);
        }
        
        total_score += current_line_score;
    }
    return total_score;
}

/* Cette fonction remplace l'ancien evaluate_board complet.
   Elle retourne juste la différence de score (g->score est déjà à jour).
*/
int evaluate_board(game *g, int player) {
    int opponent = (player == P1) ? P2 : P1;
    
    // Score heuristique incrémental
    int score_diff = g->score[player] - g->score[opponent];
    
    // Score des captures (Poids énorme)
    // On ajoute un facteur dynamique : plus on est profond, moins la victoire vaut
    // (pour préférer les victoires rapides), mais ici restons simples.
    if (g->captures[player] >= 5) return WIN_SCORE;
    if (g->captures[opponent] >= 5) return -WIN_SCORE;
    
    score_diff += (g->captures[player] * 200000);
    score_diff -= (g->captures[opponent] * 200000);

    return score_diff;
}