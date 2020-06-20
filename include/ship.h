#ifndef __ship_H__
#define __ship_H__

typedef struct bullet_t{
signed short x_pos;
signed short y_pos;
signed short speed;

signed short color;

}bullet_t;

typedef struct ship_t{
    signed short x_pos;
    signed short y_pos;
    signed short speed;

    bullet_t* bullet;
    unsigned int color;
    signed short radius;
}ship_t;

ship_t* CreateShip(signed short initial_x, signed short initial_y, signed short speed, 
                   unsigned int ShipColor, signed short size);

void vIncrementShipLeft(ship_t* ship);
void vIncrementShipRight(ship_t* ship);

void CreateBullet(ship_t* ship);
void vUpdateShipBulletPos(ship_t* ship);
void vDrawShipBullet(ship_t* ship);
#endif 
