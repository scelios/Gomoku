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
    windows->resized = false;
    windows->isClicked = false;
    windows->changed = true;
    gameData->board_size = BOARD_SIZE;
    gameData->board = malloc(sizeof(int*) * gameData->board_size);
    if (!gameData->board)
    {
        perror("Failed to allocate memory for game board");
        return false;
    }
    for (int i = 0; i < gameData->board_size; i++)
    {
        gameData->board[i] = malloc(sizeof(int) * gameData->board_size);
        if (!gameData->board[i])
        {
            perror("Failed to allocate memory for game board row");
            for (int j = 0; j < i; j++)
                free(gameData->board[j]);
            free(gameData->board);
            return false;
        }
        memset(gameData->board[i], 0, sizeof(int) * gameData->board_size);
    }
    gameData->turn = 1; // player 1 starts
    gameData->game_over = false;
    return true;
}

void gameLoop(void *param)
{
    screen  *windows = (struct screen *)param;

    if (windows->changed)
    {
        printBlack(windows);
        windows->changed = false;
    }
}

void launchGame(game *gameData, screen *windows)
{
    (void)gameData;

    windows->mlx = mlx_init((int32_t)windows->width, (int32_t)windows->height, "Gomoku", true);
    if (!windows->mlx)
    {
        perror("mlx_init failed");
        exit(EXIT_FAILURE);
    }
    windows->img = mlx_new_image(windows->mlx, windows->width, windows->height);
    mlx_image_to_window(windows->mlx, windows->img, 0, 0);
    // mlx_resize_hook(windows->mlx, &resize, &windows);
    mlx_loop_hook(windows->mlx, &gameLoop, &windows);
    mlx_cursor_hook(windows->mlx, &cursor, &windows);
    mlx_key_hook(windows->mlx, &keyhook, &windows);
    // mlx_mouse_hook(windows->mlx, &mousehook, &windows);
    mlx_loop(windows->mlx);
    // mlx_close_hook(windows->mlx, &closeScreen, &windows);
    // mlx_delete_image(windows->mlx, windows->img);
    mlx_terminate(windows->mlx);
}

void freeData(game gameData)
{
    for (int i = 0; i < gameData.board_size; i++)
    {
        free(gameData.board[i]);
    }
    free(gameData.board);
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
    // freeData(gameData);
    
    return (EXIT_SUCCESS);

}