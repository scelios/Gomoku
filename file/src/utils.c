# include "../include/gomoku.h"

bool isIaTurn(int iaTurn, int turn){return (iaTurn == turn);}

#include "../include/gomoku.h"

void resetGame(game *gameData, screen *windows)
{
    #ifdef DEBUG
        printf("--- GAME RESET ---\n");
    #endif

    // 1. Nettoyage mémoire brute (rapide et sûr)
    memset(gameData->board, EMPTY, sizeof(gameData->board));
    
    // 2. Remise à zéro des variables
    gameData->captures[P1] = 0;
    gameData->captures[P2] = 0;
    gameData->score[P1] = 0;
    gameData->score[P2] = 0;
    
    // 3. État du jeu
    gameData->turn = P1;           // P1 reprend la main
    gameData->game_over = false;   // Le jeu reprend
    
    // IMPORTANT : Si on veut que l'IA recommence en P2, on s'assure qu'elle est prête
    // On ne touche pas à iaTurn (le mode de jeu reste le même : Humain vs IA)
    
    // 4. Timer
    resetTimer(&gameData->ia_timer);

    // 5. Graphique
    // On signale que la fenêtre a changé pour que la gameLoop redessine tout
    // (Cadrillage + Bouton + Pions vides)
    windows->resized = true; 
    windows->changed = true;
    
    // Petit hack pour forcer le texte "Turn: P1" à s'afficher tout de suite
    printInformation(windows, gameData);
}