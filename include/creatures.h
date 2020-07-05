
#ifndef __creatures_H__
#define __creatures_H__

#include "ship.h"
#include "main.h"

#define NUMB_OF_ROWS 5
#define NUMB_OF_COLUMNS 8
#define NUMB_OF_CREATURES NUMB_OF_ROWS*NUMB_OF_COLUMNS

#define CREATURE_SPEED 1
#define CREATURE_BULLET_SPEED 8
#define CREAT_BULLET_THICKNESS 3

#define CREATURE_X_POS SCREEN_WIDTH*4/8
#define CREATURE_Y_POS SCREEN_HEIGHT*50/100

#define CREATURE_Y_ROW_BEGIN SCREEN_HEIGHT*1/2
#define CREATURE_X_ROW_BEGIN SCREEN_WIDTH*1/6
#define CREATURE_X_DIST_APART 40
#define CREATURE_Y_DIST_APART 40

#define CREATURE_MIN_DIST_WALL SCREEN_WIDTH - CREATURE_WIDTH/2


typedef enum creatureIDS_t{
    CreatureONE,
    CreatureTWO,
    CreatureTHREE,
    CreatureFOUR,
    CreatureFIVE,
    CreatureSIX,
    CreatureSEVEN,
    CreatureEIGHT,
    CreatureNINE,
    CreatureTEN,
    CreatureELEVEN,
    CreatureTWELVE,
    CreatureTHIRTEEN,
    CreatureFOURTEEN,
    CreatureFIFTEEN,
    CreatureSIXTEEN,
    CreatureSEVENTEEEN,
    CreatureEIGHTEEN,
    CreatureNINETEEN,
    CreatureTWENTY,
    CreatureTWENTYONE,
    CreatureTWENTYTWO,
    CreatureTWENTYTHREE,
    CreatureTWENTYFOUR,
    CreatureTWENTYFIVE,
    CreatureTWENTYSIX,
    CreatureTWENTYSEVEN,
    CreatureTWENTYEIGHT,
    CreatureTWENTYNINE,
    CreatureTHIRTY,
    CreatureTHIRTYONE,
    CreatureTHIRTYTWO,
    CreatureTHIRTYTHREE,
    CreatureTHIRTYFOUR,
    CreatureTHIRTYFIVE,
    CreatureTHIRTYSIX,
    CreatureTHIRTYSEVEN,
    CreatureTHIRTYEIGHT,
    CreatureTHIRTYNINE,
    CreatureFOURTY
}creatureIDS_t;

typedef enum Position_t{
    Position0,
    Position1
}Position_t;

typedef enum classes_t{
    NONEXISTENT_CLASS,
    EASY,
    MEDIUM,
    HARD
}classes_t;

typedef enum H_Movement_t{
    RIGHT,
    LEFT
}H_Movement_t;

typedef struct creature_t{
    classes_t CreatureType;
    creatureIDS_t CreatureID;

    signed short x_pos;
    signed short y_pos;
    signed short speed;
    unsigned char Alive;

    Position_t Position;
    image_handle_t Image_0;
    image_handle_t Image_1;
}creature_t;



creature_t* CreateCreatures();
signed short*  vAssignFrontierCreatures(creature_t* creatures);
void vUpdateFrontierCreaturesIDs(signed short* FrontierCreaturesID, unsigned char CreatureHitID);

signed char xCheckCreaturesCollision(creature_t* creatures,
                                     signed short bullet_x_pos,
                                     signed short bullet_y_pos,
                                     H_Movement_t Direction,
                                     signed short* FrontierCreaturesID);

void vMoveCreaturesHorizontal(creature_t* creature, H_Movement_t* DIRECTION);
void vMoveCreaturesVerticalDown(creature_t* creatures);

unsigned char xCheckCreaturesTouchBunkers(creature_t* creatures,
                                          signed short* FrontierCreaturesID,
                                          bunkers_t* Bunkers);

unsigned char xCheckFrontierReachedBottom(creature_t* creatures,
                                          signed short* FrontierCreaturesID);

void vUpdateCreaturesSpeed(creature_t* Creatures);





void vCreateCreaturesBullet(creature_t* creatures, 
                            bullet_t* CreaturesBullet,
                            signed short* FrontierCreaturesID);


void vDrawCreaturesBullet(bullet_t* CreatureBullets);

void vUpdateCreaturesBulletPos(bullet_t* CreaturesBullet);

unsigned char xCheckCreaturesBulletCollisonBottomWall(signed short y_pos);


unsigned char xCheckCreaturesBulletShipCollision(signed short x_pos,
                                                 signed short y_pos,
                                                 ship_t* ship);

void vKillCreature(creature_t* creature, unsigned short* NumbOfAliveCreatures);




void vAlternateAnimation(creature_t* creature);
unsigned char xFetchCreatureValue(unsigned char creatureclassID);
#endif 

