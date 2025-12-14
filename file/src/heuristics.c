#include "../include/gomoku.h"

/*
    Analyse une ligne (séquence de pions) pour un joueur donné.
    consecutive: nombre de pions d'affilée
    open_ends: nombre de côtés libres (0, 1 ou 2)
    current_turn: permet de savoir si c'est à nous de jouer (bonus d'initiative)
*/
int get_sequence_score(int consecutive, int open_ends, int captures_count)
{
    // 1. Victoire par alignement
    if (consecutive >= 5)
        return WIN_SCORE;

    // 2. Victoire par capture (si ce coup nous amène à 10 captures ou plus)
    // Note: 1 capture = 2 pions. Donc 5 paires = 10 pions.
    // gameData->captures stocke le nombre de PAIRES (selon ton code précédent) ou de PIONS.
    // Vérifions : dans captures.c, tu fais capturesByPlayer += 1 (paire).
    // Donc victoire si captures >= 5.
    if (captures_count >= 5) 
        return WIN_SCORE;

    if (consecutive == 4)
    {
        if (open_ends == 2) return OPEN_FOUR;
        if (open_ends == 1) return CLOSED_FOUR;
    }
    
    if (consecutive == 3)
    {
        if (open_ends == 2) return OPEN_THREE;
        if (open_ends == 1) return CLOSED_THREE;
    }

    if (consecutive == 2)
    {
        if (open_ends == 2) return OPEN_TWO;
    }

    return 0;
}

/* Scanne une direction spécifique (dx, dy) pour tout le plateau
   et accumule le score pour le joueur 'player'.
   C'est la partie la plus critique en performance.
*/
int scan_direction(game *g, int player, int dx, int dy)
{
    int score = 0;
    int opponent = (player == P1) ? P2 : P1;

    // Pour éviter de parcourir les bords inutiles, on définit les limites de boucle
    // selon la direction. Simplification: on parcourt tout, le check de bornes gère.
    
    // On parcourt chaque ligne/colonne/diagonale possible
    // Cette boucle est générique mais peut être optimisée
    for (int y = 0; y < BOARD_SIZE; y++)
    {
        for (int x = 0; x < BOARD_SIZE; x++)
        {
            // On ne démarre une analyse que si la case précédente n'était pas la même
            // pour éviter de compter "X X X" deux fois (une fois à index 0, une fois à index 1)
            // C'est une optimisation de saut.
            
            int idx = GET_INDEX(x, y);
            if (g->board[idx] != player) continue;

            // Vérifier si c'est le début d'une séquence dans la direction (dx, dy)
            int prev_x = x - dx;
            int prev_y = y - dy;
            
            // Si la case d'avant est aussi à nous, on a déjà compté cette séquence
            if (IS_VALID(prev_x, prev_y) && g->board[GET_INDEX(prev_x, prev_y)] == player)
                continue;

            // --- Début d'une nouvelle séquence ---
            int consecutive = 0;
            int open_ends = 0;
            
            // 1. Check arrière (Blocked ou Open ?)
            if (IS_VALID(prev_x, prev_y) && g->board[GET_INDEX(prev_x, prev_y)] == EMPTY)
                open_ends++;

            // 2. Compter la longueur de la séquence
            int cur_x = x;
            int cur_y = y;
            while (IS_VALID(cur_x, cur_y) && g->board[GET_INDEX(cur_x, cur_y)] == player)
            {
                consecutive++;
                cur_x += dx;
                cur_y += dy;
            }

            // 3. Check avant (Blocked ou Open ?)
            // cur_x, cur_y est maintenant sur la première case NON-player
            if (IS_VALID(cur_x, cur_y) && g->board[GET_INDEX(cur_x, cur_y)] == EMPTY)
                open_ends++;

            // Ajouter le score de cette séquence
            score += get_sequence_score(consecutive, open_ends, g->captures[player]);
        }
    }
    return score;
}

/*
    FONCTION PRINCIPALE D'ÉVALUATION
    Retourne un score positif si 'player' mène, négatif si 'player' perd.
*/
int evaluate_board(game *g, int player)
{
    int opponent = (player == P1) ? P2 : P1;
    int my_score = 0;
    int op_score = 0;

    // 1. Score des Captures (Facteur très lourd)
    // On multiplie le nombre de paires capturées par un gros score
    my_score += g->captures[player] * CAPTURE_SCORE;
    op_score += g->captures[opponent] * CAPTURE_SCORE;

    // Si un joueur a gagné par capture, on renvoie direct l'infini
    if (g->captures[player] >= 5) return WIN_SCORE;
    if (g->captures[opponent] >= 5) return -WIN_SCORE;

    // 2. Score Positionnel (Alignements)
    // Directions: Horiz, Vert, Diag1, Diag2
    int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (int i = 0; i < 4; i++)
    {
        my_score += scan_direction(g, player, dirs[i][0], dirs[i][1]);
        op_score += scan_direction(g, opponent, dirs[i][0], dirs[i][1]);
    }

    // Le score final est relatif : Mon Avantage - Avantage Adversaire
    return (my_score - op_score);
}