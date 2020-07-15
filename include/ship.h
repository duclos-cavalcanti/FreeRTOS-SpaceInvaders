#ifndef __ship_H__
#define __ship_H__

#include "bunkers.h"

/**Ship Initialization Values*/
#define SHIPSPEED 4
#define SHIPSIZE 20
#define SHIP_BULLET_SPEED 13
#define SHIP_Y_OFFSET 5 //Y OFFSET from Lower Wall
#define SHIP_BULLET_THICKNESS 2
#define SHIP_BULLET_LENGTH 10

/**UPPER Wall related*/
#define UPPER_WALL_LIMIT 40


/**
 * @name Single bullet Struct
 * @brief The structure which contains all relevant information to a single bullet
 */
typedef struct bullet_t{
signed short x_pos; //! Current X coord of the bullet
signed short y_pos; //! Current Y coord of the bullet
signed short speed;

unsigned char BulletAliveFlag; //! This Flag when set to 1 -> Bullet is still traveling, hasnt crashed yet
}bullet_t;


/**
 * @name Player Ship Struct
 * @brief The structure which contains all relevant information to the players ship
 */
typedef struct ship_t{
    signed short x_pos;
    signed short y_pos;
    signed short speed;

    unsigned short size; //! Refers to the Ship's Width, since its Height is very negligable
    bullet_t* bullet;
}ship_t;


/**
 *@brief Creates a dynamically allocated pointer to a ship_t struct which is then returned.
 *@param initial_x X coord of ship when game begins and is assigned to the x_pos of the ship struct
 *@param initial_y Y coord of ship when game begins and is assigned to the y_pos of the ship struct
 *@param speed will be assigned to the speed attribute of the ship struct
 *@param ShipColor Ships color
 *@param size This value will be assigned to the size attribute of the ship struct
 *@return ship_t*
 */
ship_t* CreateShip(signed short initial_x, signed short initial_y, signed short speed, 
                   unsigned int ShipColor, signed short size);



/**
 *@brief Creates a bullet_t variable which is then assigned/appended to a previously allocated bullet_t variable in the ship_t* pointer passed as an argument
 *@param ship pointer to the ship structure whose bullet is being created for
 *@return void*
 */
void CreateShipBullet(ship_t* ship);


/**
 *@brief Evaluates if players shup bullet is colliding with the top wall of the game
 *@param b_ypos current Y coord of the players bullet
 *@return 1 on success, 0 on failure
 */
unsigned char xCheckShipBulletCollisionTopWall(signed short b_ypos);


/**
 *@brief Evaluates if the players ship bullet is colliding with creatures bullet
 *@param ship_bxpos current X coord of the players bullet
 *@param ship_bypos current Y coord of the players bullet
 *@param creatures_bxpos current X coord of the creatures bullet
 *@param creatures_bypos current Y coord of the creatures bullet
 *@return 1 on success, 0 on failure
 */
unsigned char xCheckBullet2BulletCollision(signed short ship_bxpos, signed short ship_bypos,
                                           signed short creatures_bxpos, signed short creatures_bypos);


/**
 *@brief Increments the ships position to the left according to the ships speed
 *@param ship pointer to the ship structure 
 *@return void
 */
void vIncrementShipLeft(ship_t* ship);


/**
 *@brief Increments the ships position to the right according to the ships speed
 *@param ship pointer to the ship structure 
 *@return void
 */
void vIncrementShipRight(ship_t* ship);



/**
 *@brief Updates the ships bullet position according to the ships bullet speed
 *@param ship pointer to the ship structure 
 *@return void
 */
void vUpdateShipBulletPos(ship_t* ship);


/**
 *@brief Draws the ships bullet
 *@param ship pointer to the ship structure 
 *@return void
 */
void vDrawShipBullet(ship_t* ship);
#endif 
