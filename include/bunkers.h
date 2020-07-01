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

#define BUNKERS_LOWERLIMIT BUNKERS_Y_POS + BUNKER_SIDE_SIZE/2
#define BUNKERS_UPPERLIMIT BUNKERS_Y_POS - BUNKER_SIDE_SIZE/2

#define B1_LEFT_LIMIT B1_X_POS - BUNKER_SIDE_SIZE/2 - SHIP_BULLET_THICKNESS/2
#define B1_RIGHT_LIMIT B1_X_POS + BUNKER_SIDE_SIZE/2 + SHIP_BULLET_THICKNESS/2

#define B2_LEFT_LIMIT B2_X_POS - BUNKER_SIDE_SIZE/2- SHIP_BULLET_THICKNESS/2
#define B2_RIGHT_LIMIT B2_X_POS + BUNKER_SIDE_SIZE/2+ SHIP_BULLET_THICKNESS/2

#define B3_LEFT_LIMIT B3_X_POS - BUNKER_SIDE_SIZE/2- SHIP_BULLET_THICKNESS/2
#define B3_RIGHT_LIMIT B3_X_POS + BUNKER_SIDE_SIZE/2+ SHIP_BULLET_THICKNESS/2

#define B4_LEFT_LIMIT B4_X_POS - BUNKER_SIDE_SIZE/2- SHIP_BULLET_THICKNESS/2
#define B4_RIGHT_LIMIT B4_X_POS + BUNKER_SIDE_SIZE/2+ SHIP_BULLET_THICKNESS/2

typedef enum Bunkers_ID_t{
    NONEXISTENT_BUNKER,
    B1,
    B2,
    B3,
    B4
}Bunkers_ID_t;

typedef struct SingleBunker_t{
    signed short x_pos;
    signed short y_pos;
    
    unsigned int color;
    signed short size;

}SingleBunker_t;

typedef struct bunkers_t{
    SingleBunker_t* b1;
    signed short b1Lives;

    SingleBunker_t* b2;
    signed short b2Lives;

    SingleBunker_t* b3;
    signed short b3Lives;
    
    SingleBunker_t* b4;
    signed short b4Lives;

}bunkers_t;


bunkers_t* CreateBunkers();

unsigned char xCheckBunkersCollision(signed short bullet_xpos, signed short bullet_ypos,
                                     bunkers_t bunkers);

void vUpdateBunkersStatus(bunkers_t* bunkers, unsigned char bunkerID);

#endif
