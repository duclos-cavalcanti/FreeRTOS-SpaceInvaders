
#ifndef __creatures_H__
#define __creatures_H__

#include "ship.h"
#include "main.h"

/** Rows and Columns related */
#define NUMB_OF_ROWS 5
#define NUMB_OF_COLUMNS 8
#define NUMB_OF_CREATURES NUMB_OF_ROWS*NUMB_OF_COLUMNS

/** Creature Bullet related */
#define CREATURE_SPEED 10
#define CREATURE_BULLET_SPEED 5
#define CREAT_BULLET_LENGTH 10
#define CREAT_BULLET_THICKNESS 3

/** Creatures initial position related */
#define CREATURE_Y_ROW_BEGIN SCREEN_HEIGHT*1/2
#define CREATURE_X_ROW_BEGIN SCREEN_WIDTH*1/6
#define CREATURE_X_DIST_APART 40
#define CREATURE_Y_DIST_APART 40

//Creature right screen boundary
#define CREATURE_MIN_DIST_WALL SCREEN_WIDTH - CREATURE_WIDTH/2

//Enum - Used to ID creatures 
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

//Enum - Used to save and toggle the last animation state of a creature's image
typedef enum Position_t{
    Position0,
    Position1
}Position_t;

//Enum - Used to ID what difficulty level is assigned to a creature
typedef enum classes_t{
    NONEXISTENT_CLASS,
    EASY,
    MEDIUM,
    HARD
}classes_t;

//Enum - Used to save and toggle the direction in which a row/creature/ship is moving, in a readable manner.
typedef enum H_Movement_t{
    RIGHT,
    LEFT
}H_Movement_t;

///Struct - Creature Structure
typedef struct creature_t{
    classes_t CreatureType; //! Holds the difficulty level of the creature struct instance
    creatureIDS_t CreatureID; //! Holds this instance's ID, since there are 40 creatures

    signed short x_pos; //! Current X Position of the creature
    signed short y_pos; //! Current Y Position of the creature
    signed short speed; //! Speed in which the creature moves left and right
    unsigned char Alive; //! Alive Flag: 1 -> creature is alive/has not been shot

    Position_t Position; //! Holds what image state of the creature should be drawn, gets toggled in game
    image_handle_t Image_0;//! Image States, each of whom are drawn according to the position_t variable above
    image_handle_t Image_1;
}creature_t;


/**
 *@brief Creates a dynamically allocated array of creature_t structs with the size 40
 *@param void
 *@return creature_t*
 */
creature_t* CreateCreatures();


/**
 *@brief Initializes the FrontierCreaturesID array
 *
 *Frontier creatures are a signed short array of size 8 (NUMB_OF_COLUMNS), that saves
 *the current index/CreaureID of the 8 bottom-most creatures, thus knowing which creatures
 *are allowed to shoot at a given moment, this functio assigns the base initial values when
 *the game has begun which are from 0 to 7.
 *
 *@param FrontierCreaturesID[] The array which holds the indexes of the bottom-most/"Frontier" creatures
 *@return void
 */
void vAssignFrontierCreatures(signed short FrontierCreaturesID[8]);



/**
 *@brief Updates the values within the FrontierCreaturesID array
 *
 *Evaluates which creatures was shot within the 8x5 matrix using the CreatureHitID parameter,
 *then according to the current matrix scenario will choose which will be the new frontier ID
 *saved within the column (index of FrontierCreaturesID).
 *
 *@param FrontierCreaturesID[] The array which holds the indexes of the bottom-most/"Frontier" creatures
 *@param CreatureHitID The ID value/index of the creature that has been currently shot/killed
 *@param creatures The struct array that holds all current creatures information
 *@return void
 */
void vUpdateFrontierCreaturesIDs(signed short* FrontierCreaturesID, 
                                 unsigned char CreatureHitID,
                                 creature_t* creatures);


/**
 *@brief Checks if currently killed creature was port of the frontier
 *
 *Evaluates if the killed creature (using CreatureCollisionID), is part of the frontier/bottom most creatures of
 *a column and returns 1 if it in fact was.
 *
 *@param CreatureCollisionID The ID value/index of the creature that has been currently shot/killed
 *@param FrontierCreaturesID[] The array which holds the indexes of the bottom-most/"Frontier" creatures
 *@return 1 on success, 0 on failure
 */
unsigned char xCheckKilledCreatureWithinFrontier(unsigned char CreatureCollisionID, signed short* FrontierCreaturesID);



/**
 *@brief Checks if there has been a bullet collision within the alive creatures in game
 *
 *Runs a loop on all currently alive creatures and check if the player's bullet is colliding with any of them, finally
 *returning the collided creature's id/index within the creatures struct array in case of an actual collision.
 *
 *@param creatures The struct array that holds all current creatures information
 *@param bullet_x_pos The current X position of the Players Ship Bullet
 *@param bullet_y_pos The current Y position of the Players Ship Bullet
 *@param Direction The current direction in which the swarm of creatures are moving, using the H_Movement_t enum 
 *@return values from 0 to 39 on success, -1 on failure.
 */
signed char xCheckCreaturesCollision(creature_t* creatures,
                                     signed short bullet_x_pos,
                                     signed short bullet_y_pos,
                                     H_Movement_t Direction);


/**
 *@brief Checks if there has been a horizontal direction change regarding the creature swarm
 *
 *Only compares the most current value to the last saved one, if they are different, meaning directions have been changed,
 *return 1, else return 0.
 *
 *@param LastDirection The previously saved direction in which the swarm of creatures are moving, using the H_Movement_t enum 
 *@param CurrentHorizontalDirection The current direction in which the swarm of creatures are moving, using the H_Movement_t enum 
 *@return 1 on success, 0 on failure.
 */
unsigned char xCheckDirectionChange(H_Movement_t* LastDirection, H_Movement_t CurrentHorizontalDirection);


/**
 *@brief Controls Horizontal movement of creatures
 *
 *Checks what is the new direction of the creature swarm and then saves it onto the pointer argument, 
 *depending on this new direction value, it will move accordingly the creatures in a specific direction.
 *
 *@param creatures The struct array that holds all current creatures information
 *@param DIRECTION Pointer to the current direction in which the swarm of creatures are moving, using the H_Movement_t enum 
 *@return void
 */
void vMoveCreaturesHorizontal(creature_t* creature, H_Movement_t* DIRECTION);


/**
 *@brief Controls Vertical movement of creatures
 *
 *Simply moves all alive creatures vertically down as the condition to be met that would allow this action
 *is checked before the calling of this function.
 *
 *@param creatures The struct array that holds all current creatures information
 *@return void
 */
void vMoveCreaturesVerticalDown(creature_t* creatures);



/**
 *@brief Checks if bottom-most/frontier any creature are in contact with an "alive" bunker
 *
 *Checks the position of the current bottom-most/frontier alive creatures and cross references them with
 *the top edge of the currently alive bunkers, if there is contact, the bunker ID of the bunker
 *being collided with will be returned, the bunker ID is of the enum type Bunkers_ID_t.
 *
 *@param creatures The struct array that holds all current creatures information
 *@param FrontierCreaturesID[] The array which holds the indexes of the bottom-most/"Frontier" creatures
 *@param Bunkers array which holds all current bunkers information
 *@return values from 1 to 4 on success, 0 on failure
 */
unsigned char xCheckCreaturesTouchBunkers(creature_t* creatures,
                                          signed short* FrontierCreaturesID,
                                          bunkers_t* Bunkers);




/**
 *@brief Checks if bottom-most/frontier any creature are in contact with the bottom-limit of the game
 *
 *Checks the position of the current bottom-most/frontier alive creatures and cross references them with
 *the bottom-limit of the game, if there is contact return 1.
 *
 *@param creatures The struct array that holds all current creatures information
 *@param FrontierCreaturesID[] The array which holds the indexes of the bottom-most/"Frontier" creatures
 *@return 1 on success, 0 on failure
 */
unsigned char xCheckFrontierReachedBottom(creature_t* creatures,
                                          signed short* FrontierCreaturesID);




/**
 *@brief Increments all the alive creatures speeed by one
 *
 *@param Creatures The struct array that holds all current creatures information
 *@return void
 */
void vUpdateCreaturesSpeed(creature_t* Creatures);





/**
 *@brief Creates new creatures bullet and overwrites its structure to an already initialized creatures bullet structure
 *
 *Chooses the creature who will shoot the bullet using a pseudo-random generator, a simple check if
 *the chosen creature is in fact alive and a check if the chosen column is empty, then creates the bullet using
 *the valid chosen creatures position to simulate a bullet leaving from said creature.
 *
 *@param creatures The struct array that holds all current creatures information
 *@param CreaturesBullet pointer to bullet struct responsible of keeping the creatures bullet information
 *@param FrontierCreaturesID[] The array which holds the indexes of the bottom-most/"Frontier" creatures
 *@return void
 */
void vCreateCreaturesBullet(creature_t* creatures, 
                            bullet_t* CreaturesBullet,
                            signed short* FrontierCreaturesID);



/**
 *@brief Draws the creatures bullet
 *
 *Uses the x and y positions stored in the bullet structure to draw bullet on screen.
 *
 *@param CreatureBullets pointer to bullet struct responsible of keeping the creatures bullet information
 *@return void
 */
void vDrawCreaturesBullet(bullet_t* CreatureBullets);



/**
 *@brief Updates bullet position
 *
 *Uses speed variable stored in bullet structure to update the new position of the creatures bullet.
 *
 *@param CreaturesBullet pointer to bullet struct responsible of keeping the creatures bullet information
 *@return void
 */
void vUpdateCreaturesBulletPos(bullet_t* CreaturesBullet);



/**
 *@brief Checks if creatures bullet has reached the bottom wall
 *
 *Uses the current y position of the creatures bullet to evaluate if it has reached the bottom wall.
 *
 *@param y_pos holds the y position of the creatures bullet
 *@return 1 on success, 0 on no collision
 */
unsigned char xCheckCreaturesBulletCollisonBottomWall(signed short y_pos);


/**
 *@brief Checks if creatures bullet has hit the players ship
 *
 *Uses the current x and y position of the creatures bullet to evaluate if it has the players ship.
 *
 *@param x_pos holds the x position of the creatures bullet
 *@param y_pos holds the y position of the creatures bullet
 *@param ship pointer to ship_t structure which holds all current information of the players ship
 *@return 1 on success, 0 on no collision
 */
unsigned char xCheckCreaturesBulletShipCollision(signed short x_pos,
                                                 signed short y_pos,
                                                 ship_t* ship);



/**
 *@brief "Kills" the creature whose pointer is handed over as argument
 *
 *Sets the Alive Flag within the handed over creature_t* to 0, indicating to other functions
 *that this creature is dead/not to be interacted with; also decrements the NumbOfAliveCreatures value
 *for future use in other functions.
 *
 *@param creature pointer to a single creature_t struct, which holds all information regarding the to-be-killed creature
 *@param NumbOfAliveCreatures pointer to an integer that hols the number of alive creatures in the game
 *@return void
 */
void vKillCreature(creature_t* creature, unsigned short* NumbOfAliveCreatures);




/**
 *@brief Alternates the to be drawn creatures image
 *
 *Toggles the value of Position_t within the creatures structure, thus telling drawing functions 
 *to draw a different image of said creature since draw functions use this value to select the necessary
 *image within an image array.
 *
 *@param creature pointer to a single creature_t struct, which holds all information regarding the to-be-animated creature
 *@return void
 */
void vAlternateAnimation(creature_t* creature);



/**
 *@brief Fetches the killed creatures value for a score update
 *
 *Enters a switch case structure and matches the possible creature types to the one handed over as an argument,
 *the returns the corresponding value in points.
 *
 *@param creatureclassID pointer to a single creature_t struct, which holds all information regarding the killed creature
 *@return Value of handed over creature, 0 in case of error
 */
unsigned char xFetchCreatureValue(unsigned char creatureclassID);


/**
 *@brief Retrieves the dead creatures X and Y position and stores it onto the corresponding pointers overloaded as arguments
 *
 *@param x pointer to the to-be-stored variable who will hold the X position of the dead creature
 *@param y pointer to the to-be-stored variable who will hold the Y position of the dead creature
 *@param creature a single creature_t struct, which holds all information regarding the to-be-killed creature
 *@return void
 */
void vRetrieveDeadCreatureXY(signed short* x, signed short* y, creature_t creature);

#endif 

