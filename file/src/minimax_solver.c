// minimax_solver.c
// Réécriture optimisée: make/undo in-place + move capping + heuristic local
#include "../include/gomoku.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Helper: applique un coup IN-PLACE et remplit moveRecord pour pouvoir undo
// - player: 1 ou 2 (convention du projet)
// - retourne true si posé, false si invalide (ex. case non vide)
bool make_move_inplace(game *g, int x, int y, int player, MoveRecord *rec)
{
    int n = g->board_size;
    if (!in_bounds(x, y, n)) return false;
    if (g->board[y][x] != 0) return false;

    // sauvegarde simple
    rec->x = x; rec->y = y;
    rec->prev_turn = g->turn;
    rec->prev_game_over = g->game_over;
    rec->prev_score[0] = g->score[0];
    rec->prev_score[1] = g->score[1];
    rec->captured_count = 0;

    // poser la pierre
    g->board[y][x] = player;
    g->turn = (player == 1) ? 2 : 1;

    // checkPieceCapture modifie g->board et g->score
    // on doit détecter quels pions ont été retirés pour l'undo
    // Pour compatibilité, on va appeler checkPieceCapture mais nous ne savons pas
    // s'il fournit la liste; donc on effectue nous-même un scan des captures
    // Pattern to capture: player - opp - opp - player (on a posé player en x,y)
    int dirs[4][2] = {{1,0},{0,1},{1,1},{1,-1}};
    int opp = (player == 1) ? 2 : 1;

    // IMPORTANT: call the project's capture function if it returns captured coords (preferred)
    // Here we emulate captures detection and removal (safe generic method)
    for (int d = 0; d < 4; ++d) {
        int dx = dirs[d][0], dy = dirs[d][1];
        // check forward direction: x+dx, x+2dx, x+3dx
        int x1 = x + dx, y1 = y + dy;
        int x2 = x + 2*dx, y2 = y + 2*dy;
        int x3 = x + 3*dx, y3 = y + 3*dy;
        if (in_bounds(x3,y3,n) && in_bounds(x1,y1,n) && in_bounds(x2,y2,n)) {
            if (g->board[y1][x1] == opp && g->board[y2][x2] == opp && g->board[y3][x3] == player) {
                // capture pair (x1,y1),(x2,y2)
                if (rec->captured_count + 2 <= MAX_CAPTURED_STONES) {
                    rec->cap_x[rec->captured_count] = x1;
                    rec->cap_y[rec->captured_count] = y1;
                    rec->cap_value[rec->captured_count] = opp;
                    rec->captured_count++;
                    rec->cap_x[rec->captured_count] = x2;
                    rec->cap_y[rec->captured_count] = y2;
                    rec->cap_value[rec->captured_count] = opp;
                    rec->captured_count++;
                }
                // remove them from board
                g->board[y1][x1] = 0;
                g->board[y2][x2] = 0;
                // update capture counters if present (convention: g->score[<player-1>])
                // It's unclear in your struct whether score[] counts stones captured BY player or lost.
                // We follow previous code: gameData->score[player-1] increments for captures by that player.
                g->score[player - 1] += 2;
            }
        }
        // check backward direction: x-dx, x-2dx, x-3dx
        int rx1 = x - dx, ry1 = y - dy;
        int rx2 = x - 2*dx, ry2 = y - 2*dy;
        int rx3 = x - 3*dx, ry3 = y - 3*dy;
        if (in_bounds(rx3,ry3,n) && in_bounds(rx1,ry1,n) && in_bounds(rx2,ry2,n)) {
            if (g->board[ry1][rx1] == opp && g->board[ry2][rx2] == opp && g->board[ry3][rx3] == player) {
                if (rec->captured_count + 2 <= MAX_CAPTURED_STONES) {
                    rec->cap_x[rec->captured_count] = rx1;
                    rec->cap_y[rec->captured_count] = ry1;
                    rec->cap_value[rec->captured_count] = opp;
                    rec->captured_count++;
                    rec->cap_x[rec->captured_count] = rx2;
                    rec->cap_y[rec->captured_count] = ry2;
                    rec->cap_value[rec->captured_count] = opp;
                    rec->captured_count++;
                }
                g->board[ry1][rx1] = 0;
                g->board[ry2][rx2] = 0;
                g->score[player - 1] += 2;
            }
        }
    }

    // Si la capture ou alignement transforme l'état en game over, mettre à jour flag.
    // Appeler la fonction existante qui détecte la victoire (si elle existe)
    // sinon on laisse game_over inchangé et rely evaluateBoard to detect terminal states.
    // Ici on laisse g->game_over inchangé; evaluateBoard vérifiera 5-en-ligne et captures>=10.

    return true;
}

// Undo : restaure l'état précédent à partir de MoveRecord
void undo_move_inplace(game *g, MoveRecord *rec)
{
    // supprimer la pierre jouée
    g->board[rec->y][rec->x] = 0;

    // restaurer les pierres capturées
    for (int i = 0; i < rec->captured_count; ++i) {
        int cx = rec->cap_x[i];
        int cy = rec->cap_y[i];
        int val = rec->cap_value[i];
        if (in_bounds(cx, cy, g->board_size)) {
            g->board[cy][cx] = val;
        }
    }

    // restaurer les scores et turn and flags
    g->score[0] = rec->prev_score[0];
    g->score[1] = rec->prev_score[1];
    g->turn = rec->prev_turn;
    g->game_over = rec->prev_game_over;
}

// -----------------------------
// getValidMoves : génère position candidates autour des pierres
// -----------------------------
vector2 *getValidMoves(const game *gameData, int *num_moves)
{
    int n = gameData->board_size;
    bool used[50][50] = {false};
    vector2 *out = NULL;
    *num_moves = 0;

    for (int y = 0; y < n; ++y) {
        for (int x = 0; x < n; ++x) {
            if (gameData->board[y][x] != 0) {
                for (int dy = -SEARCH_RADIUS; dy <= SEARCH_RADIUS; ++dy) {
                    for (int dx = -SEARCH_RADIUS; dx <= SEARCH_RADIUS; ++dx) {
                        int nx = x + dx, ny = y + dy;
                        if (!in_bounds(nx, ny, n)) continue;
                        if (gameData->board[ny][nx] != 0) continue;
                        if (used[ny][nx]) continue;
                        used[ny][nx] = true;
                        // add
                        vector2 v = {nx, ny};
                        *num_moves += 1;
                        out = (vector2 *)realloc(out, (*num_moves) * sizeof(vector2));
                        if (!out) {
                            perror("realloc failed in getValidMoves");
                            exit(EXIT_FAILURE);
                        }
                        out[*num_moves - 1] = v;
                    }
                }
            }
        }
    }

    // plateau vide -> centre
    if (*num_moves == 0) {
        *num_moves = 1;
        out = (vector2 *)malloc(sizeof(vector2));
        out[0].x = n / 2;
        out[0].y = n / 2;
    }

    return out;
}

// -----------------------------
// getScoredMoves : score et limite à MAX_CANDIDATES
// - Pour chaque candidat : make_move_inplace -> countFormations (existant) -> undo
// - on garde seulement les MAX_CANDIDATES meilleurs
// -----------------------------
move *getScoredMoves(const game *gameDataConst, int *num_moves_out)
{
    // work on a mutable copy pointer since we'll apply moves in-place on a working copy
    // but we prefer to avoid an entire board copy; we will apply moves directly to the original
    // object provided by caller only if caller expects it — safer: create a shallow copy of board in stack and restore.
    //
    // Here we will create a *local mutable copy* of the game structure (only board array) to simulate moves,
    // because original gameDataConst must stay immutable for callers.
    game work = *gameDataConst; // shallow copy of struct (board array will be copied if contained by value)
    // NOTE: this assumes 'board' is embedded array in game struct (as in your earlier code).
    // If board is pointer inside game, you must use a proper deep copy. We assume embedding.

    int n_valid = 0;
    vector2 *valid = getValidMoves(gameDataConst, &n_valid);
    if (n_valid == 0) {
        *num_moves_out = 0;
        return NULL;
    }

    // temporary array of scored moves (capacity n_valid)
    move *scored_all = (move *)malloc(n_valid * sizeof(move));
    if (!scored_all) {
        perror("malloc failed in getScoredMoves");
        free(valid);
        *num_moves_out = 0;
        return NULL;
    }

    int current_player = work.turn;
    int opponent = (current_player == 1) ? 2 : 1;

    for (int i = 0; i < n_valid; ++i) {
        vector2 pos = valid[i];

        // Apply move on work game using move record
        MoveRecord rec;
        memset(&rec, 0, sizeof(rec));
        // Make move in place on work
        bool ok = make_move_inplace(&work, pos.x, pos.y, current_player, &rec);
        if (!ok) {
            // should not happen since valid moves are empty; skip
            scored_all[i].position = pos;
            scored_all[i].score = MIN_SCORE;
            continue;
        }

        // Evaluate locally: count formations for both players
        int score_p = 0, threes_p = 0;
        int score_o = 0, threes_o = 0;
        countFormations(&work, current_player, &score_p, &threes_p);
        countFormations(&work, opponent, &score_o, &threes_o);

        // priority: favor immediate win / blocks / captures
        int priority = 0;

        if (score_p >= F_FIVE || work.score[current_player - 1] >= 10) priority += 10000000;
        if (score_o >= F_FIVE || work.score[opponent - 1] >= 10) priority -= 10000000;

        // boost for open fours and threes (these constants assumed defined)
        priority += score_p;
        priority -= score_o;

        // boosts for captures made by this move
        int capture_gain = work.score[current_player - 1] - gameDataConst->score[current_player - 1];
        priority += capture_gain * 20000;

        // double-three extra
        if (threes_p >= 2) priority += 50000;
        if (threes_o >= 2) priority -= 50000;

        scored_all[i].position = pos;
        scored_all[i].score = priority;

        // undo move on work
        undo_move_inplace(&work, &rec);
    }

    free(valid);

    // Sort all candidates by score descending
    qsort(scored_all, n_valid, sizeof(move), compareMovesPriority);

    // Keep only top MAX_CANDIDATES (but at least 1)
    int keep = n_valid;
    if (keep > MAX_CANDIDATES) keep = MAX_CANDIDATES;
    if (keep < 1) keep = 1;

    move *result = (move *)malloc(keep * sizeof(move));
    if (!result) {
        perror("malloc failed in getScoredMoves (result)");
        free(scored_all);
        *num_moves_out = 0;
        return NULL;
    }
    for (int i = 0; i < keep; ++i) result[i] = scored_all[i];

    free(scored_all);
    *num_moves_out = keep;
    return result;
}

// -----------------------------
// compareMovesPriority : déjà défini mais on redéfinit guard\n
// -----------------------------
int compareMovesPriority(const void *a, const void *b)
{
    const move *ma = (const move *)a;
    const move *mb = (const move *)b;
    // protection overflow: returns sign value
    if (ma->score < mb->score) return 1;
    if (ma->score > mb->score) return -1;
    return 0;
}

// -----------------------------
// Minimax with alpha-beta (inplace moves)
// - uses make_move_inplace / undo_move_inplace to avoid copies
// - uses limited move list via getScoredMoves
// -----------------------------
int minimax(game *gameData, int depth, int alpha, int beta, bool maximizingPlayer)
{
    // terminal checks
    if (depth <= 0 || gameData->game_over ||
        gameData->score[0] >= 10 || gameData->score[1] >= 10)
    {
        int eval = evaluateBoard(gameData);
        if (eval >= WINNING_SCORE) return WINNING_SCORE + depth;
        if (eval <= MIN_SCORE) return MIN_SCORE - depth;
        return eval;
    }

    int num_moves = 0;
    move *moves = getScoredMoves(gameData, &num_moves);
    if (!moves || num_moves == 0) {
        if (moves) free(moves);
        return evaluateBoard(gameData);
    }

    int best_eval = maximizingPlayer ? MIN_SCORE : WINNING_SCORE;
    int player = gameData->turn;

    for (int i = 0; i < num_moves; ++i) {
        vector2 pos = moves[i].position;

        MoveRecord rec;
        memset(&rec, 0, sizeof(rec));

        // play move in-place
        bool ok = make_move_inplace(gameData, pos.x, pos.y, player, &rec);
        if (!ok) continue; // shouldn't happen but safe

        int val = minimax(gameData, depth - 1, alpha, beta, !maximizingPlayer);

        // undo
        undo_move_inplace(gameData, &rec);

        if (maximizingPlayer) {
            if (val > best_eval) best_eval = val;
            if (best_eval > alpha) alpha = best_eval;
        } else {
            if (val < best_eval) best_eval = val;
            if (best_eval < beta) beta = best_eval;
        }

        if (beta <= alpha) break;
    }

    free(moves);
    return best_eval;
}

// -----------------------------
// findBestMove: uses inplace moves and getScoredMoves
// -----------------------------
move findBestMove(game *gameData, int depth)
{
    move best_move;
    best_move.position.x = -1;
    best_move.position.y = -1;
    best_move.score = MIN_SCORE;

    int num_moves = 0;
    move *moves = getScoredMoves(gameData, &num_moves);
    if (!moves || num_moves == 0) {
        if (moves) free(moves);
        return best_move;
    }

    int alpha = MIN_SCORE;
    int beta = WINNING_SCORE;
    int player = gameData->turn;

    for (int i = 0; i < num_moves; ++i) {
        vector2 pos = moves[i].position;
        MoveRecord rec;
        memset(&rec, 0, sizeof(rec));
        if (!make_move_inplace(gameData, pos.x, pos.y, player, &rec)) continue;

        int val = minimax(gameData, depth - 1, alpha, beta, false);

        undo_move_inplace(gameData, &rec);

        if (val > best_move.score) {
            best_move.score = val;
            best_move.position = pos;
        }

        if (best_move.score > alpha) alpha = best_move.score;
        if (beta <= alpha) break;
    }

    free(moves);
    return best_move;
}
