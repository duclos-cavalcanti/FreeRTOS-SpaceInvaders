/** Basic includes */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/** Game related */
#include "ship.h"
#include "creatures.h"
#include "main.h"
#include "bunkers.h"

#include "TUM_Draw.h"

ship_t* CreateShip(signed short initial_x, signed short initial_y, signed short speed,
                   unsigned int ShipColor, signed short size)
{
    ship_t* PlayerShip = calloc (1,sizeof(ship_t));
    bullet_t* PlayerShipBullet = calloc(1, sizeof(bullet_t));

    if(!PlayerShip){
        fprintf(stderr,"Creating Ship failed.\n");
        exit(EXIT_FAILURE);
    }
    if(!PlayerShipBullet){
        fprintf(stderr,"Creating Ships bullet failed.\n");
        exit(EXIT_FAILURE);
    }

    PlayerShip->size = size;
    PlayerShip->x_pos = initial_x;
    PlayerShip->y_pos = initial_y;
    PlayerShip->speed = speed;
    PlayerShip->bullet = PlayerShipBullet;

    return PlayerShip;
}

void CreateShipBullet(ship_t* ship)
{
    ship->bullet->x_pos=ship->x_pos;
    ship->bullet->y_pos=ship->y_pos - SHIP_Y_OFFSET;
    ship->bullet->speed = SHIP_BULLET_SPEED;
    ship->bullet->BulletAliveFlag=1;
    
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
    if(b_ypos<=UPPER_WALL_LIMIT) return 1;
    else return 0;
}

unsigned char xCheckBullet2BulletCollision(signed short ship_bxpos, signed short ship_bypos,
                                           signed short creatures_bxpos, signed short creatures_bypos)
{
    signed short Creature_bullet_LEFT_LIMIT = creatures_bxpos - CREAT_BULLET_THICKNESS; 
    signed short Creature_bullet_RIGHT_LIMIT = creatures_bxpos + CREAT_BULLET_THICKNESS; 
    signed short Creature_bullet_TOP_LIMIT = creatures_bypos + CREAT_BULLET_LENGTH;
    signed short Creature_bullet_BOTTOM_LIMIT = creatures_bypos;

    if(ship_bxpos <= Creature_bullet_RIGHT_LIMIT && ship_bxpos >= Creature_bullet_LEFT_LIMIT){ //If Ship bullet is within creatures bullet left and right limits
        if(ship_bypos >= Creature_bullet_TOP_LIMIT && ship_bypos <= Creature_bullet_BOTTOM_LIMIT) //If Ship bullet is within creatures bullet top and lower limits

            return 1;
       else
            return 0; 
    }
    return 0;
}

