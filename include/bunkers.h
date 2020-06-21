#ifndef __bunkers_H__
#define __bunkers_H__

typedef struct SingleBunker_t{
    signed short x_pos;
    signed short y_pos;
    
    unsigned int color;
    signed short size;

}SingleBunker_t;

typedef struct bunkers_t{
    SingleBunker_t* b1;
    SingleBunker_t* b2;
    SingleBunker_t* b3;
    SingleBunker_t* b4;

}bunkers_t;


bunkers_t* CreateBunkers();

#endif
