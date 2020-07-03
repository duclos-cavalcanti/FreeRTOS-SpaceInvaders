#ifndef __main_H__
#define __main_H__

#define PLAYERSHIP_HEIGHT 10
#define PLAYERSHIP_WIDTH 30

#define CREATURE_HEIGHT 23
#define CREATURE_WIDTH 25

#define BOTTOM_WALLTHICKNESS 3
#define BOTTOM_WALLPOSITION SCREEN_HEIGHT*95/100


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
    Row_4,
    Row_5
}Rows_t;

#define BEGIN MainMenuState
typedef enum GameState_t{
    MainMenuState,
    PlayingState,
    NextLevelState,
    PausedState,
    GameOverState
}GameState_t;

typedef enum SelectedMenuOption_t{
    SinglePlayer,
    MultiPlayer,
    Leave
}SelectedMenuOption_t;

typedef enum PlayerOutsideGameActions_t{
    NoAction,
    PauseGameAction,
    LostGameAction 
}PlayerOutsideGameActions_t;

typedef enum PausedGameActions_t{
    RemainPaused,
    ResumeGame
}PausedGameActions_t;

typedef enum SelectedGameOverOption_t{
    PlayAgain,
    Quit
}SelectedGameOverOption_t;

#endif 
