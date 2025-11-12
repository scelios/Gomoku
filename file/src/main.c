# include "../include/gomoku.h"

bool checkArgs(int argc, char **argv, void**args)
{
    return true;
}

bool initialized(void *args, void **windows, void **game)
{
    
    return true;
}


int32_t main(int argc, char **argv)
{
    void *args; // args will be used to know wich mode of play we are (player vs machine, p vs p, m vs m, or other game mode)
    void *windows; // windows will be used to put the MLX nescessaries information inside
    void *game; // will contain all nescessary data for the game

    if (!checkArgs(argc,argv, &args))
    {
        return (EXIT_FAILURE);
    }

    if (!initialized(args, &windows, &game)) // initialized will launch the game windows and initialized the nescessary data
    {
        return (EXIT_FAILURE);
    }

    // gameLoop(game,windows);

    
    return (EXIT_SUCCESS);

}