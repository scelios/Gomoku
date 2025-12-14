#include "../include/gomoku.h"

bool checkArgs(int argc, char **argv, void**args)
{
    if (argc == 1)
        *args = NULL;
    else
    {
        (void)argv;
        *args = NULL;
    }
    return true;
}

bool initialized(void *args, screen *windows, game *gameData)
{
    (void) args;

    // Screen Init
    windows->width = WIDTH;
    windows->height = HEIGHT;
    windows->moved = false;
    windows->resized = true;
    windows->isClicked = false;
    windows->changed = true;
    windows->board_size = BOARD_SIZE;
    windows->text_img = NULL;

    // Game Init
    gameData->board_size = BOARD_SIZE;

    // OPTIMISATION: memset met tout le tableau à 0 en une seule instruction CPU
    memset(gameData->board, EMPTY, sizeof(gameData->board));
    
    // Init captures
    gameData->captures[P1] = 0;
    gameData->captures[P2] = 0;

    gameData->iaTurn = 0; // 0 = Pas d'IA, sinon 1 ou 2
    gameData->turn = P1;  // P1 commence toujours
    gameData->game_over = false;

    // Timer Init
    gameData->ia_timer.elapsed = 0.0;
    gameData->ia_timer.running = false;
    gameData->ia_timer.start_ts.tv_sec = 0;
    gameData->ia_timer.start_ts.tv_nsec = 0;

    gameData->score[0] = 0; // Unused
    gameData->score[P1] = 0;
    gameData->score[P2] = 0;

    return true;
}

void putPiecesOnBoard(screen *windows, int *board)
{
    // On parcourt le tableau 1D comme une matrice pour l'affichage
    for (int y = 0; y < windows->board_size; y++)
    {
        for (int x = 0; x < windows->board_size; x++)
        {
            // Utilisation de la Macro pour récupérer la valeur
            int val = board[GET_INDEX(x, y)];
            
            if (val == P1 || val == P2)
                drawSquare(windows, x, y, val);
            else if (val == PREVIS)
                 drawSquare(windows, x, y, val); // Si tu as une couleur de prévis
        }
    }
}

void resetScreen(screen *windows, int *board)
{
    printBlack(windows);
    putCadrillage(windows);
    putPiecesOnBoard(windows, board);
}

// C'est ici que nous connecterons la Gemme #4 (Minimax) plus tard
void makeIaMove(game *gameData, screen *windows)
{
    printf("IA réfléchit...\n");
    // TODO: Connecter l'algo Alpha-Beta ici
    
    // Exemple temporaire pour ne pas bloquer: joue au hasard
    /*
    for (int i=0; i<MAX_BOARD; i++) {
        if (gameData->board[i] == EMPTY) {
            gameData->board[i] = gameData->iaTurn;
            checkPieceCapture(gameData, windows, GET_X(i), GET_Y(i));
            break;
        }
    }
    */
    (void)windows;
}

void gameLoop(void *param)
{
    both        *args = (struct both *)param;
    screen      *windows = args->windows;
    game        *gameData = args->gameData;

    if (windows->changed)
    {
        if (windows->resized)
        {
            windows->resized = false;
            resetScreen(windows, gameData->board);
        }

        // Gestion du tour IA
        if (isIaTurn(gameData->iaTurn, gameData->turn) && !gameData->game_over)
        {
            makeIaMove(gameData, windows);
            
            // Après le coup de l'IA, on change de tour et on redraw
            checkVictoryCondition(gameData, windows);
            gameData->turn = (gameData->turn == P1) ? P2 : P1;
            windows->changed = true; // Forcer le redraw après coup IA
        }
        else
        {
            resetTimer(&gameData->ia_timer);
        }

        checkVictoryCondition(gameData, windows);
        // Note: Le changement de tour Humain se fait généralement dans le mousehook
        windows->changed = false;
    }

    printInformation(windows, gameData);
}

void launchGame(game *gameData, screen *windows)
{
    both args;
    args.windows = windows;
    args.gameData = gameData;

    // Init MLX
    windows->mlx = mlx_init((int32_t)windows->width, (int32_t)windows->height, "Gomoku IA", true);
    if (!windows->mlx)
    {
        perror("mlx_init failed");
        exit(EXIT_FAILURE);
    }

    windows->img = mlx_new_image(windows->mlx, windows->width, windows->height);
    if (mlx_image_to_window(windows->mlx, windows->img, 0, 0) == -1)
    {
        mlx_close_window(windows->mlx);
        puts(mlx_strerror(mlx_errno));
        exit(EXIT_FAILURE);
    }

    // Hooks
    mlx_resize_hook(windows->mlx, &resize, windows);
    mlx_loop_hook(windows->mlx, &gameLoop, &args);
    mlx_cursor_hook(windows->mlx, &cursor, windows);
    mlx_mouse_hook(windows->mlx, &mousehook, &args);
    mlx_key_hook(windows->mlx, &keyhook, &args);

    mlx_loop(windows->mlx);
    mlx_terminate(windows->mlx);
}

int main(int argc, char **argv)
{
    void *args;
    screen windows;
    game gameData;

    if (!checkArgs(argc, argv, &args))
        return (EXIT_FAILURE);

    if (!initialized(args, &windows, &gameData))
        return (EXIT_FAILURE);

    launchGame(&gameData, &windows);

    return (EXIT_SUCCESS);
}