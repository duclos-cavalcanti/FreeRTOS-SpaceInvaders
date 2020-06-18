#ifndef __ship_H__
#define __ship_H__

typedef struct ship_t{
    signed short x_pos;
    signed short y_pos;

    unsigned int color;
    signed short radius;
}ship_t;

ship_t* CreateShip(signed short initial_x, signed short initial_y, 
                   unsigned int ShipColor, signed short size);


#endif 
