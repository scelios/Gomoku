# include "../include/gomoku.h"

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

void	cursor(double xpos, double ypos, void *param)
{
    screen		*windows = (struct screen *)param;

    windows->x = xpos;
    windows->y = ypos;
}

void	keyhook(mlx_key_data_t keydata, void *param)
{
    screen	*windows = (struct screen *)param;

    if (keydata.key == MLX_KEY_ESCAPE && keydata.action == MLX_PRESS)
    {
        printf("Escape key pressed, closing window.\n");
        mlx_close_window(windows->mlx);
    }
}

int get_rgba(int r, int g, int b, int a)
{
    return (r << 24 | g << 16 | b << 8 | a);
}

void printBlack(screen *windows)
{
    for (int i = 0; i < windows->width; i++)
    {
        for (int j = 0; j < windows->height; j++)
        {
            // if (i % (windows->width / MAP_SIZE) == 0 || j % (windows->height / MAP_SIZE) == 0)
            // 	continue;
            mlx_put_pixel(windows->img, i, j, get_rgba(0, 0, 0, 255));
        }
    }
}
