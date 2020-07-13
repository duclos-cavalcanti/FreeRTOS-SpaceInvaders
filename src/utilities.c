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
        case Cheats:
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
        case Cheats:
            (*CurrentSelect)--;
            break;
        case SinglePlayer:
           (*CurrentSelect) = Leave;
    }
}


void vDownGameOverSelection(SelectedGameOverOption_t* CurrentSelect)
{
    switch(*CurrentSelect){
        case PlayAgain:
            (*CurrentSelect)++;
            break;
        case Quit:
           (*CurrentSelect) = PlayAgain;
    }
}

void vUpGameOverSelection(SelectedGameOverOption_t* CurrentSelect)
{
    switch(*CurrentSelect){
        case Quit:
            (*CurrentSelect)--;
            break;
        case PlayAgain:
           (*CurrentSelect) = Quit;
    }
}

void vDownPausedSelection(SelectedPausedGameOption_t* CurrentSelect)
{
    switch(*CurrentSelect){
        case Resume:
            (*CurrentSelect)++;
            break;
        case RestartReset:
           (*CurrentSelect) = Resume;
            break;
    }
}

void vUpPausedSelection(SelectedPausedGameOption_t* CurrentSelect)
{
    switch(*CurrentSelect){
        case RestartReset:
            (*CurrentSelect)--;
            break;
        case Resume:
           (*CurrentSelect) = RestartReset;
            break;
    }
}

void vUpCheatsSelection(SelectedCheatsOption_t* CurrentSelect)
{
    switch(*CurrentSelect){
        case ChooseStartingLevel:
        case ChooseStartingScore:
            (*CurrentSelect)--;
            break;
        case InfiniteLives:
           (*CurrentSelect) = ChooseStartingLevel;
            break;
    }
}

void vDownCheatsSelection(SelectedCheatsOption_t* CurrentSelect)
{
    switch(*CurrentSelect){
        case InfiniteLives:
        case ChooseStartingScore:
            (*CurrentSelect)++;
            break;
        case ChooseStartingLevel:
           (*CurrentSelect) = InfiniteLives;
            break;
    }
}

void vIncrementValue(SelectedCheatsOption_t CurrentSelect,
                     unsigned int* StartingScoreValue,
                     unsigned int* StartingLevelValue)
{
    switch(CurrentSelect){
        case ChooseStartingScore:
            (*StartingScoreValue)+=100;
            break;
        case ChooseStartingLevel:
            (*StartingLevelValue)++;
           break;
        case InfiniteLives:
            break;
    }

}

void vDecrementValue(SelectedCheatsOption_t CurrentSelect,
                     unsigned int* StartingScoreValue,
                     unsigned int* StartingLevelValue)
{
    switch(CurrentSelect){
        case ChooseStartingScore:
            if((*StartingScoreValue) > 0)
                (*StartingScoreValue)-=100;
            break;
        case ChooseStartingLevel:
            if((*StartingLevelValue) > 1)
                (*StartingLevelValue)--;
           break;
        case InfiniteLives:
            break;
    }
}

unsigned int xFetchAnimationColor(unsigned char AnimationCondition)
{
    if(AnimationCondition == LivesLost)
        return Red;
    else if(AnimationCondition == LivesGained)
        return Green;
    else
        return White;
}

unsigned int xFetchSelectedColor(unsigned char CurrentSelect, unsigned char ConsideredOption)
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
        else if(Creatures[i].CreatureType==HARD){
            Creatures[i].Image_0 = ImageCatalog[4];
            Creatures[i].Image_1 = ImageCatalog[5];
        }
    }
}
