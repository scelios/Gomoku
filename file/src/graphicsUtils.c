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

    int ml = BOARD_MARGIN_LEFT;
    int mr = BOARD_MARGIN_RIGHT;
    int mt = BOARD_MARGIN_TOP;
    int mb = BOARD_MARGIN_BOTTOM;

    int drawable_w = (int)windows->width - ml - mr;
    int drawable_h = (int)windows->height - mt - mb;
    if (drawable_w <= 0 || drawable_h <= 0)
        return;

    int cell_w = drawable_w / windows->board_size;
    int cell_h = drawable_h / windows->board_size;
    if (cell_w <= 0 || cell_h <= 0)
        return;

    int color = get_rgba(255, 0, 0, 255);

    /* vertical lines: compute positions by scaling so lines stay inside drawable rect */
    for (int col = 0; col <= windows->board_size; col++)
    {
        /* position scaled to drawable area to account for integer remainders */
        int x = ml + (int)round((double)col * (double)drawable_w / (double)windows->board_size);
        if (x < ml) x = ml;
        if (x > ml + drawable_w) x = ml + drawable_w;
        /* draw only inside drawable vertical span */
        for (int y = mt; y < mt + drawable_h; y++)
            safe_put_pixel(windows, x, y, color);
    }

    /* horizontal lines: same scaling for Y */
    for (int row = 0; row <= windows->board_size; row++)
    {
        int y = mt + (int)round((double)row * (double)drawable_h / (double)windows->board_size);
        if (y < mt) y = mt;
        if (y > mt + drawable_h) y = mt + drawable_h;
        for (int x = ml; x < ml + drawable_w; x++)
            safe_put_pixel(windows, x, y, color);
    }
}

int teamColor(unsigned short int team)
{
    switch (team)
    {
    case 0:
        return get_rgba(0, 0, 0, 255);
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

    int ml = BOARD_MARGIN_LEFT;
    int mr = BOARD_MARGIN_RIGHT;
    int mt = BOARD_MARGIN_TOP;
    int mb = BOARD_MARGIN_BOTTOM;

    int drawable_w = (int)windows->width - ml - mr;
    int drawable_h = (int)windows->height - mt - mb;
    if (drawable_w <= 0 || drawable_h <= 0)
        return;

    /* compute cell size using same scaling as putCadrillage (distribute remainder) */
    double cell_w_f = (double)drawable_w / (double)windows->board_size;
    double cell_h_f = (double)drawable_h / (double)windows->board_size;

    /* center of the cell using rounding so it matches the grid lines */
    int cx = ml + (int)round((x0 + 0.5) * cell_w_f);
    int cy = mt + (int)round((y0 + 0.5) * cell_h_f);

    int radius_x = (int)round(cell_w_f * 0.25);
    int radius_y = (int)round(cell_h_f * 0.25);
    if (radius_x < 1) radius_x = 1;
    if (radius_y < 1) radius_y = 1;

    int x_start = cx - radius_x;
    int x_end   = cx + radius_x;
    int y_start = cy - radius_y;
    int y_end   = cy + radius_y;

    /* clamp to drawable area (inside margins) */
    if (x_start < ml) x_start = ml;
    if (y_start < mt) y_start = mt;
    if (x_end >= ml + drawable_w) x_end = ml + drawable_w - 1;
    if (y_end >= mt + drawable_h) y_end = mt + drawable_h - 1;

    int color = teamColor(team);
    for (int i = x_start; i <= x_end; i++)
    {
        for (int j = y_start; j <= y_end; j++)
        {
            safe_put_pixel(windows, i, j, color);
        }
    }
}

void drawReplayButton(screen *windows)
{
    int x_start = BUTTON_X;
    int y_start = BUTTON_Y;
    int width = BUTTON_WIDTH;
    int height = BUTTON_HEIGHT;
    int color = get_rgba(100, 100, 100, 255); // Grey
    int border_color = get_rgba(200, 200, 200, 255); // Light grey

    for (int x = x_start; x < x_start + width; x++)
    {
        for (int y = y_start; y < y_start + height; y++)
        {
            if (x == x_start || x == x_start + width - 1 || y == y_start || y == y_start + height - 1)
                safe_put_pixel(windows, x, y, border_color);
            else
                safe_put_pixel(windows, x, y, color);
        }
    }
}