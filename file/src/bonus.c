#include "../include/gomoku.h"


void flashbang(screen *windows)
{
    if (!windows) return;

    // Flash the screen white briefly
    for (int y = 0; y < windows->height; y++)
    {
        for (int x = 0; x < windows->width; x++)
        {
            mlx_put_pixel(windows->img, x, y, 0xFFFFFFFF); // White pixel
        }
    }
    mlx_image_to_window(windows->mlx, windows->img, 0, 0);
    windows->changed = true;
}

void openBrainRot()
{
    const char *url[] = {"https://www.youtube.com/shorts/6e0ctwJ_yrI",
                         "https://www.youtube.com/shorts/iqnLbqsS8-w",
                         "https://www.youtube.com/shorts/rKWMzbS2zU8",
                         "https://www.youtube.com/shorts/twyGVqu6kmI",
                         "https://www.youtube.com/shorts/twyGVqu6kmI",
    };
    int choice = rand() % 5;
    char command[256];
    
    // Open browser to a funny video and sleep for the duration of the video
    #ifdef _WIN32
        snprintf(command, sizeof(command), "start %s", url[choice]);
        system(command);
#elif __APPLE__
        snprintf(command, sizeof(command), "open %s", url[choice]);
        system(command);
#else // Linux and others
        snprintf(command, sizeof(command), "xdg-open %s", url[choice]);
        system(command);
#endif
    // Sleep for 45 seconds to allow the video to play
    sleep(5);
}
