#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "bunkers.h"
#include "main.h"
#include "ship.h"


#define B1_LEFT_LIMIT B1_X_POS - BUNKER_SIDE_SIZE/2
#define B1_RIGHT_LIMIT B1_X_POS + BUNKER_SIDE_SIZE/2

#define B2_LEFT_LIMIT B2_X_POS - BUNKER_SIDE_SIZE/2
#define B2_RIGHT_LIMIT B2_X_POS + BUNKER_SIDE_SIZE/2

#define B3_LEFT_LIMIT B3_X_POS - BUNKER_SIDE_SIZE/2
#define B3_RIGHT_LIMIT B3_X_POS + BUNKER_SIDE_SIZE/2

#define B4_LEFT_LIMIT B4_X_POS - BUNKER_SIDE_SIZE/2
#define B4_RIGHT_LIMIT B4_X_POS + BUNKER_SIDE_SIZE/2


SingleBunker_t* CreateSingleBunker(signed short x_pos, signed short y_pos,
                                   unsigned int color, signed short size)
{
    SingleBunker_t* bunker = calloc(1,sizeof(SingleBunker_t));
    if(!bunker){
        fprintf(stderr,"Creating Single Bunker failed.\n");
        exit(EXIT_FAILURE);
    }

    bunker->x_pos=x_pos;
    bunker->y_pos=y_pos;
    bunker->color=color;
    bunker->size=size;

    return bunker;
}

bunkers_t* CreateBunkers()
{
   SingleBunker_t* b1=CreateSingleBunker(B1_X_POS,BUNKERS_Y_POS,BUNKER_COLOR, BUNKER_SIDE_SIZE);
   SingleBunker_t* b2=CreateSingleBunker(B2_X_POS,BUNKERS_Y_POS,BUNKER_COLOR, BUNKER_SIDE_SIZE);
   SingleBunker_t* b3=CreateSingleBunker(B3_X_POS,BUNKERS_Y_POS,BUNKER_COLOR, BUNKER_SIDE_SIZE);
   SingleBunker_t* b4=CreateSingleBunker(B4_X_POS,BUNKERS_Y_POS,BUNKER_COLOR, BUNKER_SIDE_SIZE);

    bunkers_t* Bunkers = calloc(1,sizeof(bunkers_t));
    if(!Bunkers){
        fprintf(stderr,"Creating Bunkers wrapper failed.\n");
        exit(EXIT_FAILURE);
    }

    Bunkers->b1=b1;
    Bunkers->b2=b2;
    Bunkers->b3=b3;
    Bunkers->b4=b4;
    
    Bunkers->b1Lives=5;
    Bunkers->b2Lives=5;
    Bunkers->b3Lives=5;
    Bunkers->b4Lives=5;

    return Bunkers;
}

unsigned char xCheckBunkerTopSideCollision(signed short b_xpos)
{
    return 0;
}

unsigned char xCheckBunkerLowSideCollision(signed short b_xpos)
{   
    if(B1_LEFT_LIMIT <= b_xpos && b_xpos < B1_RIGHT_LIMIT) 
        return B1;

    else if(B2_LEFT_LIMIT <= b_xpos && b_xpos < B2_RIGHT_LIMIT) 
        return B2;

    else if(B3_LEFT_LIMIT <= b_xpos &&  b_xpos < B3_RIGHT_LIMIT) 
        return B3;

    else if(B4_LEFT_LIMIT <= b_xpos && b_xpos < B4_RIGHT_LIMIT) 
        return B4;
    else
        return NONE;
}

unsigned char xCheckBunkersCollision(signed short b_xpos,signed short b_ypos)
{
    if(b_ypos >= BUNKERS_LOWERLIMIT){  
        if(b_ypos-SHIP_BULLET_SPEED <= BUNKERS_LOWERLIMIT) return xCheckBunkerLowSideCollision(b_xpos);
        else return 0;
    }
    else if(b_ypos <= BUNKERS_UPPERLIMIT)
        return xCheckBunkerTopSideCollision(b_xpos);
    else
        return 0;
}

void vUpdateBunkersStatus(bunkers_t* bunkers)
{

}