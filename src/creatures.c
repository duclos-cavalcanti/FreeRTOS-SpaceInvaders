#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "TUM_Draw.h"
#include "creatures.h"
#include "main.h"
#include "ship.h"


creature_t* CreateCreatures()
{
    creature_t* CreatureArray = calloc(NUMB_OF_CREATURES, sizeof(creature_t));
    if(!CreatureArray){
        fprintf(stderr, "Failed to creature arrays.");
        exit(EXIT_FAILURE);
    }

    unsigned short CreatureCountID=0; 
    unsigned short CreatureDistance_X=0;

    while(CreatureCountID<NUMB_OF_CREATURES){
        CreatureDistance_X = CREATURE_X_DIST_APART*CreatureCountID;
        CreatureArray[CreatureCountID]=CreateSingleCreature(CREATURE_X_ROW_BEGIN + CreatureDistance_X,
                                                            CREATURE_Y_ROW_BEGIN,
                                                            MEDIUM, CreatureCountID);
        ++CreatureCountID;
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

signed char xCheckCreaturesCollision(creature_t creatures[],
                                     signed short bullet_x_pos,
                                     signed short bullet_y_pos)
{   
    unsigned char CreatureCount=0;
    signed  char CreatureCollisionID=0;

    while(CreatureCount<NUMB_OF_CREATURES){
       CreatureCollisionID=xCheckSingleCreatureCollision(bullet_x_pos,
                                                         bullet_y_pos,
                                                         &creatures[CreatureCount]);

       if(CreatureCollisionID>=0) return CreatureCollisionID;
       ++CreatureCount;
    }

    return -1;
}

signed char xCheckSingleCreatureCollision(signed short bullet_x, signed short bullet_y,
                                          creature_t* creature)
{
    signed short LEFT_LIMIT = creature->x_pos - CREATURE_WIDTH/2 + SHIP_BULLET_THICKNESS/2;
    signed short RIGHT_LIMIT = creature->x_pos + CREATURE_WIDTH/2 + SHIP_BULLET_THICKNESS/2;
    signed short LOWER_LIMIT = creature->y_pos + CREATURE_HEIGHT/2;

    if(LEFT_LIMIT <= bullet_x && bullet_x <= RIGHT_LIMIT)
        if(bullet_y <= LOWER_LIMIT)
            return creature->CreatureID;
        else return -1;
    else
        return -1;
}

void vKillCreature(creature_t* creature, unsigned char creatureID)
{
   creature->Alive=0; 
}

void vAlternateAnimation(creature_t* creature)
{
    if(creature->Position == Position0) creature->Position=Position1;
    else creature->Position=Position0;
}

unsigned char xFetchCreatureValue(unsigned char creatureclassID)
{
    switch(creatureclassID){
        case EASY:
            return 10;
            break;
        case MEDIUM:
            return 20;
            break;
        case HARD:
            return 30;
            break;
        case NONEXISTENT_CLASS:
        default:
            return 0;
    }
}
