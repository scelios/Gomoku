#include "../include/gomoku.h"

void	resize(int32_t width, int32_t height, void *param)
{
    screen	*windows = (struct screen *)param;

    if (!windows)
        return;

    if (windows->img)
    {
        mlx_delete_image(windows->mlx, windows->img);
        windows->img = NULL;
    }
    /* remove any text image that references the old buffers */
    if (windows->text_img)
    {
        mlx_delete_image(windows->mlx, windows->text_img);
        windows->text_img = NULL;
    }
    windows->img = mlx_new_image(windows->mlx, width, height);
    if (!windows->img)
    {
        // fprintf(stderr, "mlx_new_image failed in resize\n");
        mlx_close_window(windows->mlx);
        return;
    }

    windows->width = width;
    windows->height = height;
    windows->moved = true;
    windows->resized = true;
    windows->changed = true;
    mlx_image_to_window(windows->mlx, windows->img, 0, 0);
}

void	cursor(double xpos, double ypos, void *param)
{
    screen		*windows = (struct screen *)param;

    windows->x = xpos;
    windows->y = ypos;
}

void	keyhook(mlx_key_data_t keydata, void *param)
{
    both   *args = (struct both *)param;
    screen *windows = args->windows;
    game   *gameData = args->gameData;

    if (keydata.key == MLX_KEY_ESCAPE && keydata.action == MLX_PRESS)
    {
        // printf("Escape key pressed, closing window.\n");
        mlx_close_window(windows->mlx);
    }
    if (keydata.key == MLX_KEY_SPACE && keydata.action == MLX_PRESS)
    {
        // set ia mode on
        gameData->iaTurn = (gameData->iaTurn == 0) ? 1 : 0;
    }
}

void	mousehook(mouse_key_t button, action_t action, modifier_key_t mods, void *param)
{
    both   *args = (struct both *)param;
    screen *windows = args->windows;
    game   *gameData = args->gameData;
    (void)mods;

    if (button == MLX_MOUSE_BUTTON_LEFT && action == MLX_PRESS)
    {
        if (windows->x >= BUTTON_X && windows->x <= BUTTON_X + BUTTON_WIDTH &&
            windows->y >= BUTTON_Y && windows->y <= BUTTON_Y + BUTTON_HEIGHT)
        {
            resetGame(gameData, windows);
            return;
        }
    }

    int ml = BOARD_MARGIN_LEFT;
    int mr = BOARD_MARGIN_RIGHT;
    int mt = BOARD_MARGIN_TOP;
    int mb = BOARD_MARGIN_BOTTOM;

    int drawable_w = (int)windows->width - ml - mr;
    int drawable_h = (int)windows->height - mt - mb;
    if (drawable_w <= 0 || drawable_h <= 0 || windows->board_size <= 0)
         return;

    /* ignore clicks outside the board rectangle */
    if (windows->x < ml || windows->x >= (ml + drawable_w) ||
        windows->y < mt || windows->y >= (mt + drawable_h))
         return;

    double rel_x = windows->x - ml;
    double rel_y = windows->y - mt;

    int cell_x = (int)(rel_x * windows->board_size / (double)drawable_w);
    int cell_y = (int)(rel_y * windows->board_size / (double)drawable_h);

    if (cell_x < 0) cell_x = 0;
    if (cell_y < 0) cell_y = 0;
    if (cell_x >= windows->board_size) cell_x = windows->board_size - 1;
    if (cell_y >= windows->board_size) cell_y = windows->board_size - 1;

    if (button == MLX_MOUSE_BUTTON_LEFT && action != MLX_RELEASE)
    {
        if (!gameData->game_over && gameData->board[cell_y][cell_x] == 0)
        {
            // printf("Placing piece for player %d at (%d, %d)\n", gameData->turn, cell_x, cell_y);
            gameData->board[cell_y][cell_x] = gameData->turn;
            /* check captures caused BY this new stone only */
            checkPieceCapture(gameData, windows, cell_x, cell_y);
            drawSquare(windows, cell_x, cell_y, gameData->turn);
            gameData->turn = (gameData->turn == 1) ? 2 : 1;
            windows->changed = true;
        }
    }
}