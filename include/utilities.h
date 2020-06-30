#ifndef __utilities_H__
#define __utilities_H__

#include "main.h"

void vDownMenuSelection(SelectedMenuOption_t* CurrentSelect);
void vUpMenuSelection(SelectedMenuOption_t* CurrentSelect);

unsigned int xFetchSelectedColor(SelectedMenuOption_t CurrentSelect, SelectedMenuOption_t ConsideredOption);

#endif
