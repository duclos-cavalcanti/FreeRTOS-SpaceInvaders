
#ifndef __saucer_H__
#define __saucer_H__


#include "TUM_Draw.h"
#include "creatures.h"
#include "main.h"

/**Saucer Dimensions related*/
#define SAUCER_WIDTH 35
#define SAUCER_HEIGHT 23

/**Saucer Initialization Values related*/
#define SAUCER_APPEARS_X SCREEN_WIDTH/2
#define SAUCER_APPEARS_Y SCREEN_HEIGHT*1/10 + 15
#define SAUCER_SPEED 2



/**
 * @name Saucer Structure
 * @brief The structure which contains all relevant information to the enemy saucer
 */
typedef struct saucer_t{
    signed short x_pos; //! Current X coord
    signed short y_pos; //! Current Y coord
    signed short speed;
    
    image_handle_t Images[2]; //!Image Array, each of whom corresponds either to the Single Player type Saucer or AI Saucer
}saucer_t;


/**
 * @name Opponent Commands
 * @brief Enum used to manipulate AI sent commands in a readable manner
 */
typedef enum OpponentCommands_t{
    NONE,
    INC=1,
    DEC=-1
}OpponentCommands_t;



/**
 * @brief Creates pointer to saucer_t structure which is then returned
 * @return saucer_t*
 */
saucer_t* CreateSaucer();


/**
 * @brief Moves Saucer to the left using its attribute speed
 * @param saucer pointer to saucer_t that contains all relevant information to the saucer
 * @return void
 */
void vMoveSaucerLeft(saucer_t* saucer);

/**
 * @brief Moves Saucer to the right using its attribute speed
 * @param saucer pointer to saucer_t that contains all relevant information to the saucer
 * @return void
 */
void vMoveSaucerRight(saucer_t* saucer);


/**
 * @brief Function used in single player mode, where it controls the left and right movement of the saucer in game 
 *
 * This Function will check the new possible direction of the saucer by evaluating if it is currently located at a edge of the screen
 * and possibly changing the direction pointer value accordingly, finally using the newly changed/or maintained direction, the function
 * will order the saucer to move either using vMoveSaucerLeft or vMoveSaucerRight.
 *
 * @param saucer pointer to saucer_t that contains all relevant information to the saucer
 * @param Direction Enum type variable that shows which direction the saucer is moving currently
 * @return void
 */
void vMoveSaucerHorizontal(saucer_t* saucer, H_Movement_t* Direction);



/**
 * @brief Checks if current players bullet is colliding with Saucer
 * 
 * Using the boundaries of the saucers dimensions together with the knowledge of its current moving direction, the
 * function determines if the players bullet is in fact colliding or about to collide with the enemy saucer.
 *
 * @param saucer pointer to saucer_t that contains all relevant information to the saucer
 * @param Direction Enum type variable that shows which direction the saucer is moving currently
 * @param b_xpos integer variable holds the players bullet x pos 
 * @param b_ypos integer variable holds the players bullet y pos 
 * @return 1 on success, 0 on failure
 */
unsigned char xCheckSaucerCollision(saucer_t* saucer,
                                    H_Movement_t Direction,
                                    signed short b_xpos,
                                    signed short b_ypos);


/**
 * @brief Dedicated function for the AI Saucer that wraps around its movement in case its stuck at border
 * 
 * Using the boundaries of the saucers dimensions together with the knowledge of its current moving direction, the
 * function determines if the players bullet is in fact colliding or about to collide with the enemy saucer.
 *
 * @param saucer pointer to saucer_t that contains all relevant information to the saucer
 * @param CurrentDirection Enum type variable that shows which direction the saucer is moving currently
 * @return void
 */
void xCheckAISaucerBorder(saucer_t* saucer, H_Movement_t CurrentDirection);


/**
 * @brief Sets SaucerHitFlag to 1, indicating that the Saucer has been hit 
 * and therefore sets SaucerAppearsFlag=0 to impede drawing of saucer.
 * 
 * @param SaucerHitFlag pointer to SaucerHitFlag 
 * @param SaucerAppearsFlag pointer to SaucerAppearsFlag 
 * @return void
 */
void vKillSaucer(unsigned char* SaucerHitFlag, unsigned char* SaucerAppearsFlag);


/**
 * @brief Saves the dead sauce's X and Y values onto the handed over pointers so the corresponding animation
 * to saucer death can be correctly drawn.
 * 
 * @param DeadSaucerX pointer to DeadSaucerX where the saucer's X will be stored
 * @param DeadSaucerY pointer to DeadSaucerY where the saucer's Y will be stored
 * @param saucer pointer to saucer_t that contains all relevant information to the saucer
 * @return void
 */
void vRetrieveDeadSaucerXY(signed short* DeadSaucerX, signed short* DeadSaucerY, saucer_t* saucer);



/**
 * @brief Fetches the randomly generated value associated with the death of the enemy saucer
 * @return Value of Saucer's death
 */
unsigned char xFetchSaucerValue();


#endif
