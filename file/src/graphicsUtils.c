# include "../include/gomoku.h"

int get_rgba(int r, int g, int b, int a)
{
    return (r << 24 | g << 16 | b << 8 | a);
}

/* safe pixel put: check boundaries before writing */
static inline void safe_put_pixel(screen *windows, int x, int y, int color)
{
    if (!windows || !windows->img)
        return;
    if (x < 0 || y < 0)
        return;
    if (x >= windows->width || y >= windows->height)
        return;
    mlx_put_pixel(windows->img, x, y, color);
}

void printBlack(screen *windows)
{
    for (int i = 0; i < windows->width; i++)
    {
        for (int j = 0; j < windows->height; j++)
        {
            safe_put_pixel(windows, i, j, get_rgba(0, 0, 0, 255));
        }
    }
}

void putCadrillage(screen *windows)
{
    if (windows->board_size <= 0)
        return;
    int cell_w = windows->width / windows->board_size;
    int cell_h = windows->height / windows->board_size;
    if (cell_w <= 0 || cell_h <= 0)
        return;

    for (int i = 0; i < windows->width; i++)
    {
        for (int j = 0; j < windows->height; j++)
        {
            if (i % cell_w == 0 || j % cell_h == 0)
                safe_put_pixel(windows, i, j, get_rgba(255, 0, 0, 255));
        }
    }
}

int teamColor(unsigned short int team)
{
    switch (team)
    {
    case 0:
        return get_rgba(0, 0, 0, 0);
    case 1:
        return get_rgba(0, 255, 0, 255);
    case 2:
        return get_rgba(255, 0, 0, 255);
    default:
        return get_rgba(255, 255, 255, 255);
    }
}

void drawSquare(screen *windows, int x0, int y0, unsigned short int team)
{
    if (windows->board_size <= 0)
        return;
    int cell_w = windows->width / windows->board_size;
    int cell_h = windows->height / windows->board_size;
    if (cell_w <= 0 || cell_h <= 0)
        return;

    int radius_x = cell_w / 4;
    int radius_y = cell_h / 4;

    int cx = x0 * cell_w + cell_w / 2;
    int cy = y0 * cell_h + cell_h / 2;

    int x_start = cx - radius_x;
    int x_end   = cx + radius_x;
    int y_start = cy - radius_y;
    int y_end   = cy + radius_y;

    if (x_start < 0) x_start = 0;
    if (y_start < 0) y_start = 0;
    if (x_end >= windows->width) x_end = windows->width - 1;
    if (y_end >= windows->height) y_end = windows->height - 1;

    int color = teamColor(team);
    for (int i = x_start; i <= x_end; i++)
        for (int j = y_start; j <= y_end; j++)
            safe_put_pixel(windows, i, j, color);
}