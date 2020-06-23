#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "creatures.h"
#include "main.h"
#include "ship.h"

creature_t* CreateCreature(signed short x_pos, signed short y_pos,
                           classes_t CreatureType, creatureIDS_t ID)
{
    creature_t* creature = calloc(1,sizeof(creature_t));

    if(!creature){
        fprintf(stderr,"Failed creating creature.\n");
        exit(EXIT_FAILURE);

    }
    creature->x_pos=x_pos;
    creature->y_pos=y_pos;
    creature->speed=SPEED;

    creature->Alive=1;
    creature->CreatureType=CreatureType;
    creature->CreatureID=ID;

    return creature;
}

unsigned char xCheckCreatureCollision(signed short x_pos, signed short y_pos,
                                     creature_t* creature)
{
    signed short LEFT_LIMIT = creature->x_pos - CREATURE_WIDTH/2;
    signed short RIGHT_LIMIT = creature->x_pos + CREATURE_WIDTH/2;

    signed short LOWER_LIMIT = creature->y_pos + CREATURE_HEIGHT/2;

    if(LEFT_LIMIT <= x_pos && x_pos <= RIGHT_LIMIT)
        if(y_pos - SHIP_BULLET_SPEED <= LOWER_LIMIT)
            return 1;
        else return 0;

    else
        return 0; 
}

void vUpdateCreatureStatus(creature_t* creature, unsigned char creatureID)
{
   creature->Alive=0; 
}
