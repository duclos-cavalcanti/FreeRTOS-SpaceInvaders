#ifndef __utilities_H__
#define __utilities_H__

#include "main.h"
#include "creatures.h"

void vDownMenuSelection(SelectedMenuOption_t* CurrentSelect);
void vUpMenuSelection(SelectedMenuOption_t* CurrentSelect);

unsigned int xFetchSelectedColor(SelectedMenuOption_t CurrentSelect, SelectedMenuOption_t ConsideredOption);
void vAssignCreaturesImages(creature_t* Creatures, image_handle_t* ImageCatalog);

#endif
