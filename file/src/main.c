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
    // Initialize game board to empty
    windows->width = WIDTH;
    windows->height = HEIGHT;
    windows->moved = false;
    windows->resized = true;
    windows->isClicked = false;
    windows->changed = true;
    windows->board_size = BOARD_SIZE;
    windows->text_img = NULL;
    gameData->board_size = windows->board_size;
    /* Initialize the entire fixed-size board to zero to avoid uninitialized memory */
    for (int i = 0; i < 50; i++)
    {
        for (int j = 0; j < 50; j++)
            gameData->board[i][j] = 0;
    }
    gameData->iaTurn = 0;
    gameData->ia_timer.elapsed = 0.0;
    gameData->turn = 1; // player 1 starts
    gameData->game_over = false;
    gameData->ia_timer.running = false;
    gameData->ia_timer.start_ts.tv_sec = 0;
    gameData->ia_timer.start_ts.tv_nsec = 0;
    gameData->score[0] = 0;
    gameData->score[1] = 0;
    return true;
}


void putPiecesOnBoard(screen *windows, int board[50][50])
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

void resetScreen(screen *windows, int board[50][50])
{
    printBlack(windows);
    putCadrillage(windows);
    putPiecesOnBoard(windows, board);
}

void makeIaMove(game *gameData, screen *windows)
{
    // TODO
    // Placeholder for IA move logic
    (void)gameData;
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
        if (isIaTurn(gameData->iaTurn, gameData->turn) && !gameData->game_over)
        {
            makeIaMove(gameData, windows);
        }
        checkVictoryCondition(gameData, windows);
        windows->changed = false;
        gameData->turn = (gameData->turn == 1) ? 2 : 1;
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