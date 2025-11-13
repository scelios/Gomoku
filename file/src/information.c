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
    /* compute total elapsed clock ticks (accumulated + current if running) */
    clock_t total_ticks = gameData->ia_timer.elapsed_time;
    if (gameData->ia_timer.running)
        total_ticks += (clock() - gameData->ia_timer.start_time);
    double elapsed_sec = (double)total_ticks / (double)CLOCKS_PER_SEC;
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
    windows->text_img = mlx_put_string(windows->mlx, sprintfBuf, 10, 10);
}
