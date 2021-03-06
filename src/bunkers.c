/** Basic includes */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/** Game related */
#include "bunkers.h"
#include "main.h"
#include "ship.h"


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

unsigned char xCheckBunkerTopSideCollision(signed short b_xpos, bunkers_t bunkers)
{
    if(B1_LEFT_LIMIT <= b_xpos && b_xpos < B1_RIGHT_LIMIT){
        if(bunkers.b1Lives>0) 
            return B1;
        else return NONEXISTENT_BUNKER;
    }

    else if(B2_LEFT_LIMIT <= b_xpos && b_xpos < B2_RIGHT_LIMIT){
        if(bunkers.b2Lives>0) 
            return B2;
        else return NONEXISTENT_BUNKER;
    }

    else if(B3_LEFT_LIMIT <= b_xpos &&  b_xpos < B3_RIGHT_LIMIT){
        if(bunkers.b3Lives>0)
            return B3;
        else return NONEXISTENT_BUNKER;
    }   

    else if(B4_LEFT_LIMIT <= b_xpos && b_xpos < B4_RIGHT_LIMIT){
        if(bunkers.b4Lives>0)
            return B4;
        else return NONEXISTENT_BUNKER;
    }
    else
        return NONEXISTENT_BUNKER;
}

unsigned char xCheckBunkerLowSideCollision(signed short b_xpos, bunkers_t bunkers)
{   
    if(B1_LEFT_LIMIT <= b_xpos && b_xpos < B1_RIGHT_LIMIT){
        if(bunkers.b1Lives>0) 
            return B1;
        else return NONEXISTENT_BUNKER;
    }

    else if(B2_LEFT_LIMIT <= b_xpos && b_xpos < B2_RIGHT_LIMIT){
        if(bunkers.b2Lives>0) 
            return B2;
        else return NONEXISTENT_BUNKER;
    }

    else if(B3_LEFT_LIMIT <= b_xpos &&  b_xpos < B3_RIGHT_LIMIT){
        if(bunkers.b3Lives>0)
            return B3;
        else return NONEXISTENT_BUNKER;
    }   

    else if(B4_LEFT_LIMIT <= b_xpos && b_xpos < B4_RIGHT_LIMIT){
        if(bunkers.b4Lives>0)
            return B4;
        else return NONEXISTENT_BUNKER;
    }
    else
        return NONEXISTENT_BUNKER;
}

unsigned char xCheckBunkersCollision(signed short b_xpos,signed short b_ypos, bunkers_t bunkers)
{
    if(b_ypos >= BUNKERS_LOWERLIMIT){  //bullet is below lower limit of bunkers
        if(b_ypos-SHIP_BULLET_SPEED <= BUNKERS_LOWERLIMIT) return xCheckBunkerLowSideCollision(b_xpos, bunkers); //if next frame will lead to bullet being above its lower limit -> possible collision
        else return 0;
    }
    else if(b_ypos <= BUNKERS_UPPERLIMIT){ //if bullet is above upper limit of bunkers
        if(b_ypos+SHIP_BULLET_SPEED >= BUNKERS_UPPERLIMIT) return xCheckBunkerTopSideCollision(b_xpos, bunkers); //if next frame will lead to bullet being below its upper limit -> possible collision
        else return 0;
    }
    else
        return 0;
}

unsigned char xCheckSingleCreatureBunkerCollision(signed short c_xpos, 
                                                  signed short c_ypos,
                                                  bunkers_t* bunkers)
{
    for(int i=0;i<4;++i){
        if(c_ypos >= BUNKERS_UPPERLIMIT) 
            return xCheckBunkerTopSideCollision(c_xpos, bunkers[i]);
    }
    return 0;
}


void vUpdateBunkersStatus(bunkers_t* bunkers, unsigned char bunkerID)
{
    switch(bunkerID){
        case B1:
            if(bunkers->b1Lives>0)
                bunkers->b1Lives--;
            break;
        case B2:
            if(bunkers->b2Lives>0)
                bunkers->b2Lives--;
            break;
        case B3:
            if(bunkers->b3Lives>0)
                bunkers->b3Lives--;
            break;
        case B4:
            if(bunkers->b4Lives>0)
                bunkers->b4Lives--;
            break;

        case NONEXISTENT_BUNKER:
        default: 
            break;
    }
    
}

void vKillBunker(bunkers_t* bunkers, unsigned char bunkerID)
{
    switch(bunkerID){
        case B1:
            if(bunkers->b1Lives>0)
                bunkers->b1Lives=0;
            break;
        case B2:
            if(bunkers->b2Lives>0)
                bunkers->b2Lives=0;
            break;
        case B3:
            if(bunkers->b3Lives>0)
                bunkers->b3Lives=0;
            break;
        case B4:
            if(bunkers->b4Lives>0)
                bunkers->b4Lives=0;
            break;

        case NONEXISTENT_BUNKER:
        default: 
            break;
    }
    
}
