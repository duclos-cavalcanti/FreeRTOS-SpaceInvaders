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


unsigned char xCheckSaucerCollision(saucer_t* saucer,
                                    H_Movement_t Direction,
                                    signed short b_xpos,
                                    signed short b_ypos)
{
    signed short R_OFFSET = 0;
    signed short L_OFFSET = 0;

    if(Direction == RIGHT){
        R_OFFSET = saucer->speed/2;
        L_OFFSET = 0;
    }
    else{
        L_OFFSET = saucer->speed/2;   
        R_OFFSET = 0;
    }
    signed short LEFT_LIMIT = saucer->x_pos - SAUCER_WIDTH/2 - L_OFFSET;
    signed short RIGHT_LIMIT = saucer->x_pos + SAUCER_WIDTH/2 + R_OFFSET;
    signed short LOWER_LIMIT = saucer->y_pos + SAUCER_HEIGHT/2;
    signed short UPPER_LIMIT = saucer->y_pos - SAUCER_HEIGHT/2;

    if (b_xpos <= RIGHT_LIMIT && b_xpos >= LEFT_LIMIT)
        if(b_ypos >= UPPER_LIMIT && b_ypos <= LOWER_LIMIT){
            return 1;
        }
        else return 0;
    else
        return 0;
}


H_Movement_t xCheckSaucerLeftEdgeDistance(saucer_t* saucer)
{
    if(saucer->x_pos - SAUCER_WIDTH/2 <= 0)
        return RIGHT;
    else
        return LEFT;
}

H_Movement_t xCheckSaucerRightEdgeDistance(saucer_t* saucer)
{
    if(saucer->x_pos + SAUCER_WIDTH/2 >= SCREEN_WIDTH)
        return LEFT;
    else
        return RIGHT;
}

H_Movement_t xCheckDirectionSaucer(saucer_t* saucer, H_Movement_t Direction)
{
    if(Direction == RIGHT)
        return xCheckSaucerRightEdgeDistance(saucer);
    else
        return xCheckSaucerLeftEdgeDistance(saucer);
}

void xCheckAISaucerBorder(saucer_t* saucer, H_Movement_t CurrentDirection)
{
    H_Movement_t LastDirection = CurrentDirection;
    H_Movement_t NewDirection =xCheckSaucerRightEdgeDistance(saucer);
    if(NewDirection!=LastDirection && LastDirection==RIGHT)
        saucer->x_pos=0+SAUCER_WIDTH/2;
    else if(NewDirection!=LastDirection && LastDirection==LEFT)
        saucer->x_pos=SCREEN_WIDTH - SAUCER_WIDTH/2;
}
void vMoveSaucerRight(saucer_t* saucer)
{
    saucer->x_pos+=saucer->speed;
}

void vMoveSaucerLeft(saucer_t* saucer)
{
    saucer->x_pos-=saucer->speed;
}
void vMoveSaucerHorizontal(saucer_t* saucer, H_Movement_t* Direction)
{
    (*Direction) = xCheckDirectionSaucer(saucer, (*Direction));
    if((*Direction) == RIGHT)
        vMoveSaucerRight(saucer);
    else
        vMoveSaucerLeft(saucer);
}

void vKillSaucer(unsigned char* SaucerHitFlag, unsigned char* SaucerAppearsFlag)
{
    (*SaucerHitFlag)=1;
    (*SaucerAppearsFlag)=0;
}

unsigned char xFetchSaucerValue()
{
    unsigned char value = (rand() % 100);
    value=value + 100;
    return value; 
} 

void vRetrieveDeadSaucerXY(signed short* DeadSaucerX, signed short* DeadSaucerY, saucer_t* saucer)
{
    (*DeadSaucerX) = saucer->x_pos;
    (*DeadSaucerY) = saucer->y_pos;
}

