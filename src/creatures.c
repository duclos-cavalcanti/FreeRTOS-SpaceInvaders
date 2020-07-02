#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "TUM_Draw.h"
#include "creatures.h"
#include "ship.h"

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
        CreaturesYcoords[i]=CREATURE_Y_ROW_BEGIN + i*CREATURE_Y_DIST_APART;

    return CreaturesYcoords;
}

int* SetTypes()
{
    int* CreaturesTypes = calloc(NUMB_OF_ROWS, sizeof(int));

    CreaturesTypes[0]=EASY;
    CreaturesTypes[1]=MEDIUM;
    return CreaturesTypes;
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
bullet_t CreateCreateSingleBullet(creature_t* creature)
{
    bullet_t CreatureBullet;

    CreatureBullet.x_pos=creature->x_pos;
    CreatureBullet.y_pos=creature->y_pos + CREATURE_HEIGHT/2;
    CreatureBullet.speed=CREATURE_BULLET_SPEED;
    CreatureBullet.BulletAliveFlag=1;

    return CreatureBullet;
}

signed char xCheckCreaturesCollision(creature_t* creatures,
                                 signed short bullet_x_pos,
                                 signed short bullet_y_pos)
{   
    unsigned char CreatureCount=0;
    signed  char CreatureCollisionID=0;

    while(CreatureCount<NUMB_OF_CREATURES){
        if(creatures[CreatureCount].Alive==1){
            CreatureCollisionID=xCheckSingleCreatureCollision(bullet_x_pos,
                                                              bullet_y_pos,
                                                              &creatures[CreatureCount]);

            if(CreatureCollisionID>=0) return CreatureCollisionID;
        }
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
    signed short UPPER_LIMIT = creature->y_pos - CREATURE_HEIGHT/2;

    if(LEFT_LIMIT <= bullet_x && bullet_x <= RIGHT_LIMIT)
        if(bullet_y <= LOWER_LIMIT && bullet_y >= UPPER_LIMIT)
            return creature->CreatureID;
        else return -1;
    else
        return -1;
}

void vKillCreature(creature_t* creature, unsigned short* NumbOfAliveCreatures)
{
    creature->Alive=0; 
    (*NumbOfAliveCreatures)--;
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

unsigned char xCheckLeftEdgeDistance(signed short x_pos)
{
    if(x_pos - CREATURE_SPEED <=  CREATURE_WIDTH/2) return 1;
    else return 0;
}

unsigned char xCheckRightEdgeDistance(signed short x_pos)
{
    if(x_pos + CREATURE_SPEED >= CREATURE_MIN_DIST_WALL) return 1;
    else return 0;
}

H_Movement_t xCheckDirectionOfRow(creature_t* creatures,H_Movement_t DIRECTION, Rows_t Row)
{ 
    signed short creature_count;
    signed short ROW_END = NUMB_OF_COLUMNS - 1 + (Row)*NUMB_OF_COLUMNS;
    signed short ROW_BEGIN = 0 + (Row)*(NUMB_OF_COLUMNS); 

    if(DIRECTION == RIGHT){
        creature_count=ROW_END;        
        while(creature_count >= ROW_BEGIN){
            if(creatures[creature_count].Alive==1){
                if(xCheckRightEdgeDistance(creatures[creature_count].x_pos))
                    return LEFT;
                else 
                    return RIGHT;
                }

            --creature_count;
            } 
        }
    else if(DIRECTION == LEFT){ 
        creature_count = ROW_BEGIN;
        while(creature_count<ROW_END){  
            if(creatures[creature_count].Alive==1){
                if(xCheckLeftEdgeDistance(creatures[creature_count].x_pos))
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

void vMoveCreaturesRowLeftHorizontal(creature_t* creatures, Rows_t Row)
{
    signed short ROW_END = NUMB_OF_COLUMNS - 1 + (Row)*NUMB_OF_COLUMNS;
    signed short ROW_BEGIN = 0 + (Row)*(NUMB_OF_COLUMNS); 
    unsigned char CreatureCountID = ROW_BEGIN;

    while(CreatureCountID<=ROW_END){
        if(creatures[CreatureCountID].Alive==1)
            vMoveSingleCreatureLeftHorizontal(&creatures[CreatureCountID]);
        ++CreatureCountID;
    }

}

void vMoveCreaturesRowRightHorizontal(creature_t* creatures, Rows_t Row)

{
    signed short ROW_END = NUMB_OF_COLUMNS - 1 + (Row)*NUMB_OF_COLUMNS;
    signed short ROW_BEGIN = 0 + (Row)*(NUMB_OF_COLUMNS);
    unsigned char CreatureCountID = ROW_BEGIN;

    while(CreatureCountID<=ROW_END){
        if(creatures[CreatureCountID].Alive==1)
            vMoveSingleCreatureRightHorizontal(&creatures[CreatureCountID]);
        ++CreatureCountID;
    }
}

void vMoveCreaturesRowHorizontal(creature_t* creatures, H_Movement_t* DIRECTION_OF_ROW, Rows_t Row)
{ 
    (*DIRECTION_OF_ROW) = xCheckDirectionOfRow(creatures, (*DIRECTION_OF_ROW), Row);

    if((*DIRECTION_OF_ROW) == RIGHT)
        vMoveCreaturesRowRightHorizontal(creatures, Row);
    else 
        vMoveCreaturesRowLeftHorizontal(creatures, Row);
}

void vMoveCreaturesHorizontal(creature_t* creatures, H_Movement_t* DIRECTIONS_ARRAY)
{
    vMoveCreaturesRowHorizontal(creatures, 
                                &DIRECTIONS_ARRAY[Row_1], 
                                Row_1);

    vMoveCreaturesRowHorizontal(creatures, 
                                &DIRECTIONS_ARRAY[Row_2], 
                                Row_2);
}

unsigned char xCheckCreatureChoice(creature_t* creatures, unsigned char ChoiceID)
{
   if(creatures[ChoiceID].Alive == 1)
       return 1;
   else
       return 0;
}

unsigned int xChooseCreature(creature_t* creatures)
{
    unsigned int Choice=0;
    unsigned short ChoiceMade=0;

    while(ChoiceMade==0){
        Choice = rand()%NUMB_OF_COLUMNS;

        if(xCheckCreatureChoice(creatures, Choice))
            ChoiceMade=1;
    }
    
    return Choice;
}
void vCreateCreaturesBullet(creature_t* creatures, 
                             bullet_t* CreatureBullet)
{
    unsigned int CreatureChoiceID=0;
    CreatureChoiceID = xChooseCreature(creatures);
    (*CreatureBullet)=CreateCreateSingleBullet(&creatures[CreatureChoiceID]);
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
