#include "../include/gomoku.h"

// Helper : Vérifie les limites
static inline bool is_valid(int x, int y) {
    return (x >= 0 && x < 19 && y >= 0 && y < 19);
}

// Vérifie si une pierre située en (x, y) est "prenable" (fait partie d'une paire capturable)
// Cette fonction regarde dans les 8 directions si la pierre est impliquée dans un schéma :
// [ENNEMI] [ALLIÉ(x,y)] [ALLIÉ] [VIDE]  (Menace de capture sur le vide)
// ou [VIDE] [ALLIÉ] [ALLIÉ(x,y)] [ENNEMI]
bool isStoneCapturable(int board[19][19], int x, int y, int player)
{
    int opponent = (player == 1) ? 2 : 1;
    
    // Les 4 axes (Horizontal, Vertical, Diag1, Diag2)
    // On vérifie les deux sens pour chaque axe, donc 8 directions implicites
    int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    for (int d = 0; d < 4; d++)
    {
        int dx = dirs[d][0];
        int dy = dirs[d][1];

        // On regarde autour de la pierre (x,y) sur l'axe donné
        // On cherche le pattern : O A A _  ou  _ A A O
        // La pierre (x,y) peut être le premier A ou le deuxième A.
        
        // Positions relatives : P_prev2, P_prev1, [P_curr], P_next1, P_next2
        
        // Check forward : (x,y) est le premier de la paire ? [x,y] [Ally]
        int nx = x + dx, ny = y + dy;       // Next (Ally?)
        int nnx = x + 2*dx, nny = y + 2*dy; // NextNext (Empty/Enemy?)
        int px = x - dx, py = y - dy;       // Prev (Empty/Enemy?)

        if (is_valid(nx, ny) && board[ny][nx] == player) 
        {
            // On a une paire : [x,y] - [nx,ny]
            // Cas 1 : Ennemi derrière, Vide devant => [Opp] [P] [P] [_]
            if (is_valid(px, py) && board[py][px] == opponent &&
                is_valid(nnx, nny) && board[nny][nnx] == 0) return true;
                
            // Cas 2 : Vide derrière, Ennemi devant => [_] [P] [P] [Opp]
            if (is_valid(px, py) && board[py][px] == 0 &&
                is_valid(nnx, nny) && board[nny][nnx] == opponent) return true;
        }

        // Check backward : (x,y) est le deuxième de la paire ? [Ally] [x,y]
        // C'est redondant si on scanne tout le plateau, mais nécessaire si on scanne juste une ligne
        int b_px = x - dx, b_py = y - dy;       // Prev (Ally?)
        int b_ppx = x - 2*dx, b_ppy = y - 2*dy; // PrevPrev (Empty/Enemy?)
        int b_nx = x + dx, b_ny = y + dy;       // Next (Empty/Enemy?)

        if (is_valid(b_px, b_py) && board[b_py][b_px] == player)
        {
            // On a une paire : [b_px,b_py] - [x,y]
            // Cas 1 : Ennemi derrière la paire, Vide devant
            if (is_valid(b_ppx, b_ppy) && board[b_ppy][b_ppx] == opponent &&
                is_valid(b_nx, b_ny) && board[b_ny][b_nx] == 0) return true;

            // Cas 2 : Vide derrière, Ennemi devant
            if (is_valid(b_ppx, b_ppy) && board[b_ppy][b_ppx] == 0 &&
                is_valid(b_nx, b_ny) && board[b_ny][b_nx] == opponent) return true;
        }
    }
    return false;
}

// Vérifie si une ligne gagnante (5+) contient une pierre capturable
// Si oui, la victoire est annulée (temporairement).
bool isWinningLineBreakable(int board[19][19], int start_x, int start_y, int dx, int dy, int length, int player)
{
    // On parcourt chaque pierre de l'alignement
    for (int i = 0; i < length; i++)
    {
        int cx = start_x + i * dx;
        int cy = start_y + i * dy;

        // Si UNE SEULE pierre de la ligne peut être capturée, la ligne est "cassable"
        if (isStoneCapturable(board, cx, cy, player))
        {
            // printf("Winning line interrupted: stone at (%d, %d) is capturable!\n", cx, cy);
            return true; // Breakable
        }
    }
    return false; // Unbreakable
}

void checkVictoryCondition(game *gameData, screen *windows)
{
    // 1. Victoire par Capture (10 pierres = 5 paires)
    // Note : score stocke le nombre de paires
    if (gameData->score[0] >= 5 || gameData->score[1] >= 5)
    {
        printf("Victory by capture (10 stones)!\n");
        gameData->game_over = true;
        return;
    }

    // 2. Victoire par Alignement (5+)
    int player = gameData->turn;
    int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};
    
    // Matrice pour éviter de recompter les mêmes lignes (optimisation)
    // On ne scanne que dans le sens "positif"
    
    for (int y = 0; y < 19; y++)
    {
        for (int x = 0; x < 19; x++)
        {
            if (gameData->board[y][x] != player) continue;

            for (int d = 0; d < 4; d++)
            {
                int dx = dirs[d][0];
                int dy = dirs[d][1];

                // Optimisation: si la pierre précédente était du même joueur, 
                // alors cette pierre a déjà été comptée dans la boucle précédente.
                int px = x - dx, py = y - dy;
                if (is_valid(px, py) && gameData->board[py][px] == player) continue;

                // On compte la longueur de la ligne à partir d'ici
                int length = 0;
                int tx = x, ty = y;
                while (is_valid(tx, ty) && gameData->board[ty][tx] == player)
                {
                    length++;
                    tx += dx;
                    ty += dy;
                }

                if (length >= 5)
                {
                    // Ligne de 5 trouvée !
                    // Est-elle cassable ? (Contient-elle une pierre capturable ?)
                    if (!isWinningLineBreakable(gameData->board, x, y, dx, dy, length, player))
                    {
                        // Si elle n'est PAS cassable, c'est gagné
                        printf("Victory by alignment! (%d in a row)\n", length);
                        gameData->game_over = true;
                        
                        // Optionnel : Dessiner ou marquer la ligne gagnante ici
                        return;
                    }
                    else
                    {
                        // printf("Alignment of %d found but it can be broken by capture.\n", length);
                        // On continue la partie, l'adversaire DOIT capturer au prochain tour sinon il perd.
                    }
                }
            }
        }
    }
}