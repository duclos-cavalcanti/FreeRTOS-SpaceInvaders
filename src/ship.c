#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ship.h"
#include "TUM_Draw.h"
#include "main.h"

#define SHIP_BULLET_SPEED 8
#define SHIP_Y_OFFSET 5
#define SHIP_BULLET_THICKNESS 5

ship_t* CreateShip(signed short initial_x, signed short initial_y, signed short speed,
                   unsigned int ShipColor, signed short size)
{

    ship_t* PlayerShip = calloc (1,sizeof(ship_t));

    if(!PlayerShip){
        fprintf(stderr,"Creating Ship failed.\n");
        exit(EXIT_FAILURE);
    }

    PlayerShip->color = ShipColor;
    PlayerShip->radius = size;
    PlayerShip->x_pos = initial_x;
    PlayerShip->y_pos = initial_y;
    PlayerShip->speed = speed;

    return PlayerShip;
}


void vIncrementShipLeft(ship_t* ship)
{
    ship->x_pos-=ship->speed;
}

void vIncrementShipRight(ship_t* ship)
{
    ship->x_pos+=ship->speed;
}


void CreateBullet(ship_t* ship)
{
    
    bullet_t* ShipBullet = calloc(1,sizeof(bullet_t));
    if(!ShipBullet){
        fprintf(stderr, "Creating Ship's Bullet Failed\n");
        exit(EXIT_FAILURE);
    }

    ShipBullet->color = Green;
    ShipBullet->x_pos=ship->x_pos;
    ShipBullet->y_pos=ship->y_pos - SHIP_Y_OFFSET;
    ShipBullet->speed = SHIP_BULLET_SPEED;
    ShipBullet->BulletAliveFlag=1;

    ship->bullet = ShipBullet; 

}

void vDrawShipBullet(ship_t* ship)
{
    checkDraw(tumDrawLine(ship->bullet->x_pos, ship->bullet->y_pos,
                         ship->bullet->x_pos, ship->bullet->y_pos+10,
                          SHIP_BULLET_THICKNESS,Green),
                          __FUNCTION__);
}

void vUpdateShipBulletPos(ship_t* ship)
{
    ship->bullet->y_pos-=ship->bullet->speed;
}

unsigned char xCheckShipBulletLeftScreen(ship_t* ship)
{
    if(ship->bullet->y_pos<=10) return 1;
    else return 0;

}


unsigned char xCheckShipBulletStatus(ship_t* ship)
{
    return xCheckShipBulletLeftScreen(ship);
}
