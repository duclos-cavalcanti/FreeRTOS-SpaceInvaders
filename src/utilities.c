#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#include "TUM_Draw.h"
#include "main.h"
#include "utilities.h"
#include "creatures.h"



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


void vAssignCreaturesImages(creature_t* Creatures, image_handle_t* ImageCatalog)
{
    for(int i=0;i<NUMB_OF_CREATURES;i++){ 
        if(Creatures[i].CreatureType==EASY){
            Creatures[i].Image_0 = ImageCatalog[0];
            Creatures[i].Image_1 = ImageCatalog[1];
        }

        else if(Creatures[i].CreatureType==MEDIUM){
            Creatures[i].Image_0 = ImageCatalog[2];
            Creatures[i].Image_1 = ImageCatalog[3];
        }
    }
}

NumberToMatrix_t xFindMatrixValue(unsigned char CreatureChoiceID)
{
    NumberToMatrix_t MatrixValues;
    if(CreatureChoiceID<NUMB_OF_COLUMNS){
        MatrixValues.row_x = 0;
        MatrixValues.column_y = CreatureChoiceID; 
    }
    else{
        unsigned short Remainder = CreatureChoiceID % NUMB_OF_COLUMNS; 
        MatrixValues.column_y=Remainder;
        if(CreatureChoiceID < 16)
            MatrixValues.row_x=1;
        else if(CreatureChoiceID < 24)
            MatrixValues.row_x=2;
        else 
            MatrixValues.row_x=3;
    }
    return MatrixValues;
}
