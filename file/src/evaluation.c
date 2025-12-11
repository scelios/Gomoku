// evaluation.c
// Implementation of countFormations and evaluateBoard for Gomoku
// Uses the project types and constants declared in gomoku.h

#include "../include/gomoku.h"
#include <string.h>

static inline bool in_bounds(int x, int y, int n) {
    return (x >= 0 && x < n && y >= 0 && y < n);
}

// Helper: check cell value with bounds
static inline int cell_at(const game *g, int x, int y)
{
    if (!in_bounds(x, y, g->board_size)) return -1; // Utilisation de in_bounds ici
    return g->board[y][x];
}

// Helper: detect "gapped" patterns (one gap inside window length 5)
// returns true if there is a pattern of 4 stones of 'player' within any length-5 window centered or starting
static bool has_gapped_four(const game *g, int player, int cx, int cy, int dx, int dy)
{
    // We'll scan windows of length 5 that include (cx,cy) in a stretch along direction (dx,dy)
    for (int offset = -4; offset <= 0; ++offset) {
        int player_count = 0;
        int opp_found = 0;
        for (int k = 0; k < 5; ++k) {
            int x = cx + (offset + k) * dx;
            int y = cy + (offset + k) * dy;
            int v = cell_at(g, x, y);
            if (v == player) player_count++;
            else if (v != 0) opp_found = 1;
        }
        if (!opp_found && player_count == 4) return true;
    }
    return false;
}

// countFormations: scans the board for sequences for 'player'
// Writes a formation score (sum of pattern weights) into *score_out and number of open threes into *open_threes
// Dans evaluation.c

void countFormations(const game *gameData, int player, int *score_out, int *open_threes, int min_x, int max_x, int min_y, int max_y)
{
    if (score_out) *score_out = 0;
    if (open_threes) *open_threes = 0;
    int n = gameData->board_size;
    int opponent = (player == 1) ? 2 : 1;

    // Scores locaux basés sur les constantes du .h
    // On utilise les constantes mises à jour !

    const int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};

    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            if (gameData->board[y][x] != player) continue;

            for (int d = 0; d < 4; ++d) {
                int dx = dirs[d][0], dy = dirs[d][1];
                
                // Vérifier début de séquence
                int px = x - dx, py = y - dy;
                if (in_bounds(px, py, n) && gameData->board[py][px] == player) continue;

                // Longueur
                int len = 0;
                int cx = x, cy = y;
                while (in_bounds(cx, cy, n) && gameData->board[cy][cx] == player) {
                    len++;
                    cx += dx; cy += dy;
                }

                // Extrémités
                int ax = x - dx, ay = y - dy; 
                int bx = x + len * dx, by = y + len * dy;

                // Analyse des bouts
                int blocked_ends = 0;
                
                // Check Avant
                int v_before = -1; // -1 = MUR
                if (in_bounds(ax, ay, n)) v_before = gameData->board[ay][ax];
                if (v_before != 0) blocked_ends++; 

                // Check Après
                int v_after = -1; // -1 = MUR
                if (in_bounds(bx, by, n)) v_after = gameData->board[by][bx];
                if (v_after != 0) blocked_ends++;

                if (blocked_ends == 2) continue; // Totalement mort

                // --- SCORING ---

                if (len >= 5) {
                    if (score_out) *score_out = F_FIVE;
                    return; 
                }
                else if (len == 4) {
                    if (blocked_ends == 0) { 
                        if (score_out) *score_out += F_FOUR_OPEN; 
                    }
                    else { 
                        // C'est ici que l'IA perdait au bord.
                        // Un 4 collé au mur (v_before == -1) est tout aussi mortel qu'un 4 libre.
                        if (score_out) *score_out += F_FOUR_HALF_OPEN; 
                    }
                }
                else if (len == 3) {
                    if (blocked_ends == 0) {
                        if (score_out) *score_out += F_THREE_OPEN;
                        if (open_threes) *open_threes += 1;
                    } 
                    else { 
                        // 3 collé au mur -> C'est un futur 4 mortel.
                        // On lui donne un score respectable.
                        if (score_out) *score_out += F_THREE_HALF_OPEN; 
                    }
                }
                else if (len == 2) {
                    // Gestion Paires / Captures (Le code précédent était bon ici)
                    if (blocked_ends == 0) {
                        if (score_out) *score_out += F_TWO_OPEN;
                    } 
                    else {
                        // Si bloqué par ENNEMI (pas mur), risque de capture
                        bool enemy_block = (v_before == opponent || v_after == opponent);
                        if (enemy_block) {
                             // Grosse pénalité (Peut être mangé)
                            if (score_out) *score_out -= 80000; 
                        }
                    }
                }
                
                // Patterns troués (Gapped)
                if (has_gapped_four(gameData, player, x, y, dx, dy)) {
                    // Un pattern troué type X X _ X X est aussi fort qu'un 4 Open
                    if (score_out) *score_out += F_FOUR_OPEN;
                }
            }
        }
    }
}

int evaluateBoard(game *gameData)
{
    // 1. Définition des Rôles (IA vs Humain)
    int ai_player = gameData->iaTurn; 
    int human_player = (ai_player == 1) ? 2 : 1;

    // 2. Victoires par Capture
    if (gameData->score[ai_player - 1] >= 10) return WINNING_SCORE;
    if (gameData->score[human_player - 1] >= 10) return MIN_SCORE;

    // 3. Bounding Box (inchangé)
    int min_x = 19, max_x = 0, min_y = 19, max_y = 0;
    bool empty = true;
    for (int y=0; y<19; y++) {
        for (int x=0; x<19; x++) {
            if (gameData->board[y][x] != 0) {
                if (x < min_x) min_x = x;
                if (x > max_x) max_x = x;
                if (y < min_y) min_y = y;
                if (y > max_y) max_y = y;
                empty = false;
            }
        }
    }
    if (empty) return 0;
    
    min_x = (min_x - 4 < 0) ? 0 : min_x - 4;
    max_x = (max_x + 4 > 18) ? 18 : max_x + 4;
    min_y = (min_y - 4 < 0) ? 0 : min_y - 4;
    max_y = (max_y + 4 > 18) ? 18 : max_y + 4;

    // 4. Calcul des Formations
    int score_ai = 0, threes_ai = 0;
    int score_human = 0, threes_human = 0;

    countFormations(gameData, ai_player, &score_ai, &threes_ai, min_x, max_x, min_y, max_y);
    countFormations(gameData, human_player, &score_human, &threes_human, min_x, max_x, min_y, max_y);

// --- 1. VICTOIRES IMMÉDIATES (PRIORITÉ ABSOLUE) ---
    if (score_ai >= F_FIVE) return WINNING_SCORE; // On a gagné
    if (score_human >= F_FIVE) return MIN_SCORE;  // On a perdu

    // --- 2. GESTION DE CRISE (DEFENSE FORCEE) ---
    
    // Si l'adversaire a un 4 ouvert, c'est perdu (sauf si on a gagné ligne 1)
    if (score_human >= F_FOUR_OPEN) return MIN_SCORE + 5000;

    // Si l'adversaire a un 4 fermé...
    if (score_human >= F_FOUR_HALF_OPEN) {
        // ... Et qu'on n'a pas nous-même une menace mortelle immédiate (4 ouvert ou fermé)
        // ALORS ON DOIT BLOQUER. Tout autre plan est un suicide.
        if (score_ai < F_FOUR_HALF_OPEN) {
            return MIN_SCORE + 10000; // Situation critique
        }
    }

    // Si l'adversaire a un 3 OUVERT (Le tueur silencieux)
    if (score_human >= F_THREE_OPEN) {
        // Si je n'ai pas au moins un 4 qui traîne pour contre-attaquer, je suis mort.
        if (score_ai < F_FOUR_HALF_OPEN) {
             return MIN_SCORE + 20000; // Je considère que j'ai déjà perdu
        }
    }

    // --- 3. CALCUL SCORE CLASSIQUE ---
    long final_score = 0;

    // On booste l'attaque, mais on SUR-BOOSTE la défense
    final_score += (long)score_ai; 
    
    // On multiplie la menace adverse par 2.0 (Au lieu de 1.2 précédemment)
    // L'IA aura deux fois plus peur de vos lignes qu'elle n'aura envie de faire les siennes.
    final_score -= (long)(score_human * 2.0); 

    // Double-trois
    if (threes_ai >= 2) final_score += 500000L;
    if (threes_human >= 2) final_score -= 1000000L; // Peur doublée

    // Captures (inchangé)
    int capture_diff = gameData->score[ai_player - 1] - gameData->score[human_player - 1];
    final_score += (long)capture_diff * 200000L;

    // Center Control (Faible poids, juste pour le début de partie)
    int center = gameData->board_size / 2;
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            int v = gameData->board[y][x];
            if (v == 0) continue;
            // Poids très faible (10) comparé aux Millions des lignes
            int val = (gameData->board_size - (abs(x - center) + abs(y - center)));
            
            if (v == ai_player) final_score += val * 10L;
            else if (v == human_player) final_score -= val * 10L;
        }
    }

    // Clamp pour rester dans les bornes d'un int
    if (final_score >= WINNING_SCORE) return WINNING_SCORE - 1;
    if (final_score <= MIN_SCORE) return MIN_SCORE + 1;

    return (int)final_score;
}