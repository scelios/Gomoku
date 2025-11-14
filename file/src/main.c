# include "../include/gomoku.h"

bool checkArgs(int argc, char **argv, void**args)
{
    if (argc == 1)
    {
        *args = NULL; // default mode
    }
    else
    {
        // parse arguments here for different game modes
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
    for (int i = 0; i < gameData->board_size; i++)
    {
        for (int j = 0; j < gameData->board_size; j++)
        {
            gameData->board[i][j] = 0; // empty cell
        }
    }
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

bool in_bounds(int x, int y, int n){
    return x >= 0 && y >= 0 && x < n && y < n;
}

void checkPieceCapture(game *gameData, screen *windows, int lx, int ly)
{
    if (!gameData || !windows)
        return;

    int n = gameData->board_size;
    if (n <= 0 || n > 50)
        return;

    /* safeguard: last move must be inside board */
    if (!in_bounds(lx, ly, n))
        return;

    bool marked[50][50] = { false };
    int captures_by_player[2] = {0, 0};

    int owner = gameData->board[ly][lx];
    if (owner != 1 && owner != 2)
        return;
    int opponent = (owner == 1) ? 2 : 1;

    /* 8 directions */
    const int dirs[8][2] = {
        { 1, 0}, { 0, 1}, {-1, 0}, { 0,-1},
        { 1, 1}, { 1,-1}, {-1, 1}, {-1,-1}
    };

    for (int d = 0; d < 8; d++)
    {
        int dx = dirs[d][0];
        int dy = dirs[d][1];

        int x1 = lx + dx, y1 = ly + dy;
        int x2 = lx + 2*dx, y2 = ly + 2*dy;
        int x3 = lx + 3*dx, y3 = ly + 3*dy;

        if (!in_bounds(x1, y1, n) || !in_bounds(x2, y2, n) || !in_bounds(x3, y3, n))
            continue;

        /* pattern: owner (lx,ly) - opponent - opponent - owner (x3,y3) */
        if (gameData->board[y1][x1] == opponent &&
            gameData->board[y2][x2] == opponent &&
            gameData->board[y3][x3] == owner)
        {
            /* mark the two opponent stones for removal (avoid double counting) */
            if (!marked[y1][x1] && !marked[y2][x2])
            {
                marked[y1][x1] = true;
                marked[y2][x2] = true;
                captures_by_player[owner - 1] += 1; /* one pair captured */
            }
        }
    }

    /* apply captures (if any) */
    int total_captures = 0;
    for (int y = 0; y < n; y++)
    {
        for (int x = 0; x < n; x++)
        {
            if (marked[y][x])
            {
                gameData->board[y][x] = 0;
                total_captures++;
                drawSquare(windows, x, y, 0); /* redraw empty square */
            }
        }
    }

    if (total_captures > 0)
    {
        /* update scores: captures_by_player counts pairs */
        gameData->score[0] += captures_by_player[0];
        gameData->score[1] += captures_by_player[1];

        /* request redraw once */
        windows->changed = true;
    }
}

void checkVictoryCondition(game *gameData, screen *windows)
{
    // Placeholder for victory condition logic
    (void)gameData;
    (void)windows;
}

void makeIaMove(game *gameData, screen *windows)
{
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