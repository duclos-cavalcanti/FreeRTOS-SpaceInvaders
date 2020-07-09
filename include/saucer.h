
#ifndef __saucer_H__
#define __saucer_H__


#include "TUM_Draw.h"
#include "creatures.h"
#include "main.h"

#define SAUCER_WIDTH 35
#define SAUCER_HEIGHT 23

#define SAUCER_APPEARS_X SCREEN_WIDTH/2
#define SAUCER_APPEARS_Y SCREEN_HEIGHT*1/10 + 15
#define SAUCER_SPEED 2

typedef struct saucer_t{
    signed short x_pos;
    signed short y_pos;
    signed short speed;
    
    image_handle_t Image;
}saucer_t;

typedef enum OpponentCommands_t{
    NONE,
    INC=1,
    DEC=-1
}OpponentCommands_t;



saucer_t* CreateSinglePlayerSaucer();


void vMoveSaucerHorizontal(saucer_t* saucer, H_Movement_t* Direction);

unsigned char xCheckSaucerCollision(saucer_t* saucer,
                                    H_Movement_t Direction,
                                    signed short b_xpos,
                                    signed short c_ypos);


void vKillSaucer(unsigned char* SaucerHitFlag, unsigned char* SaucerAppearsFlag);
void vRetrieveDeadSaucerXY(signed short* DeadSaucerX, signed short* DeadSaucerY, saucer_t* saucer);

unsigned char xFetchSaucerValue();






#endif
