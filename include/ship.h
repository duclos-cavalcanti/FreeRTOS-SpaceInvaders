#ifndef __ship_H__
#define __ship_H__

#include "TUM_Draw.h"

#define SHIP_BULLET_SPEED 8
#define SHIP_Y_OFFSET 5
#define SHIP_BULLET_THICKNESS 5

typedef struct bullet_t{
signed short x_pos;
signed short y_pos;
signed short speed;

unsigned char BulletAliveFlag;
}bullet_t;

typedef struct ship_t{
    signed short x_pos;
    signed short y_pos;
    signed short speed;

    unsigned short size;
    bullet_t* bullet;
}ship_t;

ship_t* CreateShip(signed short initial_x, signed short initial_y, signed short speed, 
                   unsigned int ShipColor, signed short size);

void vIncrementShipLeft(ship_t* ship);
void vIncrementShipRight(ship_t* ship);

void CreateBullet(ship_t* ship);

unsigned char xCheckShipBulletCollision(ship_t* ship);

void vUpdateShipBulletPos(ship_t* ship);
void vDrawShipBullet(ship_t* ship);
#endif 
