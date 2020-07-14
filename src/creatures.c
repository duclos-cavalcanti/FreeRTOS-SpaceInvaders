/** Basic includes */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/** Game related */
#include "TUM_Draw.h"
#include "creatures.h"
#include "ship.h"
#include "utilities.h"

int* SetXcoords()//Creates and assigns an array that holds the x coordinates of each column of creatures
{
    int* CreaturesXcoords = calloc(NUMB_OF_COLUMNS, sizeof(int));

    for(int i=0;i<NUMB_OF_COLUMNS;i++)
        CreaturesXcoords[i]=CREATURE_X_ROW_BEGIN + i*CREATURE_X_DIST_APART;

    return CreaturesXcoords;
}

int* SetYcoords()//Creates and assigns an array that holds the y coordinates of each row of creatures
{
    int* CreaturesYcoords = calloc(NUMB_OF_ROWS, sizeof(int));

    for(int i=0;i<NUMB_OF_ROWS;i++)
        CreaturesYcoords[i]=CREATURE_Y_ROW_BEGIN - i*CREATURE_Y_DIST_APART;

    return CreaturesYcoords;
}

int* SetTypes()//Creates and assings to an array that holds what level type each row of creatures will consist of
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
    creature.CreatureType=CreatureType; //Difficulty Level
    creature.CreatureID=ID; //Simply the index in which this creature is stored in the array of structs that hold all creatures
    creature.Position=Position0;

    return creature;
}

creature_t* CreateCreatures()
{
    creature_t* CreatureArray = calloc(NUMB_OF_CREATURES, sizeof(creature_t)); //Numb of creatures = rows*columns which will be 40
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
            CreatureArray[CreatureCountID]=CreateSingleCreature(CreaturesX[j], //defined in SetXcoords
                                                                CreaturesY[i], //defined in SetYcoords
                                                                CreaturesTYPES[i], //defined in SetTypes
                                                                CreatureCountID); //Index of array
            ++CreatureCountID;
        }
    }

    return CreatureArray;
}

void vAssignFrontierCreatures(signed short FrontierCreaturesID[8])
{
   for(int i=0;i<8;i++)
       FrontierCreaturesID[i]=i;
}

unsigned char xCheckKilledCreatureWithinFrontier(unsigned char CreatureCollisionID, signed short* FrontierCreaturesID)
{
    for(int i=0;i<NUMB_OF_COLUMNS;++i){
        if(FrontierCreaturesID[i]==CreatureCollisionID) 
                return 1;
    }

    return 0;
}

/**
 *@brief Looks for alive creatures behind the one associated to the CreatureHitID that has been killed
 *
 *Checks if the creature "behind" the killed one is alive and keeps 
 *checking until either finding an alive creature at rom above and then returning how many rows above it is
 *or either finding out that the column has been vanquished, which in thi case will end in a return -1,
 *signaling the calling function that this column is now entirely dead.
 *
 *@param creatures array of structs that hold all creature instances and their current information
 *@param CreatureHitID integer that shows the index/id of the killed creature
 *@param Row an enum integer that tells me which Row the killed creature finds itslef
 */
signed char xCheckCreatureBehindAlive(creature_t* creatures, 
                                      unsigned char CreatureHitID,
                                      Rows_t Row)
{
    switch(Row){
        case Row_1:
            if(creatures[CreatureHitID + 8].Alive == 1)
                return 1;
            if(creatures[CreatureHitID + 16].Alive == 1)
                return 2;
            if(creatures[CreatureHitID + 24].Alive == 1)
                return 3;
            if(creatures[CreatureHitID + 32].Alive == 1)
                return 4;
            return -1;
            break; 
        case Row_2:
            if(creatures[CreatureHitID + 8].Alive == 1)
                return 1;
            if(creatures[CreatureHitID + 16].Alive == 1)
                return 2;
            if(creatures[CreatureHitID + 24].Alive == 1)
                return 3;
            return -1;
            break; 
        case Row_3:
            if(creatures[CreatureHitID + 8].Alive == 1)
                return 1;
            if(creatures[CreatureHitID + 16].Alive == 1)
                return 2;
            return -1;
            break;   
        case Row_4:
            if(creatures[CreatureHitID + 8].Alive == 1)
                return 1;
            return -1;
            break;   
        default:
            return -1;
            break;   
    }
}

//Updates the values within the FrontierCreaturesID array
void vUpdateFrontierCreaturesIDs(signed short* FrontierCreaturesID, 
                                 unsigned char CreatureHitID,
                                 creature_t* creatures)
{
    signed char StepDifference=0; 

    if(CreatureHitID < NUMB_OF_COLUMNS){ //if within row 1
        
        //finds out the step difference/how many rows above is the next alive creature
        StepDifference = xCheckCreatureBehindAlive(creatures,
                                                   CreatureHitID,
                                                   Row_1);
        if(StepDifference>0) 
            FrontierCreaturesID[CreatureHitID] += 8 * StepDifference;
        else //Entire column is dead
            FrontierCreaturesID[CreatureHitID] = -1;
    }
    else if(CreatureHitID < NUMB_OF_COLUMNS*2){ //if within row 2
        StepDifference = xCheckCreatureBehindAlive(creatures,
                                                   CreatureHitID,
                                                   Row_2);
        if(StepDifference>0) //if within row 3
            FrontierCreaturesID[CreatureHitID - 8] += 8 * StepDifference;
        else 
            FrontierCreaturesID[CreatureHitID - 8] = -1;
    }
    else if(CreatureHitID < NUMB_OF_COLUMNS*3){ //if within row 3
        StepDifference = xCheckCreatureBehindAlive(creatures,
                                                   CreatureHitID,
                                                   Row_3);
        if(StepDifference>0)
            FrontierCreaturesID[CreatureHitID -16] += 8 * StepDifference;
        else 
            FrontierCreaturesID[CreatureHitID -16] = -1;
    }
    else if(CreatureHitID < NUMB_OF_COLUMNS*4){ //if within row 4
        StepDifference = xCheckCreatureBehindAlive(creatures,
                                                   CreatureHitID,
                                                   Row_4);
        if(StepDifference>0)
            FrontierCreaturesID[CreatureHitID - 24] += 8 * StepDifference;
        else 
            FrontierCreaturesID[CreatureHitID - 24] = -1;
    }
    else{ //if within row 5 
        FrontierCreaturesID[CreatureHitID - 32] = -1; //there is no row above row 5 -> Column is vanquished
    }
}

signed char xCheckSingleCreatureCollision(signed short bullet_x, signed short bullet_y,
                                          H_Movement_t Direction, creature_t* creature)
{
    signed short R_OFFSET = 0;
    signed short L_OFFSET = 0;

    if(Direction == RIGHT){ //Meaning creture is currently moving to the tight
        R_OFFSET = creature->speed; //take into account its speed to know if bullet could hit it in the next frame   
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
            return creature->CreatureID;//return the ID of the hit/to-be-hit creature
        else return -1;
    else
        return -1; //No hit
}

signed char xCheckCreaturesCollision(creature_t* creatures,
                                     signed short bullet_x_pos,
                                     signed short bullet_y_pos,
                                     H_Movement_t Direction)
{   
    signed  char CreatureCollisionID=0;

    for(int i=0;i<NUMB_OF_CREATURES;++i){ 
        if(creatures[i].Alive == 1){
            CreatureCollisionID=xCheckSingleCreatureCollision(bullet_x_pos,
                                                              bullet_y_pos,
                                                              Direction,
                                                              &creatures[i]);

            if(CreatureCollisionID>=0) //A creature hit was found
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
            BunkerCollisionID=xCheckSingleCreatureBunkerCollision(creatures[CreatureID].x_pos, //Checks for bunker collision
                                                                  creatures[CreatureID].y_pos + CREATURE_HEIGHT/2,
                                                                  Bunkers);


            if(BunkerCollisionID>0) 
                return BunkerCollisionID;
        }
    }
    return 0;
}

unsigned char xCheckSingleCreatureReachedBottom(creature_t creature)
{
    if(creature.y_pos + CREATURE_HEIGHT/2 + 10 >= PLAYERSHIP_Y_BEGIN - PLAYERSHIP_HEIGHT/2)
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
        if(CreatureID >= 0 && creatures[CreatureID].Alive == 1){ //Checking if the to-be-checked creature is in fact alive or if the column is already dead
            CreatureReachedBottomID = xCheckSingleCreatureReachedBottom(creatures[CreatureID]);

            if(CreatureReachedBottomID>0) //We have a hit, game over
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

    if(DIRECTION == RIGHT){ //if current direction is right
        creature_count=ROW_END; //Begin checking by the right-most creatures position 
        while(creature_count >= ROW_BEGIN){
            if(creatures[creature_count].Alive==1 || //only skip checking edge distance if entire column is dead
               creatures[creature_count+8].Alive==1 ||
               creatures[creature_count+16].Alive==1 ||
               creatures[creature_count+24].Alive==1  ||
               creatures[creature_count+32].Alive==1){

                if(xCheckRightEdgeDistance(creatures[creature_count]) || //Check for all currently-iterated creatures if any one of them are at the right edge
                   xCheckRightEdgeDistance(creatures[creature_count+8]) ||
                   xCheckRightEdgeDistance(creatures[creature_count+16]) ||
                   xCheckRightEdgeDistance(creatures[creature_count+24]) ||
                   xCheckRightEdgeDistance(creatures[creature_count+32]))

                    return LEFT; //If any one of them returns positively we must change the creatures block direction
                else 
                    return RIGHT;
                }

            --creature_count; //Look at the to-the-left column
            } 
        }
    else if(DIRECTION == LEFT){ //if current direction is left
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

unsigned char xCheckDirectionChange(H_Movement_t* LastDirection, H_Movement_t CurrentHorizontalDirection)
{
   if((*LastDirection)!=CurrentHorizontalDirection)
       return 1;
   else
       return 0;
}
void vMoveCreaturesHorizontal(creature_t* creatures, H_Movement_t* DIRECTION)
{
    (*DIRECTION) = xCheckDirectionOfRows(creatures, (*DIRECTION)); //finds out the newest possible direction of the creatures block

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

void xChooseShootingCreature(signed short* FrontierCreaturesID, creature_t* creatures, signed short* Choice)
{
    unsigned short i = 0;
    unsigned char LeaveLoop=0;
    signed short PreliminaryChoice=0;

    while(LeaveLoop == 0){
        PreliminaryChoice = FrontierCreaturesID[rand()%NUMB_OF_COLUMNS]; //Chooses a randomn column from the FrontierCreaturesID array
        if(PreliminaryChoice >=0 && creatures[PreliminaryChoice].Alive == 1){ //check if the value stored there is negative, meaning column is vanquished or if the current choice is of an alive creature (double-checking)
            (*Choice) = PreliminaryChoice;
            LeaveLoop = 1;
        }
        else
            ++i;
    }
}

void vCreateCreaturesBullet(creature_t* creatures, 
                            bullet_t* CreaturesBullet,
                            signed short* FrontierCreaturesID)

{
    signed short CreatureChoiceID = 0;
    xChooseShootingCreature(FrontierCreaturesID, creatures, &CreatureChoiceID);
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
                          CreatureBullet->y_pos+CREAT_BULLET_LENGTH,
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

void vRetrieveDeadCreatureXY(signed short* x, signed short* y, creature_t creature)
{
    (*x)=creature.x_pos;
    (*y)=creature.y_pos;
}
