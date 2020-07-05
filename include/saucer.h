
#ifndef __saucer_H__
#define __saucer_H__


#include "TUM_Draw.h"
#include "ship.h"
#include "main.h"

#define SAUCER_WIDTH 35
#define SAUCER_HEIGHT 23

#define SAUCER_APPEARS_X SCREEN_WIDTH/2
#define SAUCER_APPEARS_Y SCREEN_HEIGHT*1/10 + 15
#define SAUCER_SPEED 10

typedef struct saucer_t{
    signed short x_pos;
    signed short y_pos;
    signed short speed;

    image_handle_t Image;
}saucer_t;


saucer_t* CreateSinglePlayerSaucer();











#endif
