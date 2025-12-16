#include "../include/gomoku.h"

// Tables de score pré-calculées (à ajuster selon vos préférences)
// C'est beaucoup plus rapide que des 'if' en cascade
static const int SCORE_TABLE[6][3] = {
    // [Consecutive][OpenEnds]
    {0, 0, 0},             // 0 pierres
    {1, 10, 15},           // 1 pierre  (Fermé, 1 bout, 2 bouts)
    {10, 50, 200},          // 2 pierres
    {100, 1000, 5000},      // 3 pierres
    {5000, 10000, 50000},   // 4 pierres
    {WIN_SCORE, WIN_SCORE, WIN_SCORE} // 5 pierres
};

/*
    Version OPTIMISÉE de get_point_score.
    Au lieu de scanner toute la ligne, on scanne juste un segment de 9 cases 
    (4 avant, 4 après, 1 milieu) centré sur (x,y).
*/
int get_point_score(game *g, int x, int y, int player) {
    int total_score = 0;
    int opponent = (player == P1) ? P2 : P1;

    // Directions: H, V, D1, D2
    const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (int d = 0; d < 4; d++) {
        int dx = dirs[d][0];
        int dy = dirs[d][1];
        
        int consecutive = 1; // La pierre qu'on vient de poser
        int open_ends = 0;
        
        // --- 1. Regarder "AVANT" (ex: vers la gauche) ---
        int i;
        for (i = 1; i <= 4; i++) {
            int tx = x - i * dx;
            int ty = y - i * dy;
            
            if (!IS_VALID(tx, ty)) break; // Bord du plateau
            
            int cell = g->board[GET_INDEX(tx, ty)];
            if (cell == player) {
                consecutive++;
            } else if (cell == EMPTY) {
                open_ends++;
                break; // Fin de la séquence par une case vide
            } else {
                break; // Bloqué par l'adversaire
            }
        }

        // --- 2. Regarder "APRÈS" (ex: vers la droite) ---
        for (i = 1; i <= 4; i++) {
            int tx = x + i * dx;
            int ty = y + i * dy;
            
            if (!IS_VALID(tx, ty)) break;
            
            int cell = g->board[GET_INDEX(tx, ty)];
            if (cell == player) {
                consecutive++;
            } else if (cell == EMPTY) {
                open_ends++;
                break;
            } else {
                break;
            }
        }

        // --- 3. Attribution du score ---
        if (consecutive >= 5) total_score += WIN_SCORE;
        else total_score += SCORE_TABLE[consecutive][open_ends];
    }
    return total_score;
}

// Evaluate board reste le même, mais profitera de l'accélération
int evaluate_board(game *g, int player) {
    int opponent = (player == P1) ? P2 : P1;
    
    // Le score incrémental g->score est maintenu par apply_move
    // qui appelle get_point_score. Donc tout s'accélère.
    
    int score_diff = g->score[player] - g->score[opponent];
    
    // Facteur Captures (Très important pour votre variante)
    if (g->captures[player] >= 5) return WIN_SCORE;
    if (g->captures[opponent] >= 5) return -WIN_SCORE;
    
    score_diff += (g->captures[player] * 100000);
    score_diff -= (g->captures[opponent] * 100000);

    return score_diff;
}