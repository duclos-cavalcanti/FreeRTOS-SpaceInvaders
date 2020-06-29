#ifndef __main_H__
#define __main_H__

#define PLAYERSHIP_HEIGHT 10
#define PLAYERSHIP_WIDTH 30

#define CREATURE_HEIGHT 23
#define CREATURE_WIDTH 25

#define BOTTOM_WALLTHICKNESS 3
#define BOTTOM_WALLPOSITION SCREEN_HEIGHT*95/100

void checkDraw(unsigned char status, const char *msg);

typedef enum Rows_t{
    Row_1,
    Row_2,
    Row_3,
    Row_4
}Rows_t;

#endif 
