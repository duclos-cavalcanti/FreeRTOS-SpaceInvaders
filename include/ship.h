#ifndef __ship_H__
#define __ship_H__

#include "bunkers.h"

#define SHIPSIZE 20
#define SHIPSPEED 4

#define SHIP_BULLET_SPEED 8
#define SHIP_Y_OFFSET 5
#define SHIP_BULLET_THICKNESS 3
#define SHIP_BULLET_LENGTH 10

typedef struct BunkerCollisionStatus_t{
    unsigned short BunkerID;
    unsigned char HIT;
}BunkerCollisionStatus_t;

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
    BunkerCollisionStatus_t* BunkerCollisionStatus;
}ship_t;

ship_t* CreateShip(signed short initial_x, signed short initial_y, signed short speed, 
                   unsigned int ShipColor, signed short size);

void CreateShipBullet(ship_t* ship);
BunkerCollisionStatus_t* CreateBunkerCollisionStatus();

unsigned char xCheckShipBulletCollisionTopWall(signed short b_ypos);

void vIncrementShipLeft(ship_t* ship);
void vIncrementShipRight(ship_t* ship);

void vUpdateShipBulletPos(ship_t* ship);
void vDrawShipBullet(ship_t* ship);
#endif 
