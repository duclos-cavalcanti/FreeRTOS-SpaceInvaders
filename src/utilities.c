#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#include "TUM_Draw.h"
#include "main.h"




void vDownMenuSelection(SelectedMenuOption_t* CurrentSelect)
{
    switch(*CurrentSelect){
        case SinglePlayer:
        case MultiPlayer:
            (*CurrentSelect)++;
            break;
        case Leave:
           (*CurrentSelect) = SinglePlayer;
    }
}

void vUpMenuSelection(SelectedMenuOption_t* CurrentSelect)
{
    switch(*CurrentSelect){
        case Leave:        
        case MultiPlayer:
            (*CurrentSelect)--;
            break;
        case SinglePlayer:
           (*CurrentSelect) = Leave;
    }
}

unsigned int xFetchSelectedColor(SelectedMenuOption_t CurrentSelect, SelectedMenuOption_t ConsideredOption)
{
    if(ConsideredOption == CurrentSelect)
        return Green;
    else 
        return White;
}
