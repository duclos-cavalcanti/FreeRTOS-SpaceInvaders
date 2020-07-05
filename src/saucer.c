#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "creatures.h"
#include "saucer.h"


saucer_t* CreateSinglePlayerSaucer()
{
    saucer_t* Saucer = calloc(1,sizeof(saucer_t));
    if(!Saucer){
        fprintf(stderr,"Unable to create Saucer.");
        exit(EXIT_FAILURE);
    }

    Saucer->x_pos = SAUCER_APPEARS_X;
    Saucer->y_pos = SAUCER_APPEARS_Y;
    Saucer->speed = SAUCER_SPEED;


    return Saucer;
}



