# include "../include/gomoku.h"

bool checkArgs(int argc, char **argv, void**args)
{
    if (argc == 1)
    {
        *args = NULL; // default mode
    }
    else
    {
        //! TODO: parse arguments here for different game modes
        // for now, we just set args to NULL
        (void)argv;
        *args = NULL;
    }
    return true;
}

bool initialized(void *args, screen *windows, game *gameData)
{
    (void) args; // currently unused

    initZobrist();

    // Initialize game board to empty
    windows->width = WIDTH;
    windows->height = HEIGHT;
    windows->moved = false;
    windows->resized = true;
    windows->isClicked = false;
    windows->changed = true;
    windows->board_size = BOARD_SIZE;
    windows->text_img = NULL;
    windows->img = NULL;
    windows->mlx = NULL;
    gameData->board_size = windows->board_size;
    
    resetGame(gameData, windows);
    return true;
}

void resetGame(game *gameData, screen *windows)
{
    /* Initialize the entire fixed-size board to zero to avoid uninitialized memory */
    for (int i = 0; i < 19; i++)
    {
        for (int j = 0; j < 19; j++)
            gameData->board[i][j] = 0;
    }
    gameData->iaTurn = 2;
    gameData->ia_timer.elapsed = 0.0;
    gameData->turn = 1; // player 1 starts
    gameData->game_over = false;
    gameData->ia_timer.running = false;
    gameData->ia_timer.start_ts.tv_sec = 0;
    gameData->ia_timer.start_ts.tv_nsec = 0;
    gameData->score[0] = 0;
    gameData->score[1] = 0;

    resetScreen(windows, gameData->board);
}


void putPiecesOnBoard(screen *windows, int board[19][19])
{
    for (int i = 0; i < windows->board_size; i++)
    {
        for (int j = 0; j < windows->board_size; j++)
        {
            if (board[i][j] == 1 || board[i][j] == 2)
                drawSquare(windows, j, i, board[i][j]);
        }
    }
}

void resetScreen(screen *windows, int board[19][19])
{
    // 1. Effacer l'écran (remplir de noir ou texture bois)
    printBlack(windows); 
    
    // 2. Dessiner la grille
    putCadrillage(windows);
    
    // 3. Dessiner TOUTES les pièces existantes (C'est ça qui fait apparaître vos fantômes)
    putPiecesOnBoard(windows, board);

    // 4. Dessiner le bouton replay
    drawReplayButton(windows);
}

void makeIaMove(game *gameData, screen *windows)
{
    // printf("IA (Player %d) is thinking...\n", gameData->turn);
    // NOUVEAU : On demande une profondeur max très élevée (12).
    // Le solveur utilise "Iterative Deepening" et s'arrêtera tout seul
    // quand le temps (0.45s) sera écoulé.
    const int max_depth_limit = 12; 
    
    // On garde le timer ici juste pour l'affichage (le solveur a le sien en interne)
    launchTimer(&gameData->ia_timer);
    
    move best_move = findBestMove(gameData, max_depth_limit);
    
    // stopTimer(&gameData->ia_timer);
    
    // Note : best_move.score contient le score du plateau, pas la profondeur atteinte.
    // printf("IA chose move (%d, %d) with score %d in %.3f seconds.\n", 
    //     best_move.position.x, best_move.position.y, best_move.score, 
    //     gameData->ia_timer.elapsed);
    // return ;
    if (best_move.position.x != -1)
    {
        int x = best_move.position.x;
        int y = best_move.position.y;
        
        gameData->board[y][x] = gameData->turn;
        
        drawSquare(windows, x, y, gameData->turn);
        checkPieceCapture(gameData, windows, x, y);
        
        windows->changed = true;
        gameData->turn = (gameData->turn == 1) ? 2 : 1;
    }
    else
    {
        // printf("IA: No valid moves found or Board Full.\n");
        // Optionnel : gameData->game_over = true;
    }
    return ;
}

void gameLoop(void *param)
{
    both        *args = (struct both *)param;
    screen      *windows = args->windows;
    game        *gameData = args->gameData;

    // Gestion du redimensionnement
    if (windows->resized)
    {
        windows->resized = false;
        resetScreen(windows, gameData->board);
        windows->changed = true;
    }
    
    // ====================================================
    // FIX AFFICHAGE : Retarder l'IA d'une ou deux frames
    // ====================================================
    static int frames_to_wait = 0;

    if (isIaTurn(gameData->iaTurn, gameData->turn) && !gameData->game_over)
    {
        // Si c'est au tour de l'IA, on attend que l'affichage du joueur soit passé
        if (frames_to_wait < 2) 
        {
            frames_to_wait++;
            return; // On rend la main à MLX pour qu'il dessine le coup précédent
        }

        // Le délai est passé, l'IA réfléchit
        makeIaMove(gameData, windows);
        
        // On remet le compteur à 0 pour le prochain tour
        frames_to_wait = 0;
    }
    else
    {
        // Si c'est au joueur, on s'assure que le compteur est prêt
        frames_to_wait = 0;
    }
    
    // Le reste ne change pas...
    if (windows->changed)
    {
        checkVictoryCondition(gameData, windows);
        windows->changed = false;
    }

    printInformation(windows, gameData);
}

void launchGame(game *gameData, screen *windows)
{
    both args;
    args.windows = windows;
    args.gameData = gameData;
    (void)gameData;

    windows->mlx = mlx_init((int32_t)windows->width, (int32_t)windows->height, "Gomoku", true);
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

    mlx_resize_hook(windows->mlx, &resize, windows);
    mlx_loop_hook(windows->mlx, &gameLoop, &args);
    mlx_cursor_hook(windows->mlx, &cursor, windows);
    mlx_mouse_hook(windows->mlx, &mousehook, &args);
    mlx_key_hook(windows->mlx, &keyhook, &args);
    mlx_loop(windows->mlx);
    // mlx_close_hook(windows->mlx, &closeScreen, windows);
    // mlx_delete_image(windows->mlx, windows->img);
    mlx_terminate(windows->mlx);
}


int main(int argc, char **argv)
{
    (void) argv;
    void *args; // args will be used to know wich mode of play we are (player vs machine, p vs p, m vs m, or other game mode)
    screen windows; // windows will be used to put the MLX nescessaries information inside
    game gameData; // will contain all nescessary data for the game

    if (!checkArgs(argc,argv, &args))
    {
        return (EXIT_FAILURE);
    }

    if (!initialized(args, &windows, &gameData)) // initialized will launch the game windows and initialized the nescessary data
    {
        return (EXIT_FAILURE);
    }

    launchGame(&gameData, &windows);

    // exit(EXIT_SUCCESS);
    return (EXIT_SUCCESS);

}