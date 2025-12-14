#include "../include/gomoku.h"

void resize(int32_t width, int32_t height, void *param)
{
    screen *windows = (struct screen *)param;

    if (!windows) return;

    if (windows->img) mlx_delete_image(windows->mlx, windows->img);
    if (windows->text_img) mlx_delete_image(windows->mlx, windows->text_img);

    windows->img = mlx_new_image(windows->mlx, width, height);
    if (!windows->img)
    {
        fprintf(stderr, "mlx_new_image failed in resize\n");
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

void cursor(double xpos, double ypos, void *param)
{
    screen *windows = (struct screen *)param;
    windows->x = xpos;
    windows->y = ypos;
}

void keyhook(mlx_key_data_t keydata, void *param)
{
    both *args = (struct both *)param;
    screen *windows = args->windows;
    game *gameData = args->gameData;

    if (keydata.key == MLX_KEY_ESCAPE && keydata.action == MLX_PRESS)
    {
        printf("Escape key pressed, closing window.\n");
        mlx_close_window(windows->mlx);
    }
    // Activation/Désactivation IA
    if (keydata.key == MLX_KEY_SPACE && keydata.action == MLX_PRESS)
    {
        // Si 0 -> Devient P2 (IA joue les Blancs/O)
        // Si non 0 -> Devient 0 (Humain vs Humain)
        gameData->iaTurn = (gameData->iaTurn == 0) ? P2 : 0;
        printf("IA Mode: %s\n", gameData->iaTurn ? "ON (Player 2)" : "OFF");
    }
}

void mousehook(mouse_key_t button, action_t action, modifier_key_t mods, void *param)
{
    both *args = (struct both *)param;
    screen *windows = args->windows;
    game *gameData = args->gameData;
    (void)mods;

    // Pas de clic si c'est au tour de l'IA !
    if (isIaTurn(gameData->iaTurn, gameData->turn))
        return;

    // Marges définies dans .h (ou ici si locales)
    int ml = 30, mr = 30, mt = 30, mb = 30; 

    int drawable_w = (int)windows->width - ml - mr;
    int drawable_h = (int)windows->height - mt - mb;

    if (drawable_w <= 0 || drawable_h <= 0) return;

    /* ignore clicks outside the board rectangle */
    if (windows->x < ml || windows->x >= (ml + drawable_w) ||
        windows->y < mt || windows->y >= (mt + drawable_h))
         return;

    double rel_x = windows->x - ml;
    double rel_y = windows->y - mt;

    // Conversion précise souris -> case
    int cell_x = (int)(rel_x * windows->board_size / (double)drawable_w);
    int cell_y = (int)(rel_y * windows->board_size / (double)drawable_h);

    // Clamp (Sécurité)
    if (cell_x < 0) cell_x = 0;
    if (cell_y < 0) cell_y = 0;
    if (cell_x >= windows->board_size) cell_x = windows->board_size - 1;
    if (cell_y >= windows->board_size) cell_y = windows->board_size - 1;

    if (button == MLX_MOUSE_BUTTON_LEFT && action != MLX_RELEASE)
    {
        if (!gameData->game_over)
        {
            // Vérification case vide via Index 1D
            int idx = GET_INDEX(cell_x, cell_y);
            
            if (gameData->board[idx] == EMPTY)
            {
                // 1. Jouer le coup
                gameData->board[idx] = gameData->turn;
                
                // 2. Dessiner
                drawSquare(windows, cell_x, cell_y, gameData->turn);
                
                // 3. Gérer les captures
                checkPieceCapture(gameData, windows, cell_x, cell_y);
                
                // 4. Changer de tour
                gameData->turn = (gameData->turn == P1) ? P2 : P1;
                windows->changed = true;
                
                // Reset du timer pour l'IA (si elle doit jouer ensuite)
                if (isIaTurn(gameData->iaTurn, gameData->turn)) {
                     // launchTimer(...) si nécessaire
                }
            }
        }
    }
}