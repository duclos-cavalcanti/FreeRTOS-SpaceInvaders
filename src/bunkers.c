#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#include "bunkers.h"
#include "main.h"

#include "TUM_Draw.h"


#define BUNKER_SIDE_SIZE 20

SingleBunker_t* CreateSingleBunker(signed short x_pos, signed short y_pos,
                                   unsigned int color, signed short size)
{
    SingleBunker_t* bunker = calloc(1,sizeof(SingleBunker_t));
    if(!bunker){
        fprintf(stderr,"Creating Single Bunker failed.\n");
        exit(EXIT_FAILURE);
    }

    bunker->x_pos=x_pos;
    bunker->y_pos=y_pos;
    bunker->color=color;
    bunker->size=size;

    return bunker;
}

bunkers_t* CreateBunkers()
{
   SingleBunker_t* b1=CreateSingleBunker(SCREEN_WIDTH*1/8,SCREEN_HEIGHT*70/100,Green, BUNKER_SIDE_SIZE);
   SingleBunker_t* b2=CreateSingleBunker(SCREEN_WIDTH*3/8,SCREEN_HEIGHT*70/100,Green, BUNKER_SIDE_SIZE);
   SingleBunker_t* b3=CreateSingleBunker(SCREEN_WIDTH*5/8,SCREEN_HEIGHT*70/100,Green, BUNKER_SIDE_SIZE);
   SingleBunker_t* b4=CreateSingleBunker(SCREEN_WIDTH*7/8,SCREEN_HEIGHT*70/100,Green, BUNKER_SIDE_SIZE);

    bunkers_t* Bunkers = calloc(1,sizeof(bunkers_t));
    if(!Bunkers){
        fprintf(stderr,"Creating Bunkers wrapper failed.\n");
        exit(EXIT_FAILURE);
    }

    Bunkers->b1=b1;
    Bunkers->b2=b2;
    Bunkers->b3=b3;
    Bunkers->b4=b4;

    return Bunkers;
}

