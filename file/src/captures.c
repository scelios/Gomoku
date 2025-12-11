#include "../include/gomoku.h"

// Helper simple et inline pour la performance
static inline bool is_valid(int x, int y, int n) {
    return x >= 0 && x < n && y >= 0 && y < n;
}

bool captureParamsValid(game *gameData, screen *windows, int lx, int ly)
{
    if (!gameData) return false;
    int n = gameData->board_size;
    if (!is_valid(lx, ly, n)) return false;
    
    // On vérifie que le coup joué est bien un pion (1 ou 2)
    int owner = gameData->board[ly][lx];
    if (owner != 1 && owner != 2) return false;
    
    return true;
}

// Cette fonction fait tout : détection + suppression + mise à jour score + affichage
// Elle retourne le nombre de PAIRES capturées (0, 1, 2...)
int processCaptures(game *gameData, screen *windows, int lx, int ly)
{
    int n = gameData->board_size;
    int owner = gameData->board[ly][lx];
    int opponent = (owner == 1) ? 2 : 1;
    int pairs_captured = 0;

    // Les 8 directions
    const int dirs[8][2] = {
        { 1, 0}, { 0, 1}, {-1, 0}, { 0,-1},
        { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1}
    };

    for (int d = 0; d < 8; d++)
    {
        int dx = dirs[d][0];
        int dy = dirs[d][1];

        // Coordonnées : P1(Opp) - P2(Opp) - P3(Owner)
        int x1 = lx + dx;     int y1 = ly + dy;
        int x2 = lx + 2*dx;   int y2 = ly + 2*dy;
        int x3 = lx + 3*dx;   int y3 = ly + 3*dy;

        // Optimisation : On vérifie d'abord si x3 est dans les bornes (le plus loin)
        if (!is_valid(x3, y3, n)) continue;

        // Lecture directe des valeurs
        int p1 = gameData->board[y1][x1];
        int p2 = gameData->board[y2][x2];
        int p3 = gameData->board[y3][x3];

        // LOGIQUE DE CAPTURE STRICTE : X O O X
        // On vérifie p1 != 0 et p2 != 0 pour éviter de capturer du vide
        if (p1 == opponent && p2 == opponent && p3 == owner)
        {
            // 1. Suppression logique
            gameData->board[y1][x1] = 0;
            gameData->board[y2][x2] = 0;

            // 2. Mise à jour graphique immédiate (plus de double boucle)
            if (windows) {
                drawSquare(windows, x1, y1, 0); // Dessiner vide
                drawSquare(windows, x2, y2, 0); // Dessiner vide
            }

            // 3. Compter
            pairs_captured++;
            
            // Debug optionnel
            // printf("Capture: Removed pair at (%d,%d) and (%d,%d)\n", x1, y1, x2, y2);
        }
    }
    return pairs_captured;
}

void checkPieceCapture(game *gameData, screen *windows, int lx, int ly)
{
    if (!captureParamsValid(gameData, windows, lx, ly)) return;

    int pairs = processCaptures(gameData, windows, lx, ly);

    if (pairs > 0)
    {
        // Mise à jour du score
        gameData->score[gameData->turn - 1] += pairs; // turn est 1 ou 2
        
        // Signal de rafraîchissement
        if (windows) windows->changed = true;
    }
}