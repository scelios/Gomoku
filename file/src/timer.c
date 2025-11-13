#include "../include/gomoku.h"

void launchTimer(timer *t)
{
    if (!t) return;
    if (!t->running)
    {
        clock_gettime(CLOCK_MONOTONIC, &t->start_ts);
        t->running = true;
    }
}

void stopTimer(timer *t)
{
    if (!t) return;
    if (t->running)
    {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double diff = (now.tv_sec - t->start_ts.tv_sec) + (now.tv_nsec - t->start_ts.tv_nsec) / 1e9;
        t->elapsed += diff;
        t->running = false;
    }
}

void resetTimer(timer *t)
{
    if (!t) return;
    t->running = false;
    t->elapsed = 0.0;
    t->start_ts.tv_sec = 0;
    t->start_ts.tv_nsec = 0;
}