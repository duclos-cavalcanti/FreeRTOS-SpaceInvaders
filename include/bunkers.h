#ifndef __bunkers_H__
#define __bunkers_H__

#include "TUM_Draw.h"
#include "ship.h"


#define BUNKERS_Y_POS SCREEN_HEIGHT*70/100
#define BUNKER_SIDE_SIZE 40
#define BUNKER_COLOR Green

#define B1_X_POS SCREEN_WIDTH*1/8
#define B2_X_POS SCREEN_WIDTH*3/8
#define B3_X_POS SCREEN_WIDTH*5/8
#define B4_X_POS SCREEN_WIDTH*7/8

#define BUNKERS_LOWERLIMIT BUNKERS_Y_POS + BUNKER_SIDE_SIZE

typedef struct SingleBunker_t{
    signed short x_pos;
    signed short y_pos;
    
    unsigned int color;
    signed short size;

}SingleBunker_t;

typedef struct bunkers_t{
    SingleBunker_t* b1;
    unsigned short b1Lives;

    SingleBunker_t* b2;
    unsigned short b2Lives;

    SingleBunker_t* b3;
    unsigned short b3Lives;
    
    SingleBunker_t* b4;
    unsigned short b4Lives;

}bunkers_t;


bunkers_t* CreateBunkers();

unsigned char xCheckBunkersCollision(ship_t* ship);
void vUpdateBunkersStatus(bunkers_t* bunkers);

#endif
