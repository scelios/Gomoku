#include "../include/gomoku.h"

void printInformation(screen *windows, game *gameData)
{
    if (isIaTurn(gameData->iaTurn, gameData->turn))
    {
        launchTimer(&gameData->ia_timer);
    }
    else
    {
        stopTimer(&gameData->ia_timer);
    }
    char sprintfBuf[128];
    /* compute total elapsed seconds (accumulated + current if running) */
    double elapsed_sec = gameData->ia_timer.elapsed;
    if (gameData->ia_timer.running)
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed_sec += (now.tv_sec - gameData->ia_timer.start_ts.tv_sec)
                     + (now.tv_nsec - gameData->ia_timer.start_ts.tv_nsec) / 1e9;
    }
    /* format time and scores (replace 0,0 by your score fields if available) */
    snprintf(sprintfBuf, sizeof sprintfBuf,
             "IA time: %.2fs  Player 1 score: %d  Player 2 score: %d",
             elapsed_sec, gameData->score[0], gameData->score[1]);
    
    /* delete previous text image to avoid overlays */
    if (windows->text_img)
    {
        mlx_delete_image(windows->mlx, windows->text_img);
        windows->text_img = NULL;
    }
    /* create new text image and keep pointer */
    windows->text_img = mlx_put_string(windows->mlx, sprintfBuf, BOARD_MARGIN_LEFT, 10);
}
