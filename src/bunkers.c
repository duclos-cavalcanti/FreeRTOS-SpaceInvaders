#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "bunkers.h"
#include "main.h"


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
   SingleBunker_t* b1=CreateSingleBunker(B1_X_POS,BUNKERS_Y_POS,BUNKER_COLOR, BUNKER_SIDE_SIZE);
   SingleBunker_t* b2=CreateSingleBunker(B2_X_POS,BUNKERS_Y_POS,BUNKER_COLOR, BUNKER_SIDE_SIZE);
   SingleBunker_t* b3=CreateSingleBunker(B3_X_POS,BUNKERS_Y_POS,BUNKER_COLOR, BUNKER_SIDE_SIZE);
   SingleBunker_t* b4=CreateSingleBunker(B4_X_POS,BUNKERS_Y_POS,BUNKER_COLOR, BUNKER_SIDE_SIZE);
   printf("Bunkers Y_POS: %d\n",BUNKERS_Y_POS);

    bunkers_t* Bunkers = calloc(1,sizeof(bunkers_t));
    if(!Bunkers){
        fprintf(stderr,"Creating Bunkers wrapper failed.\n");
        exit(EXIT_FAILURE);
    }

    Bunkers->b1=b1;
    Bunkers->b2=b2;
    Bunkers->b3=b3;
    Bunkers->b4=b4;
    
    Bunkers->b1Lives=5;
    Bunkers->b2Lives=5;
    Bunkers->b3Lives=5;
    Bunkers->b4Lives=5;

    return Bunkers;
}

unsigned char xCheckBunkersCollision(ship_t* ship)
{
    
    return 1;
}

void vUpdateBunkersStatus(bunkers_t* bunkers)
{

}
