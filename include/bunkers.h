#ifndef __bunkers_H__
#define __bunkers_H__

#include "TUM_Draw.h"
#include "ship.h"


/**Bunkers Initialization Values*/
#define BUNKERS_Y_POS SCREEN_HEIGHT*78/100
#define BUNKER_SIDE_SIZE 40
#define BUNKER_COLOR Green

/**Bunkers Respective X-Coordinates*/
#define B1_X_POS SCREEN_WIDTH*1/8
#define B2_X_POS SCREEN_WIDTH*3/8
#define B3_X_POS SCREEN_WIDTH*5/8
#define B4_X_POS SCREEN_WIDTH*7/8

/**Bunkers Upper and Lower Limits - Used for Bullet Collision*/
#define BUNKERS_LOWERLIMIT BUNKERS_Y_POS + BUNKER_SIDE_SIZE/2
#define BUNKERS_UPPERLIMIT BUNKERS_Y_POS - BUNKER_SIDE_SIZE/2

/**Bunkers Left and Right Limits - Used for Bullet Collision*/
#define B1_LEFT_LIMIT B1_X_POS - BUNKER_SIDE_SIZE/2 - SHIP_BULLET_THICKNESS/2
#define B1_RIGHT_LIMIT B1_X_POS + BUNKER_SIDE_SIZE/2 + SHIP_BULLET_THICKNESS/2

#define B2_LEFT_LIMIT B2_X_POS - BUNKER_SIDE_SIZE/2- SHIP_BULLET_THICKNESS/2
#define B2_RIGHT_LIMIT B2_X_POS + BUNKER_SIDE_SIZE/2+ SHIP_BULLET_THICKNESS/2

#define B3_LEFT_LIMIT B3_X_POS - BUNKER_SIDE_SIZE/2- SHIP_BULLET_THICKNESS/2
#define B3_RIGHT_LIMIT B3_X_POS + BUNKER_SIDE_SIZE/2+ SHIP_BULLET_THICKNESS/2

#define B4_LEFT_LIMIT B4_X_POS - BUNKER_SIDE_SIZE/2- SHIP_BULLET_THICKNESS/2
#define B4_RIGHT_LIMIT B4_X_POS + BUNKER_SIDE_SIZE/2+ SHIP_BULLET_THICKNESS/2

/**
 * @brief Holds an ID value that shows which Bunker is being referenced to.
 */
typedef enum Bunkers_ID_t{
    NONEXISTENT_BUNKER,
    B1,
    B2,
    B3,
    B4
}Bunkers_ID_t;

/**
 * @name Single Bunker Struct
 * @brief The structure which contains all relevant information to a single bunker
 */
typedef struct SingleBunker_t{
    signed short x_pos;
    signed short y_pos;
    
    unsigned int color;
    signed short size; //! Bunker is treated mostly as a square, the size variable works bot for height and width

}SingleBunker_t;

/**
 * @name Collective Bunker Struct
 * @brief The structure which contains all 4 single bunker structures and the respectivou count of their lives.
 */
typedef struct bunkers_t{
    SingleBunker_t* b1;
    signed short b1Lives;

    SingleBunker_t* b2;
    signed short b2Lives;

    SingleBunker_t* b3;
    signed short b3Lives;
    
    SingleBunker_t* b4;
    signed short b4Lives;

}bunkers_t;


/**
 *@brief Creates a dynamically allocated pointer to the bunkers_t struct which is then returned.
 *@param void
 *@return bunkers_t*
 */
bunkers_t* CreateBunkers();


/**
 *@brief Checks if bullets X and Y coordinates could be colliding with any of the 4 bunkers, if alive.
 *@param bullet_xpos the bullets x coordinate (Ship bullet or Creatures bullet) that is being evaluated for collision
 *@param bullet_ypos the bullets y coordinate (Ship bullet or Creatures bullet) that is being evaluated for collision
 *@param bunkers bunkers array that contains all single bunker structs and their relevant information
 *@return a Bunkers_ID_t (value from 1 to 4) on success, 0 on failure
 */
unsigned char xCheckBunkersCollision(signed short bullet_xpos, signed short bullet_ypos,
                                     bunkers_t bunkers);

/**
 *@brief Checks if any alive creature is in contact with an alive bunker, results in bunker complete destruction.
 *@param c_xpos the creatures x coord being evaluated for collision
 *@param c_ypos the creatures y coord being evaluated for collision
 *@param bunkers bunkers array that contains all single bunker structs and their relevant information
 *@return a Bunkers_ID_t (value from 1 to 4) on success, 0 on failure
 */
unsigned char xCheckSingleCreatureBunkerCollision(signed short c_xpos, signed short c_ypos,
                                                  bunkers_t* bunkers);


/**
 *@brief Decrements the life points of a single bunker structure by one
 *@param bunkers bunkers array that contains all single bunker structs and their relevant information
 *@param bunkerID integer that indicates which bunker that has to be worked upon
 *@return void
 */
void vUpdateBunkersStatus(bunkers_t* bunkers, unsigned char bunkerID);


/**
 *@brief Sets the life points of a specific bunker to 0 AKA killing it.
 *@param bunkers bunkers array that contains all single bunker structs and their relevant information
 *@param bunkerID integer that indicates which bunker that has to be worked upon
 *@return void
 */
void vKillBunker(bunkers_t* bunkers, unsigned char bunkerID);
#endif
