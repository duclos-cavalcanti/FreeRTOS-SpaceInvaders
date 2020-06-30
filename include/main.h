#ifndef __main_H__
#define __main_H__

#define PLAYERSHIP_HEIGHT 10
#define PLAYERSHIP_WIDTH 30

#define CREATURE_HEIGHT 23
#define CREATURE_WIDTH 25

#define BOTTOM_WALLTHICKNESS 3
#define BOTTOM_WALLPOSITION SCREEN_HEIGHT*95/100

#define STATE_DEBOUNCE_DELAY 1000

#define STATE_ONE 0
#define STATE_TWO 1
#define STATE_THREE 2

#define STATE_COUNT 2
#define STATE_Q_LEN 1
#define BEGIN STATE_ONE

#define NEXT_TASK 0
#define PREV_TASK 1

void checkDraw(unsigned char status, const char *msg);

typedef enum Columns_t{
    Column_1,
    Column_2,
    Column_3,
    Column_4,
    Column_5,
    Column_6,
    Column_7,
    Column_8
}Columns_t;

typedef enum Rows_t{
    Row_1,
    Row_2,
    Row_3,
    Row_4
}Rows_t;

typedef enum GameState_t{
    MainMenu,
    Game,
    Paused,
    GameOver
}GameState_t;

typedef enum SelectedMenuOption_t{
    SinglePlayer,
    MultiPlayer,
    Quit,
    Pause
}SelectedMenuOption_t;


#endif 
