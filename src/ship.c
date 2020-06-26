#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ship.h"
#include "main.h"
#include "bunkers.h"

#include "TUM_Draw.h"

ship_t* CreateShip(signed short initial_x, signed short initial_y, signed short speed,
                   unsigned int ShipColor, signed short size)
{

    ship_t* PlayerShip = calloc (1,sizeof(ship_t));

    if(!PlayerShip){
        fprintf(stderr,"Creating Ship failed.\n");
        exit(EXIT_FAILURE);
    }

    PlayerShip->size = size;
    PlayerShip->x_pos = initial_x;
    PlayerShip->y_pos = initial_y;
    PlayerShip->speed = speed;
    
    PlayerShip->BunkerCollisionStatus=CreateBunkerCollisionStatus();

    return PlayerShip;
}



BunkerCollisionStatus_t* CreateBunkerCollisionStatus()
{
    BunkerCollisionStatus_t* BunkerCollisionStatus = calloc(1, sizeof(BunkerCollisionStatus_t));

    if(!BunkerCollisionStatus){
        fprintf(stderr,"Creating BCS failed. \n");
        exit(EXIT_FAILURE);
    }
    
    BunkerCollisionStatus->BunkerID = NONEXISTENT_BUNKER;
    BunkerCollisionStatus->HIT = 0;

    return BunkerCollisionStatus;
}
void CreateBullet(ship_t* ship)
{
    
    bullet_t* ShipBullet = calloc(1,sizeof(bullet_t));
    if(!ShipBullet){
        fprintf(stderr, "Creating Ship's Bullet Failed\n");
        exit(EXIT_FAILURE);
    }

    ShipBullet->x_pos=ship->x_pos;
    ShipBullet->y_pos=ship->y_pos - SHIP_Y_OFFSET;
    ShipBullet->speed = SHIP_BULLET_SPEED;
    ShipBullet->BulletAliveFlag=1;

    ship->bullet = ShipBullet; 
}

void vIncrementShipLeft(ship_t* ship)
{
    ship->x_pos-=ship->speed;
}

void vIncrementShipRight(ship_t* ship)
{
    ship->x_pos+=ship->speed;
}

void vUpdateShipBulletPos(ship_t* ship)
{
    ship->bullet->y_pos-=ship->bullet->speed;
}

void vDrawShipBullet(ship_t* ship)
{
    checkDraw(tumDrawLine(ship->bullet->x_pos, ship->bullet->y_pos,
                          ship->bullet->x_pos, ship->bullet->y_pos+SHIP_BULLET_LENGTH,
                          SHIP_BULLET_THICKNESS,Green),
                          __FUNCTION__);
}

unsigned char xCheckShipBulletCollisionTopWall(signed short b_ypos)
{
    if(b_ypos<=10) return 1;
    else return 0;
}

