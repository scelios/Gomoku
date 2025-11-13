#include "../include/gomoku.h"

void resetTimer(timer *t)
{
    t->start_time = 0;
    t->elapsed_time = 0;
    t->running = false;
}

void launchTimer(timer *t)
{
    if (!t->running)
    {
        resetTimer(t);
        t->start_time = clock();
        t->running = true;
    }
}
void stopTimer(timer *t)
{
    if (t->running)
    {
        t->elapsed_time += clock() - t->start_time;
        t->running = false;

    }
}