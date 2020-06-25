
#ifndef __creatures_H__
#define __creatures_H__

#include "ship.h"

#define CREATURE_X_POS SCREEN_WIDTH*4/8
#define CREATURE_Y_POS SCREEN_HEIGHT*50/100

#define SPEED 5

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

typedef struct creature_t{
    classes_t CreatureType;
    creatureIDS_t CreatureID;

    signed short x_pos;
    signed short y_pos;
    
    
    unsigned char Alive;
    Position_t Position;

    signed short speed;
    bullet_t* bullet;

}creature_t;

creature_t* CreateCreatures();
creature_t CreateSingleCreature(signed short x_pos, signed short y_pos,
                           classes_t CreatureType, creatureIDS_t ID);

signed char xCheckCreatureCollision(signed short x_pos, signed short y_pos,
                                     creature_t* creature);

void vUpdateCreatureStatus(creature_t* creature, unsigned char ID);
void vAlternateAnimation(creature_t* creature);
#endif 

