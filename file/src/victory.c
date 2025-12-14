#include "../include/gomoku.h"

// Offsets pour les 8 directions sur un tableau 1D (pour Board Size 19)
// N, S, W, E, NW, NE, SW, SE
static const int DIRECTIONS[8] = {
    -BOARD_SIZE, BOARD_SIZE, -1, 1, 
    -BOARD_SIZE - 1, -BOARD_SIZE + 1, BOARD_SIZE - 1, BOARD_SIZE + 1
};

/**
 * Vérifie si le coup joué à l'index `idx` capture des pions adverses.
 * Pattern de capture : [AMI] [ENNEMI] [ENNEMI] [AMI]
 * Retourne le nombre de paires capturées.
 */
int checkCaptures(game *gameData, int idx, int player)
{
    int opponent = (player == P1) ? P2 : P1;
    int captures_count = 0;
    int *board = gameData->board;
    int x = GET_X(idx);
    int y = GET_Y(idx);

    for (int d = 0; d < 8; d++)
    {
        int dir = DIRECTIONS[d];
        
        // Coordonnées relatives pour vérifier les débordements de ligne
        // dx/dy nous servent à vérifier qu'on ne "warp" pas de l'autre côté du plateau
        int dx = 0, dy = 0;
        if (dir == 1 || dir == -BOARD_SIZE + 1 || dir == BOARD_SIZE + 1) dx = 1;
        if (dir == -1 || dir == -BOARD_SIZE - 1 || dir == BOARD_SIZE - 1) dx = -1;
        if (dir == BOARD_SIZE || dir == BOARD_SIZE - 1 || dir == BOARD_SIZE + 1) dy = 1;
        if (dir == -BOARD_SIZE || dir == -BOARD_SIZE - 1 || dir == -BOARD_SIZE + 1) dy = -1;

        // Index des 3 cases suivantes : [idx] [1] [2] [3]
        int idx1 = idx + dir;
        int idx2 = idx + 2 * dir;
        int idx3 = idx + 3 * dir;

        // Vérification des bords du plateau (IS_VALID est dans le .h)
        // On vérifie les coord absolues de la case "destination" (l'allié qui ferme la pince)
        int dest_x = x + (dx * 3);
        int dest_y = y + (dy * 3);

        if (!IS_VALID(dest_x, dest_y)) continue;

        // La logique de capture : X O O X
        if (board[idx1] == opponent && 
            board[idx2] == opponent && 
            board[idx3] == player)
        {
            // CAPTURE !
            board[idx1] = EMPTY; // Retirer le pion
            board[idx2] = EMPTY; // Retirer le pion
            captures_count++;
            
            // Effet graphique (optionnel, pour debug)
            // printf("Capture réalisée direction %d !\n", d);
        }
    }
    return captures_count;
}

/**
 * Vérifie si le joueur a aligné 5 pions (ou plus) passant par `idx`.
 * Regarde les 4 axes (Horizontal, Vertical, Diag1, Diag2).
 */
bool checkFiveInARow(int *board, int idx, int player)
{
    int x = GET_X(idx);
    int y = GET_Y(idx);

    // Les 4 axes de vérification (offsets positifs)
    // Horizontal(1), Vertical(BOARD_SIZE), Diag1(BOARD_SIZE+1), Diag2(BOARD_SIZE-1)
    int axes[4] = {1, BOARD_SIZE, BOARD_SIZE + 1, BOARD_SIZE - 1};

    for (int a = 0; a < 4; a++)
    {
        int offset = axes[a];
        int count = 1; // On compte le pion qu'on vient de poser

        // 1. Scanner dans la direction POSITIVE
        for (int i = 1; i < 5; i++)
        {
            int next_idx = idx + (i * offset);
            int nx = GET_X(next_idx);
            int ny = GET_Y(next_idx);

            // Vérifier validité (bords) et continuité visuelle
            // Astuce 1D : Si on vérifie horizontalement, y doit rester le même.
            if (!IS_VALID(nx, ny) || board[next_idx] != player) break;
            if (a == 0 && ny != y) break; // Sécurité anti-wrap horizontal

            count++;
        }

        // 2. Scanner dans la direction NEGATIVE
        for (int i = 1; i < 5; i++)
        {
            int prev_idx = idx - (i * offset);
            int px = GET_X(prev_idx);
            int py = GET_Y(prev_idx);

            if (!IS_VALID(px, py) || board[prev_idx] != player) break;
            if (a == 0 && py != y) break; 

            count++;
        }

        if (count >= 5) return true;
    }
    return false;
}

/**
 * Fonction principale appelée après chaque clic ou coup d'IA
 */
void checkVictoryCondition(game *gameData, screen *windows)
{
    // On ne vérifie rien si le jeu est fini
    if (gameData->game_over) return;

    // Trouver le dernier coup joué n'est pas stocké explicitement dans gameData actuellement
    // Pour l'optimisation, il faudrait passer le dernier coup en argument.
    // MAIS, pour l'instant, faisons une fonction qui scanne (moins performant mais compatible avec votre code actuel)
    // IDEALEMENT : Modifiez le prototype pour : void checkVictoryCondition(game *gameData, int last_move_idx)
    
    // --- TEMPORAIRE : Recherche du dernier coup (À optimiser plus tard dans votre boucle de jeu) ---
    // Pour l'instant, on va scanner tout le plateau uniquement pour l'exemple
    // car votre code actuel ne passe pas l'index du dernier coup.
    // C'est ici que l'architecture doit évoluer.
    
    // NOTE : Je laisse la logique "globale" pour que ça compile avec votre main.c actuel,
    // mais l'IA utilisera check_five_in_a_row directement.
    
    // Vérification score (Captures)
    if (gameData->captures[gameData->turn] >= MAX_CAPTURES) // 5 paires = 10 pions
    {
        printf("Victoire par capture pour le joueur %d !\n", gameData->turn);
        gameData->game_over = true;
        return;
    }

    // Vérification alignement (Naïf pour l'affichage, Optimisé pour l'IA)
    // On parcourt tout le tableau pour trouver une victoire (LENT mais sûr pour l'interface graphique)
    for (int i = 0; i < MAX_BOARD; i++)
    {
        if (gameData->board[i] == gameData->turn)
        {
            if (checkFiveInARow(gameData->board, i, gameData->turn))
            {
                printf("Victoire par alignement pour le joueur %d !\n", gameData->turn);
                gameData->game_over = true;
                return;
            }
        }
    }
}

/**
 * Fonction helper pour l'IA et le gameplay : Joue un coup, gère captures et victoire
 * À utiliser dans votre boucle de jeu principale !
 */
void playMoveAndCheck(game *gameData, int idx, screen *windows)
{
    int player = gameData->turn;
    
    // 1. Poser le pion
    gameData->board[idx] = player;

    // 2. Gérer les captures (Règle : on capture APRES avoir posé)
    int pairs_captured = checkCaptures(gameData, idx, player);
    gameData->captures[player] += (pairs_captured * 2); // *2 car on compte les pions

    // 3. Vérifier victoire immédiate
    if (gameData->captures[player] >= MAX_CAPTURES) {
        gameData->game_over = true;
        printf("WIN par Captures!\n");
    }
    else if (checkFiveInARow(gameData->board, idx, player)) {
        gameData->game_over = true;
        printf("WIN par Alignement!\n");
    }
}