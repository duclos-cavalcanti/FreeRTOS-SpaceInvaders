#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "ship.h"


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
