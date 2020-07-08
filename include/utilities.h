#ifndef __utilities_H__
#define __utilities_H__

#include "main.h"
#include "creatures.h"

typedef struct NumberToMatrix_t{
    unsigned short row_x;
    unsigned short column_y;
}NumberToMatrix_t;

void vDownMenuSelection(SelectedMenuOption_t* CurrentSelect);
void vUpMenuSelection(SelectedMenuOption_t* CurrentSelect);

void vDownGameOverSelection(SelectedGameOverOption_t* CurrentSelect);
void vUpGameOverSelection(SelectedGameOverOption_t* CurrentSelect);


void vDownPausedSelection(SelectedPausedGameOption_t* CurrentSelect);
void vUpPausedSelection(SelectedPausedGameOption_t* CurrentSelect);

unsigned int xFetchSelectedColor(unsigned char CurrentSelect, unsigned char ConsideredOption);
unsigned int xFetchAnimationColor(unsigned char AnimationCondition);
void vAssignCreaturesImages(creature_t* Creatures, image_handle_t* ImageCatalog);

#endif
