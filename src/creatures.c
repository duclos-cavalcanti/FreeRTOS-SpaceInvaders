#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "TUM_Draw.h"
#include "creatures.h"
#include "main.h"
#include "ship.h"

#define NUMB_OF_CREATURES 2

creature_t* CreateCreatures()
{
    creature_t* CreatureArray = calloc(NUMB_OF_CREATURES, sizeof(creature_t));

    if(!CreatureArray){
        fprintf(stderr, "Failed to creature arrays.");
        exit(EXIT_FAILURE);
    }

    unsigned short i=0; 

    while(i<NUMB_OF_CREATURES){
        CreatureArray[i]=CreateSingleCreature(SCREEN_WIDTH/2 + i*40,SCREEN_HEIGHT/2,
                                              MEDIUM, i);
        ++i;
    }

    return CreatureArray;

}

creature_t CreateSingleCreature(signed short x_pos, signed short y_pos,
                           classes_t CreatureType, creatureIDS_t ID)
{
    creature_t creature; 

    creature.x_pos=x_pos;
    creature.y_pos=y_pos;
    creature.speed=SPEED;
    creature.Alive=1;
    creature.CreatureType=CreatureType;
    creature.CreatureID=ID;
    creature.Position=Position0;

    return creature;
}

signed char xCheckCreatureCollision(signed short x_pos, signed short y_pos,
                                     creature_t* creature)
{
    signed short LEFT_LIMIT = creature->x_pos - CREATURE_WIDTH/2;
    signed short RIGHT_LIMIT = creature->x_pos + CREATURE_WIDTH/2;

    signed short LOWER_LIMIT = creature->y_pos + CREATURE_HEIGHT/2;

    if(LEFT_LIMIT <= x_pos && x_pos <= RIGHT_LIMIT)
        if(y_pos - SHIP_BULLET_SPEED <= LOWER_LIMIT)
            return creature->CreatureID;
        else return -1;
    else
        return -1;
}

void vUpdateCreatureStatus(creature_t* creature, unsigned char creatureID)
{
   creature->Alive=0; 
}

void vAlternateAnimation(creature_t* creature)
{
    if(creature->Position == Position0) creature->Position=Position1;
    else creature->Position=Position0;
}

