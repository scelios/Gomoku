# include "../include/gomoku.h"

bool isIaTurn(int iaTurn, int turn){return (iaTurn == turn);}

game copyGameData(const game *source)
{
    game dest;

    // 1. Copier les champs simples
    dest.board_size = source->board_size;
    dest.turn = source->turn;
    dest.iaTurn = source->iaTurn;
    dest.game_over = source->game_over;
    
    // 2. Copier les structures imbriquées (si nécessaire, ici le timer peut être ignoré car ce n'est qu'un état temporaire de l'IA)
    // On copie le score, car c'est crucial pour l'heuristique
    dest.score[0] = source->score[0];
    dest.score[1] = source->score[1];
    
    // 3. Copie du plateau (Deep Copy du tableau statique)
    // Nous copions tout le tableau 19x19, même s'il n'est utilisé qu'en 19x19
    for (int i = 0; i < 19; i++)
    {
        for (int j = 0; j < 19; j++)
        {
            dest.board[i][j] = source->board[i][j];
        }
    }
    
    // Les champs timer peuvent être ignorés pour la simulation Minimax
    dest.ia_timer.running = false;
    dest.ia_timer.elapsed = 0.0; 

    return dest;
}