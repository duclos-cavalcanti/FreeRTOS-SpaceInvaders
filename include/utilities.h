#ifndef __utilities_H__
#define __utilities_H__

#include "main.h"
#include "creatures.h"

/**
 * @brief Changes the highlighted selection in the menu of the Main Menu Task
 *
 * Receives the pointer to the SelectedMenuOption_t of the global struct MainMenuInfoBuffer_t, and then changes its value to the next
 * possible option towards the downward direction of the Menu.
 * @param CurrentSelect pointer to a SelectedMenuOption_t variable, which holds which option is being highlighed in the Main Menu Task
 * @return void
 */
void vDownMenuSelection(SelectedMenuOption_t* CurrentSelect);


/**
 * @brief Changes the highlighted selection in the menu of the Main Menu Task
 *
 * Receives the pointer to the SelectedMenuOption_t of the global struct MainMenuInfoBuffer_t, and then changes its value to the next
 * possible option towards the upward direction of the Menu.
 * @param CurrentSelect pointer to a SelectedMenuOption_t variable, which holds which option is being highlighed in the Main Menu Task
 * @return void
 */
void vUpMenuSelection(SelectedMenuOption_t* CurrentSelect);



/**
 * @brief Changes the highlighted selection in the menu of the Game Over Task
 *
 * Receives the pointer to the SelectedGameOverOption_t of the global struct GameOverInfoBuffer_t, and then changes its value to the next
 * possible option towards the downward direction of the Menu.
 * @param CurrentSelect pointer to a SelectedGameOverOption_t variable, which holds which option is being highlighed in the Game Over Task
 * @return void
 */
void vDownGameOverSelection(SelectedGameOverOption_t* CurrentSelect);


/**
 * @brief Changes the highlighted selection in the menu of the Game Over Task
 *
 * Receives the pointer to the SelectedGameOverOption_t of the global struct GameOverInfoBuffer_t, and then changes its value to the next
 * possible option towards the upward direction of the Menu.
 * @param CurrentSelect pointer to a SelectedGameOverOption_t variable, which holds which option is being highlighed in the Game Over Task
 * @return void
 */
void vUpGameOverSelection(SelectedGameOverOption_t* CurrentSelect);



/**
 * @brief Changes the highlighted selection in the menu of the Paused Game Task
 *
 * Receives the pointer to the SelectedPausedGameOption_t of the global struct PausedGameInfoBuffer_t, and then changes its value to the next
 * possible option towards the downward direction of the Menu.
 * @param CurrentSelect pointer to a SelectedPausedGameOption_t variable, which holds which option is being highlighed in the Paused Game Task
 * @return void
 */
void vDownPausedSelection(SelectedPausedGameOption_t* CurrentSelect);


/**
 * @brief Changes the highlighted selection in the menu of the Paused Game Task
 *
 * Receives the pointer to the SelectedPausedGameOption_t of the global struct PausedGameInfoBuffer_t, and then changes its value to the next
 * possible option towards the upward direction of the Menu.
 * @param CurrentSelect pointer to a SelectedPausedGameOption_t variable, which holds which option is being highlighed in the Paused Game Task
 * @return void
 */
void vUpPausedSelection(SelectedPausedGameOption_t* CurrentSelect);

/**
 * @brief Changes the highlighted selection in the menu of the Cheats Task
 *
 * Receives the pointer to the SelectedCheatsOption_t of the global struct CheatsInfoBuffer_t, and then changes its value to the next
 * possible option towards the downward direction of the Menu.
 * @param CurrentSelect pointer to a SelectedCheatsOption_t variable, which holds which option is being highlighed in the Cheats Task
 * @return void
 */
void vDownCheatsSelection(SelectedCheatsOption_t* CurrentSelect);


/**
 * @brief Changes the highlighted selection in the menu of the Cheats Task
 *
 * Receives the pointer to the SelectedCheatsOption_t of the global struct CheatsInfoBuffer_t, and then changes its value to the next
 * possible option towards the upward direction of the Menu.
 * @param CurrentSelect pointer to a SelectedCheatsOption_t variable, which holds which option is being highlighed in the Cheats Task
 * @return void
 */
void vUpCheatsSelection(SelectedCheatsOption_t* CurrentSelect);


/**
 * @brief Increments a cheat value depending on the cheat option being currently highlighted in the Cheats Task menu
 *
 * Receives pointers to the starting score values of both cheating options and depending on the 
 * highlighted cheat option, the corresponding starting score value will be incremented
 *
 * @param CurrentSelect pointer to a SelectedCheatsOption_t variable, which holds which option is being highlighed in the Cheats Task
 * @param StartingScoreValue pointer to the Starting Score Value that can be modified within the cheats menu
 * @param StartingLevelValue pointer to the Starting Level Value that can be modified within the cheats menu
 * @return void
 */
void vIncrementValue(SelectedCheatsOption_t CurrentSelect,
                     unsigned int* StartingScoreValue,
                     unsigned int* StartingLevelValue);


/**
 * @brief Decrements a cheat value depending on the cheat option being currently highlighted in the Cheats Task menu
 *
 * Receives pointers to the starting score values of both cheating options and depending on the 
 * highlighted cheat option, the corresponding starting score value will be decremented
 *
 * @param CurrentSelect pointer to a SelectedCheatsOption_t variable, which holds which option is being highlighed in the Cheats Task
 * @param StartingScoreValue pointer to the Starting Score Value that can be modified within the cheats menu
 * @param StartingLevelValue pointer to the Starting Level Value that can be modified within the cheats menu
 * @return void
 */
void vDecrementValue(SelectedCheatsOption_t CurrentSelect,
                     unsigned int* StartingScoreValue,
                     unsigned int* StartingLevelValue);


/**
 * @brief Fetches the fitting color to be used on the String associated with the ConsideredOption integer
 *
 * Compares current select option to a considered option, if there is a match returns the Green Hex, signaling to highlight the string
 * within its draw function who is associated to the enum type handed over as a "ConsideredOption", this is used by all menu drawing tasks
 *
 * @param CurrentSelect integer that indicates which option is supposed to be highlighed in the Cheats/Paused Game/MainMenu/Game Over Task
 * @param ConsideredOption possible option to match against
 * @return void
 */
unsigned int xFetchSelectedColor(unsigned char CurrentSelect, unsigned char ConsideredOption);


/**
 * @brief Fetches the corresponding Hex Color to the handed over Animation condition, used to highlight the Lives string when player loses/gains a life
 *
 * Similar to xFetchSelectedColor, evaluates which Animated condition the handed over argument fits in a switch case structure and returns the Hex Color
 * corresponding to its particular Animated Condition.
 *
 * @param AnimatedCondition integer that maps onto a LivesAnimation_t switch case and fits into one of the possible cases
 * @return void
 */
unsigned int xFetchAnimationColor(unsigned char AnimationCondition);


/**
 * @brief Assigns specific images to creatures of certain difficulty types, said images are assigned to image_handle_t variables
 * located within the creatures struct
 *
 * @param Creatures array of creature_t structs that contain all creature structs and their relevant information
 * @param ImageCatalog array of image_handle_t that contains all images regarding the three types of creatures
 * @return void
 */
void vAssignCreaturesImages(creature_t* Creatures, image_handle_t* ImageCatalog);

#endif
