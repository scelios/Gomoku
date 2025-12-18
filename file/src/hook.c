#include "../include/gomoku.h"

void resize(int32_t width, int32_t height, void *param)
{
    screen *windows = (struct screen *)param;

    windows->width = width;
    windows->height = height;

    if (windows->img) mlx_delete_image(windows->mlx, windows->img);
    
    windows->img = mlx_new_image(windows->mlx, width, height);
    mlx_image_to_window(windows->mlx, windows->img, 0, 0);

    if (windows->restart_text)
    {
        mlx_delete_image(windows->mlx, windows->restart_text);
        windows->restart_text = NULL;
        initGUI(windows); 
    }

    windows->resized = true;
    windows->changed = true;
}

void cursor(double xpos, double ypos, void *param)
{
    screen *windows = (struct screen *)param;
    windows->x = xpos;
    windows->y = ypos;
}

void keyhook(mlx_key_data_t keydata, void *param)
{
    both *data = (both *)param;
    screen *windows = data->windows;
    game *gameData = data->gameData;

    if (keydata.key == MLX_KEY_ESCAPE && keydata.action == MLX_PRESS)
    {
        #ifdef DEBUG
            printf("Escape key pressed, closing window.\n");
        #endif
        mlx_close_window(windows->mlx);
    }
    
    // Toggle IA
    if (keydata.key == MLX_KEY_SPACE && keydata.action == MLX_PRESS)
    {
        gameData->iaTurn = (gameData->iaTurn == 0) ? P2 : 0;
        #ifdef DEBUG
            printf("IA Mode: %s\n", gameData->iaTurn ? "ON (Player 2)" : "OFF");
        #endif
        
        // ✅ Broadcast state change to frontend
        if (data->mgr)
            broadcast_board_state_external(data->mgr, gameData, windows);
    }

    // Hint key
    if (keydata.key == MLX_KEY_H && keydata.action == MLX_PRESS)
    {
        suggest_move(gameData, windows, gameData->turn);
        
        // ✅ Broadcast hint to frontend
        if (data->mgr)
            broadcast_board_state_external(data->mgr, gameData, windows);
    }
}

void mousehook(mouse_key_t button, action_t action, modifier_key_t mods, void *param)
{
    both *args = (struct both *)param;
    screen *windows = args->windows;
    game *gameData = args->gameData;
    (void)mods;

    // RESET BUTTON
    if (button == MLX_MOUSE_BUTTON_LEFT && action == MLX_PRESS)
    {
        // On vérifie si la souris est dans le rectangle du bouton
        if (windows->x >= BTN_X && windows->x <= BTN_X + BTN_W &&
            windows->y >= BTN_Y && windows->y <= BTN_Y + BTN_H)
        {
            resetGame(gameData, windows);
            return; // On arrête là, on ne pose pas de pion !
        }
    }

    // Don't allow clicks during IA turn
    if (isIaTurn(gameData->iaTurn, gameData->turn))
        return;

    int ml = 30, mr = 30, mt = 30, mb = 30; 
    int drawable_w = (int)windows->width - ml - mr;
    int drawable_h = (int)windows->height - mt - mb;

    if (drawable_w <= 0 || drawable_h <= 0) return;

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
        int index = GET_INDEX(cell_x, cell_y);
        
        if (gameData->board[index] == EMPTY && !gameData->game_over)
        {
            gameData->board[index] = gameData->turn;
            drawSquare(windows, cell_x, cell_y, gameData->turn);
            checkPieceCapture(gameData, windows, cell_x, cell_y);
            
            gameData->turn = (gameData->turn == P1) ? P2 : P1;
            gameData->hint_idx = -1;
            windows->changed = true;
            
            // ✅ Broadcast move to frontend
            if (args->mgr)
                broadcast_board_state_external(args->mgr, gameData, windows);
        }
    }
}