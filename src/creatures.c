#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "TUM_Draw.h"
#include "creatures.h"
#include "ship.h"
#include "utilities.h"

int* SetXcoords()
{
    int* CreaturesXcoords = calloc(NUMB_OF_COLUMNS, sizeof(int));

    for(int i=0;i<NUMB_OF_COLUMNS;i++)
        CreaturesXcoords[i]=CREATURE_X_ROW_BEGIN + i*CREATURE_X_DIST_APART;

    return CreaturesXcoords;
}

int* SetYcoords()
{
    int* CreaturesYcoords = calloc(NUMB_OF_ROWS, sizeof(int));

    for(int i=0;i<NUMB_OF_ROWS;i++)
        CreaturesYcoords[i]=CREATURE_Y_ROW_BEGIN - i*CREATURE_Y_DIST_APART;

    return CreaturesYcoords;
}

int* SetTypes()
{
    int* CreaturesTypes = calloc(NUMB_OF_ROWS, sizeof(int));

    CreaturesTypes[0]=EASY;
    CreaturesTypes[1]=EASY;
    CreaturesTypes[2]=MEDIUM;
    CreaturesTypes[3]=MEDIUM;
    CreaturesTypes[4]=HARD;

    return CreaturesTypes;
}

creature_t CreateSingleCreature(signed short x_pos, signed short y_pos,
                                classes_t CreatureType, creatureIDS_t ID)
{
    creature_t creature; 

    creature.x_pos=x_pos;
    creature.y_pos=y_pos;
    creature.speed=CREATURE_SPEED;
    creature.Alive=1;
    creature.CreatureType=CreatureType;
    creature.CreatureID=ID;
    creature.Position=Position0;

    return creature;
}

creature_t* CreateCreatures()
{
    creature_t* CreatureArray = calloc(NUMB_OF_CREATURES, sizeof(creature_t));
    if(!CreatureArray){
        fprintf(stderr, "Failed to creature arrays.");
        exit(EXIT_FAILURE);
    }

    unsigned short CreatureCountID=0; 
    int* CreaturesX=SetXcoords();
    int* CreaturesY=SetYcoords();
    int* CreaturesTYPES=SetTypes();

    for(int i=0;i<NUMB_OF_ROWS;i++){
        for(int j=0;j<NUMB_OF_COLUMNS;j++){ 
            CreatureArray[CreatureCountID]=CreateSingleCreature(CreaturesX[j],
                                                                CreaturesY[i],
                                                                CreaturesTYPES[i], 
                                                                CreatureCountID);
            ++CreatureCountID;
        }
    }

    return CreatureArray;
}

signed short* vAssignFrontierCreatures(creature_t* creatures)
{
   signed short* FrontierCreaturesID = calloc(8,sizeof(signed short));
   if(!FrontierCreaturesID){
       fprintf(stderr,"Unable to create Frontier creatures ID.");
       exit(EXIT_FAILURE);
   }

   for(int i=0;i<8;i++)
       FrontierCreaturesID[i]=i;


   return FrontierCreaturesID;
}

void vUpdateFrontierCreaturesIDs(signed short* FrontierCreaturesID, unsigned char CreatureHitID)
{
    if(CreatureHitID < NUMB_OF_COLUMNS)
        FrontierCreaturesID[CreatureHitID] += 8;
    else if(CreatureHitID < NUMB_OF_COLUMNS*2)
        FrontierCreaturesID[CreatureHitID - 8] += 8;
    else if(CreatureHitID < NUMB_OF_COLUMNS*3)
        FrontierCreaturesID[CreatureHitID - 16] += 8;
    else if(CreatureHitID < NUMB_OF_COLUMNS*4)
        FrontierCreaturesID[CreatureHitID - 24] += 8;
    else
        FrontierCreaturesID[CreatureHitID - 32] = -1;

}

signed char xCheckSingleCreatureCollision(signed short bullet_x, signed short bullet_y,
                                          H_Movement_t Direction, creature_t* creature)
{
    signed short R_OFFSET = 0;
    signed short L_OFFSET = 0;

    if(Direction == RIGHT){
        R_OFFSET = creature->speed;    
        L_OFFSET = 0;
    }
    else{
        L_OFFSET = creature->speed;    
        R_OFFSET = 0;
    }

    signed short LEFT_LIMIT = creature->x_pos - CREATURE_WIDTH/2 - SHIP_BULLET_THICKNESS/2 - L_OFFSET;
    signed short RIGHT_LIMIT = creature->x_pos + CREATURE_WIDTH/2 + SHIP_BULLET_THICKNESS/2 + R_OFFSET;

    signed short LOWER_LIMIT = creature->y_pos + CREATURE_HEIGHT/2;
    signed short UPPER_LIMIT = creature->y_pos - CREATURE_HEIGHT/2;

    if(LEFT_LIMIT <= bullet_x && bullet_x <= RIGHT_LIMIT)
        if(bullet_y <= LOWER_LIMIT && bullet_y >= UPPER_LIMIT)
            return creature->CreatureID;
        else return -1;
    else
        return -1;
}

signed char xCheckCreaturesCollision(creature_t* creatures,
                                     signed short bullet_x_pos,
                                     signed short bullet_y_pos,
                                     H_Movement_t Direction,
                                     signed short* FrontierCreaturesID)
{   
    signed  char CreatureID=0;
    signed  char CreatureCollisionID=0;

    for(int i=0;i<NUMB_OF_COLUMNS;++i){ 
        CreatureID=FrontierCreaturesID[i];
        if(CreatureID>=0){
            CreatureCollisionID=xCheckSingleCreatureCollision(bullet_x_pos,
                                                              bullet_y_pos,
                                                              Direction,
                                                              &creatures[CreatureID]);

            if(CreatureCollisionID>=0) 
                return CreatureCollisionID;
        }
    }
    return -1;
}

unsigned char xCheckCreaturesTouchBunkers(creature_t* creatures,
                                          signed short* FrontierCreaturesID,
                                          bunkers_t* Bunkers)
{
    signed short CreatureID=0;
    unsigned short BunkerCollisionID = 0;

    for(int i=0;i<NUMB_OF_COLUMNS;++i){
        CreatureID=FrontierCreaturesID[i];
        if(CreatureID>=0){
            BunkerCollisionID=xCheckSingleCreatureBunkerCollision(creatures[CreatureID].x_pos,
                                                                  creatures[CreatureID].y_pos + CREATURE_WIDTH/2,
                                                                  Bunkers);


            if(BunkerCollisionID>0) 
                return BunkerCollisionID;
        }
    }
    return 0;
}

unsigned char xCheckSingleCreatureReachedBottom(creature_t creature)
{
    if(creature.y_pos + CREATURE_HEIGHT/2 >= PLAYERSHIP_Y_BEGIN - PLAYERSHIP_HEIGHT/2)
        return 1;
    else
        return 0;
}

unsigned char xCheckFrontierReachedBottom(creature_t* creatures,
                                  signed short* FrontierCreaturesID)
{
    signed short CreatureID=0;
    unsigned short CreatureReachedBottomID = 0;

    for(int i=0;i<NUMB_OF_COLUMNS;++i){
        CreatureID = FrontierCreaturesID[i];
        if(CreatureID > 0){
            CreatureReachedBottomID = xCheckSingleCreatureReachedBottom(creatures[CreatureID]);

            if(CreatureReachedBottomID>0)
                return CreatureReachedBottomID;
        }
    }
    return 0;
}

void vKillCreature(creature_t* creature, unsigned short* NumbOfAliveCreatures)
{
    creature->Alive=0; 
    (*NumbOfAliveCreatures)--;
}

unsigned char xCheckLeftEdgeDistance(creature_t creature)
{
    if(creature.Alive == 1)
        if(creature.x_pos - creature.speed <=  CREATURE_WIDTH/2) 
            return 1;
    return 0;
}

unsigned char xCheckRightEdgeDistance(creature_t creature)
{
    if(creature.Alive==1)
        if(creature.x_pos + creature.speed >= CREATURE_MIN_DIST_WALL) 
            return 1;
    return 0;
}

H_Movement_t xCheckDirectionOfRows(creature_t* creatures,H_Movement_t DIRECTION)
{ 
    signed short creature_count;
    signed short ROW_END = NUMB_OF_COLUMNS - 1;
    signed short ROW_BEGIN = 0; 

    if(DIRECTION == RIGHT){
        creature_count=ROW_END;        
        while(creature_count >= ROW_BEGIN){
            if(creatures[creature_count].Alive==1 ||
               creatures[creature_count+8].Alive==1 ||
               creatures[creature_count+16].Alive==1 ||
               creatures[creature_count+24].Alive==1  ||
               creatures[creature_count+32].Alive==1){

                if(xCheckRightEdgeDistance(creatures[creature_count]) ||
                   xCheckRightEdgeDistance(creatures[creature_count+8]) ||
                   xCheckRightEdgeDistance(creatures[creature_count+16]) ||
                   xCheckRightEdgeDistance(creatures[creature_count+24]) ||
                   xCheckRightEdgeDistance(creatures[creature_count+32]))

                    return LEFT;
                else 
                    return RIGHT;
                }

            --creature_count;
            } 
        }
    else if(DIRECTION == LEFT){ 
        creature_count = ROW_BEGIN;
        while(creature_count<=ROW_END){  
            if(creatures[creature_count].Alive==1 ||
               creatures[creature_count+8].Alive==1 ||
               creatures[creature_count+16].Alive==1 ||
               creatures[creature_count+24].Alive==1 ||
               creatures[creature_count+32].Alive==1){

                if(xCheckLeftEdgeDistance(creatures[creature_count]) ||
                   xCheckLeftEdgeDistance(creatures[creature_count+8]) ||
                   xCheckLeftEdgeDistance(creatures[creature_count+16]) ||
                   xCheckLeftEdgeDistance(creatures[creature_count+24]) ||
                   xCheckLeftEdgeDistance(creatures[creature_count+32]))

                    return RIGHT;
                else 
                    return LEFT;
            }
            ++creature_count;
        }
    }

    return DIRECTION;
}
void vMoveSingleCreatureLeftHorizontal(creature_t* creature)
{
    creature->x_pos-=creature->speed;
}

void vMoveSingleCreatureRightHorizontal(creature_t* creature)
{
    creature->x_pos+=creature->speed;    
}

void vMoveCreaturesLeftHorizontal(creature_t* creatures)
{
    unsigned char CreatureCountID = 0;

    while(CreatureCountID<NUMB_OF_CREATURES){
        if(creatures[CreatureCountID].Alive==1)
            vMoveSingleCreatureLeftHorizontal(&creatures[CreatureCountID]);
        ++CreatureCountID;
    }

}

void vMoveCreaturesRightHorizontal(creature_t* creatures)
{
    unsigned char CreatureCountID = 0;

    while(CreatureCountID<NUMB_OF_CREATURES){
        if(creatures[CreatureCountID].Alive==1)
            vMoveSingleCreatureRightHorizontal(&creatures[CreatureCountID]);
        ++CreatureCountID;
    }
}

void vMoveCreaturesHorizontal(creature_t* creatures, H_Movement_t* DIRECTION)
{
    (*DIRECTION) = xCheckDirectionOfRows(creatures, (*DIRECTION));

    if((*DIRECTION) == RIGHT)
        vMoveCreaturesRightHorizontal(creatures);
    else 
        vMoveCreaturesLeftHorizontal(creatures);
}

void vMoveSingleCreatureVerticalDown(creature_t* creature)
{
    creature->y_pos+=CREATURE_HEIGHT;
}

void vMoveCreaturesVerticalDown(creature_t* creatures)
{
    unsigned char CreatureCountID = 0;

    while(CreatureCountID<NUMB_OF_CREATURES){
        if(creatures[CreatureCountID].Alive==1)
            vMoveSingleCreatureVerticalDown(&creatures[CreatureCountID]);
        ++CreatureCountID;
    }
}


void vUpdateSingleCreaturesSpeed(creature_t* Creature)
{
    Creature->speed++;
}

void vUpdateCreaturesSpeed(creature_t* Creatures)
{
    unsigned short creatureIDcount=0;
    while(creatureIDcount<NUMB_OF_CREATURES){
        if(Creatures[creatureIDcount].Alive==1)
            vUpdateSingleCreaturesSpeed(&Creatures[creatureIDcount]);
        creatureIDcount++;
    }
}

bullet_t CreateCreatureSingleBullet(creature_t* creature)
{
    bullet_t CreatureBullet;

    CreatureBullet.x_pos=creature->x_pos;
    CreatureBullet.y_pos=creature->y_pos + CREATURE_HEIGHT/2;
    CreatureBullet.speed=CREATURE_BULLET_SPEED;
    CreatureBullet.BulletAliveFlag=1;

    return CreatureBullet;
}
void vCreateCreaturesBullet(creature_t* creatures, 
                            bullet_t* CreaturesBullet,
                            signed short* FrontierCreaturesID)

{
    signed int CreatureChoiceID = -1;
    while(CreatureChoiceID<0){
        CreatureChoiceID = FrontierCreaturesID[rand()%NUMB_OF_COLUMNS];
    }

    (*CreaturesBullet)=CreateCreatureSingleBullet(&creatures[CreatureChoiceID]);
}

unsigned char xCheckCreaturesBulletCollisonBottomWall(signed short b_ypos)
{
    if(b_ypos>=BOTTOM_WALLPOSITION - BOTTOM_WALLTHICKNESS/2 - CREATURE_BULLET_SPEED) return 1;
    else return 0;
}


unsigned char xCheckCreaturesBulletShipCollision(signed short b_xpos,
                                                 signed short b_ypos,
                                                 ship_t* ship)
{
    signed short SHIP_LEFT_LIMIT = ship->x_pos - PLAYERSHIP_WIDTH/2; 
    signed short SHIP_RIGHT_LIMIT = ship->x_pos + PLAYERSHIP_WIDTH/2;
    signed short SHIP_TOP_LIMIT = ship->y_pos + PLAYERSHIP_HEIGHT/2;

    if(b_xpos <= SHIP_RIGHT_LIMIT && b_xpos >= SHIP_LEFT_LIMIT)
        if(b_ypos + CREATURE_BULLET_SPEED >= SHIP_TOP_LIMIT)
            return 1;
        else 
            return 0;
    else return 0;
}

void vUpdateCreaturesBulletPos(bullet_t* CreaturesBullet)
{
    CreaturesBullet->y_pos+=CreaturesBullet->speed;
}

void vDrawCreaturesBullet(bullet_t* CreatureBullet)
{
    checkDraw(tumDrawLine(CreatureBullet->x_pos, 
                          CreatureBullet->y_pos,
                          CreatureBullet->x_pos, 
                          CreatureBullet->y_pos+SHIP_BULLET_LENGTH,
                          CREAT_BULLET_THICKNESS,
                          White),
                          __FUNCTION__);
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
